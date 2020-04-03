// marlin_client.c

#include "marlin_client.h"
#include <stdarg.h>
#include <string.h>
#include "dbg.h"
#include "app.h"
#include "bsod.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "ffconf.h"

#define DBG _dbg1 //enabled level 1
//#define DBG(...)

//#define DBG_REQ  DBG    //trace requests (client side)
#define DBG_REQ(...) //disable trace

#define DBG_EVT DBG //trace events (client side)
//#define DBG_EVT(...)    //disable trace
#define DBG_EVT_MSK (MARLIN_EVT_MSK_ALL & ~MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge))

//#define DBG_VAR  DBG    //trace variable change notifications  (client side)
#define DBG_VAR(...) //disable trace
//#define DBG_VAR_MSK     MARLIN_VAR_MSK_ALL
//#define DBG_VAR_MSK     (MARLIN_VAR_MSK(MARLIN_VAR_GQUEUE) | MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE))
/*#define DBG_VAR_MSK     ( \
						MARLIN_VAR_MSK(MARLIN_VAR_MOTION) | \
						MARLIN_VAR_MSK(MARLIN_VAR_GQUEUE) | \
						MARLIN_VAR_MSK(MARLIN_VAR_PQUEUE) | \
						MARLIN_VAR_MSK(MARLIN_VAR_POS_X) | \
						MARLIN_VAR_MSK(MARLIN_VAR_POS_Y) | \
						MARLIN_VAR_MSK(MARLIN_VAR_POS_Z) | \
						MARLIN_VAR_MSK(MARLIN_VAR_POS_E) )*/

#pragma pack(push)
#pragma pack(1)

// client
typedef struct _marlin_client_t {
    uint8_t id;          // client id (0..MARLIN_MAX_CLIENTS-1)
    uint16_t flags;      // client flags (MARLIN_CFLG_xxx)
    uint64_t events;     // event mask
    uint64_t changes;    // variable change mask
    marlin_vars_t vars;  // cached variables
    uint32_t ack;        // cached ack value from last Acknowledge event
    uint16_t last_count; // number of messages received in last client loop
    uint64_t errors;
    marlin_mesh_t mesh;           // meshbed leveling
    uint32_t command;             // processed command (G28,G29,M701,M702,M600)
    marlin_host_prompt_t prompt;  // current host prompt structure (type and buttons)
    uint8_t reheating;            // reheating in progress
    fsm_create_t fsm_create_cb;   // to register callback for screen creation (M876), callback ensures M876 is processed asap, so there is no need for queue
    fsm_destroy_t fsm_destroy_cb; // to register callback for screen destruction
    fsm_change_t fsm_change_cb;   // to register callback for change of state
} marlin_client_t;

#pragma pack(pop)

//-----------------------------------------------------------------------------
// variables

osThreadId marlin_client_task[MARLIN_MAX_CLIENTS];    // task handles
osMessageQId marlin_client_queue[MARLIN_MAX_CLIENTS]; // input queue handles (uint32_t)

marlin_client_t marlin_client[MARLIN_MAX_CLIENTS]; // client structure
uint8_t marlin_clients = 0;                        // number of connected clients

//-----------------------------------------------------------------------------
// external variables from marlin_server

extern osThreadId marlin_server_task;    // task handle
extern osMessageQId marlin_server_queue; // input queue (uint8_t)
extern osSemaphoreId marlin_server_sema; // semaphore handle

//-----------------------------------------------------------------------------
// forward declarations of private functions

void _wait_server_started(void);
void _send_request_to_server(uint8_t client_id, const char *request);
uint32_t _wait_ack_from_server(uint8_t client_id);
void _process_client_message(marlin_client_t *client, variant8_t msg);
marlin_client_t *_client_ptr(void);

//-----------------------------------------------------------------------------
// client side public functions

marlin_vars_t *marlin_client_init(void) {
    int client_id;
    marlin_client_t *client = 0;
    _wait_server_started();
    osSemaphoreWait(marlin_server_sema, osWaitForever);
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if (marlin_client_task[client_id] == 0)
            break;
    if (client_id < MARLIN_MAX_CLIENTS) {
        client = marlin_client + client_id;
        memset(client, 0, sizeof(marlin_client_t));
        osMessageQDef(clientQueue, 32, uint32_t);
        marlin_client_queue[client_id] = osMessageCreate(osMessageQ(clientQueue), NULL);
        client->id = client_id;
        client->flags = 0;
        client->events = 0;
        client->changes = 0;
        marlin_clients++;
        client->flags |= (MARLIN_CFLG_STARTED | MARLIN_CFLG_PROCESS);
        client->errors = 0;
        client->mesh.xc = 4;
        client->mesh.yc = 4;
        client->command = MARLIN_CMD_NONE;
        client->reheating = 0;
        client->fsm_create_cb = NULL;
        client->fsm_destroy_cb = NULL;
        marlin_client_task[client_id] = osThreadGetId();
    }
    osSemaphoreRelease(marlin_server_sema);
    return (client) ? &(client->vars) : 0;
}

void marlin_client_shdn(void) {
    //TODO
}

void marlin_client_loop(void) {
    uint16_t count = 0;
    osEvent ose;
    variant8_t msg;
    int client_id;
    marlin_client_t *client;
    osMessageQId queue;
    osThreadId taskHandle = osThreadGetId();
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if (taskHandle == marlin_client_task[client_id])
            break;
    if (client_id >= MARLIN_MAX_CLIENTS)
        return;
    client = marlin_client + client_id;
    if ((queue = marlin_client_queue[client_id]) != 0)
        while ((ose = osMessageGet(queue, 0)).status == osEventMessage) {
            if (client->flags & MARLIN_CFLG_LOWHIGH) {
                *(((uint32_t *)(&msg)) + 1) = ose.value.v; //store high dword
                _process_client_message(client, msg);      //call handler
                count++;
            } else
                *(((uint32_t *)(&msg)) + 0) = ose.value.v; //store low dword
            client->flags ^= MARLIN_CFLG_LOWHIGH;          //flip flag
        }
    client->last_count = count;
}

int marlin_client_id(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->id;
    return 0;
}

//register callback to fsm creation
//return success
int marlin_client_set_fsm_create_cb(fsm_create_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->fsm_create_cb = cb;
        return 1;
    }
    return 0;
}

//register callback to fsm destruction
//return success
int marlin_client_set_fsm_destroy_cb(fsm_destroy_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->fsm_destroy_cb = cb;
        return 1;
    }
    return 0;
}

//register callback to fsm change
//return success
int marlin_client_set_fsm_change_cb(fsm_change_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->fsm_change_cb = cb;
        return 1;
    }
    return 0;
}
int marlin_processing(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return (client->flags & MARLIN_CFLG_PROCESS) ? 1 : 0;
    return 0;
}

int marlin_busy(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return (client->flags & MARLIN_CFLG_BUSY) ? 1 : 0;
    return 0;
}

uint32_t marlin_command(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->command;
    return MARLIN_CMD_NONE;
}

void marlin_stop_processing(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!stop");
    _wait_ack_from_server(client->id);
}

void marlin_start_processing(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!start");
    _wait_ack_from_server(client->id);
}

int marlin_motion(void) {
    marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MOTION));
    return (vars->motion) ? 1 : 0;
}

int marlin_wait_motion(uint32_t timeout) {
    marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MOTION));
    uint32_t tick = HAL_GetTick();
    while ((vars->motion == 0) && ((HAL_GetTick() - tick) < timeout))
        marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_MOTION));
    return (vars->motion) ? 1 : 0;
}

void marlin_gcode(const char *gcode) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    strcpy(request, "!g ");
    strcat(request, gcode);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

void marlin_json_gcode(const char *gcode) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    strcpy(request, "!g ");
    strlcat(request, gcode, MARLIN_MAX_REQUEST);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

int marlin_gcode_printf(const char *format, ...) {
    int ret;
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return 0;
    strcpy(request, "!g ");
    va_list ap;
    va_start(ap, format);
    ret = vsprintf(request + 3, format, ap);
    va_end(ap);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
    return ret;
}

void marlin_gcode_push_front(const char *gcode) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    snprintf(request, MARLIN_MAX_REQUEST, "!ig %p", gcode);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

int marlin_event(MARLIN_EVT_t evt_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << evt_id;
    if (client)
        ret = (client->events & msk) ? 1 : 0;
    return ret;
}

int marlin_event_set(MARLIN_EVT_t evt_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << evt_id;
    if (client) {
        ret = (client->events & msk) ? 1 : 0;
        client->events |= msk;
    }
    return ret;
}

int marlin_event_clr(MARLIN_EVT_t evt_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << evt_id;
    if (client) {
        ret = (client->events & msk) ? 1 : 0;
        client->events &= ~msk;
    }
    return ret;
}

uint64_t marlin_events(void) {
    marlin_client_t *client = _client_ptr();
    return (client) ? client->events : 0;
}

int marlin_change(uint8_t var_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << var_id;
    if (client)
        ret = (client->changes & msk) ? 1 : 0;
    return ret;
}

int marlin_change_set(uint8_t var_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << var_id;
    if (client) {
        ret = (client->changes & msk) ? 1 : 0;
        client->changes |= msk;
    }
    return ret;
}

int marlin_change_clr(uint8_t var_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << var_id;
    if (client) {
        ret = (client->changes & msk) ? 1 : 0;
        client->changes &= ~msk;
    }
    return ret;
}

uint64_t marlin_changes(void) {
    marlin_client_t *client = _client_ptr();
    return (client) ? client->changes : 0;
}

int marlin_error(uint8_t err_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << err_id;
    if (client)
        ret = (client->errors & msk) ? 1 : 0;
    return ret;
}

int marlin_error_set(uint8_t err_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << err_id;
    if (client) {
        ret = (client->errors & msk) ? 1 : 0;
        client->errors |= msk;
    }
    return ret;
}

int marlin_error_clr(uint8_t err_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << err_id;
    if (client) {
        ret = (client->errors & msk) ? 1 : 0;
        client->errors &= ~msk;
    }
    return ret;
}

uint64_t marlin_errors(void) {
    marlin_client_t *client = _client_ptr();
    return (client) ? client->errors : 0;
}

variant8_t marlin_get_var(uint8_t var_id) {
    marlin_client_t *client = _client_ptr();
    return (client) ? marlin_vars_get_var(&(client->vars), var_id) : variant8_empty();
}

variant8_t marlin_set_var(uint8_t var_id, variant8_t val) {
    variant8_t retval = variant8_empty();
    char request[MARLIN_MAX_REQUEST];
    int n;
    marlin_client_t *client = _client_ptr();
    if (client) {
        retval = marlin_vars_get_var(&(client->vars), var_id);
        marlin_vars_set_var(&(client->vars), var_id, val);
        n = sprintf(request, "!var %s ", marlin_vars_get_name(var_id));
        marlin_vars_value_to_str(&(client->vars), var_id, request + n);
        _send_request_to_server(client->id, request);
        _wait_ack_from_server(client->id);
    }
    return retval;
}

marlin_vars_t *marlin_vars(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return &(client->vars);
    return 0;
}

marlin_vars_t *marlin_update_vars(uint64_t msk) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return 0;
    marlin_client_loop();
    client->changes &= ~msk;
    sprintf(request, "!update %08lx %08lx", (uint32_t)(msk & 0xffffffff), (uint32_t)(msk >> 32));
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
    return &(client->vars);
}

void marlin_set_printing_gcode_name(const char *filename_pntr) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0) {
        return;
    }
    uint32_t filename_len = strnlen(filename_pntr, _MAX_LFN);
    if (_MAX_LFN == filename_len) {
        _dbg0("error!: filename string is not null terminated");
    }
    snprintf(request, MARLIN_MAX_REQUEST, "!gfileset %p", filename_pntr);
    marlin_event_clr(MARLIN_EVT_GFileChange);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

void marlin_get_printing_gcode_name(char *filename_pntr) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0) {
        return;
    }
    snprintf(request, MARLIN_MAX_REQUEST, "!gfileget %p", filename_pntr);
    _send_request_to_server(client->id, request);
}

uint8_t marlin_get_gqueue(void) {
    return marlin_get_var(MARLIN_VAR_GQUEUE).ui8;
}

uint8_t marlin_get_gqueue_max(void) {
    //TODO: variable gqueue_max should be part of marlin_consts structure transmited from server
    //Marlin/queue, BUFSIZE - 1
    return 8 - 1;
}

uint8_t marlin_get_pqueue(void) {
    return marlin_get_var(MARLIN_VAR_PQUEUE).ui8;
}

uint8_t marlin_get_pqueue_max(void) {
    //TODO: variable pqueue_max should be part of marlin_consts structure transmited from server
    //Marlin/planner, BLOCK_BUFFER_SIZE - 1
    return 16 - 1;
}

float marlin_set_target_nozzle(float val) {
    return marlin_set_var(MARLIN_VAR_TTEM_NOZ, variant8_flt(val)).flt;
}

float marlin_set_target_bed(float val) {
    return marlin_set_var(MARLIN_VAR_TTEM_BED, variant8_flt(val)).flt;
}

float marlin_set_z_offset(float val) {
    return marlin_set_var(MARLIN_VAR_Z_OFFSET, variant8_flt(val)).flt;
}

uint8_t marlin_set_fan_speed(uint8_t val) {
    return marlin_set_var(MARLIN_VAR_FANSPEED, variant8_ui8(val)).ui8;
}

uint16_t marlin_set_print_speed(uint16_t val) {
    return marlin_set_var(MARLIN_VAR_PRNSPEED, variant8_ui16(val)).ui16;
}

uint16_t marlin_set_flow_factor(uint16_t val) {
    return marlin_set_var(MARLIN_VAR_FLOWFACT, variant8_ui16(val)).ui16;
}

uint8_t marlin_set_wait_heat(uint8_t val) {
    return marlin_set_var(MARLIN_VAR_WAITHEAT, variant8_ui8(val)).ui8;
}

uint8_t marlin_set_wait_user(uint8_t val) {
    return marlin_set_var(MARLIN_VAR_WAITUSER, variant8_ui8(val)).ui8;
}

void marlin_do_babysteps_Z(float offs) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    sprintf(request, "!babystep_Z %.4f", (double)offs);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

void marlin_settings_save(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!cfg_save");
    _wait_ack_from_server(client->id);
}

void marlin_settings_load(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!cfg_load");
    _wait_ack_from_server(client->id);
}

void marlin_settings_reset(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!cfg_reset");
    _wait_ack_from_server(client->id);
}

void marlin_manage_heater(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!updt");
    _wait_ack_from_server(client->id);
}

void marlin_quick_stop(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!qstop");
    _wait_ack_from_server(client->id);
}

void marlin_print_abort(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!pabort");
    _wait_ack_from_server(client->id);
}

void marlin_print_pause(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!ppause");
    _wait_ack_from_server(client->id);
}

void marlin_print_resume(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!presume");
    _wait_ack_from_server(client->id);
}

void marlin_park_head(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!park");
    _wait_ack_from_server(client->id);
}

uint8_t marlin_message_received(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return 0;
    if (client->flags & MARLIN_CFLG_MESSAGE) {
        client->flags &= ~MARLIN_CFLG_MESSAGE;
        return 1;
    } else
        return 0;
}

//-----------------------------------------------------------------------------
// host functions

host_prompt_type_t marlin_host_prompt_type(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->prompt.type;
    return HOST_PROMPT_None;
}

uint8_t marlin_host_button_count(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->prompt.button_count;
    return 0;
}

host_prompt_button_t marlin_host_button_type(uint8_t index) {
    marlin_client_t *client = _client_ptr();
    if (client && (index < client->prompt.button_count))
        return client->prompt.button[index];
    return HOST_PROMPT_BTN_None;
}

void marlin_host_button_click(host_prompt_button_t button) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    sprintf(request, "!hclick %d", (int)button);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

// returns 1 if reheating is in progress, otherwise 0
int marlin_reheating(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->reheating;
    return 0;
}

//-----------------------------------------------------------------------------
// responses from client finite state machine (like button click)
void marlin_encoded_response(uint32_t enc_phase_and_response) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    sprintf(request, "!fsm_r %d", (int)enc_phase_and_response);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

//-----------------------------------------------------------------------------
// private functions

// wait while server not started (called from client thread in marlin_client_init)
void _wait_server_started(void) {
    while (marlin_server_task == 0)
        osDelay(1);
}

// send request to server (called from client thread), infinite timeout
void _send_request_to_server(uint8_t client_id, const char *request) {
    int ret = 0;
    int len = strlen(request);
    osMessageQId queue = 0;
    int i;
    osSemaphoreWait(marlin_server_sema, osWaitForever); // lock
    if ((queue = marlin_server_queue) != 0)             // queue valid
    {
        marlin_client[client_id].events &= ~MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge);
        while (ret == 0) {
            if (osMessageAvailableSpace(queue) >= (uint32_t)(len + 1)) // check available space
            {
                osMessagePut(queue, '0' + client_id, osWaitForever); // one character client id
                for (i = 0; i < len; i++)                            // loop over every characters
                    osMessagePut(queue, request[i], osWaitForever);  //
                if ((i > 0) && (request[i - 1] != '\n'))             // automatically terminate with '\n'
                    osMessagePut(queue, '\n', osWaitForever);
                ret = 1;
            } else {
                osSemaphoreRelease(marlin_server_sema); // unlock
                osDelay(10);
                osSemaphoreWait(marlin_server_sema, osWaitForever); // lock
            }
        }
    }
    osSemaphoreRelease(marlin_server_sema); // unlock
    DBG_REQ("CL%c: REQ %s", '0' + client_id, request);
    //return ret;
}

// wait for ack event, blocking - used for synchronization, called typicaly at end of client request functions
uint32_t _wait_ack_from_server(uint8_t client_id) {
    while ((marlin_client[client_id].events & MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge)) == 0) {
        marlin_client_loop();
        if (marlin_client[client_id].last_count == 0)
            osDelay(10);
    }
    marlin_client[client_id].events &= ~MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge);
    return marlin_client[client_id].ack;
}

// process message on client side (set flags, update vars etc.)
void _process_client_message(marlin_client_t *client, variant8_t msg) {
    char var_str[16];
    uint8_t id = msg.usr8 & MARLIN_USR8_MSK_ID;
    if (msg.usr8 & MARLIN_USR8_VAR_FLG) // variable change received
    {
        marlin_vars_set_var(&(client->vars), id, msg);
        client->changes |= ((uint64_t)1 << id);
        marlin_vars_value_to_str(&(client->vars), id, var_str);
#ifdef DBG_VAR_MSK
        if (DBG_VAR_MSK & ((uint64_t)1 << id))
#endif //DBG_VAR_MSK
            DBG_VAR("CL%c: VAR %s %s", '0' + client->id, marlin_vars_get_name(id), var_str);
    } else if (msg.type == VARIANT8_USER) // event received
    {
        client->events |= ((uint64_t)1 << id);
        switch ((MARLIN_EVT_t)id) {
        case MARLIN_EVT_MeshUpdate: {
            uint8_t x = msg.usr16 & 0xff;
            uint8_t y = msg.usr16 >> 8;
            float z = msg.flt;
            client->mesh.z[x + client->mesh.xc * y] = z;
        } break;
        case MARLIN_EVT_HostPrompt:
            marlin_host_prompt_decode(msg.ui32, &(client->prompt));
            break;
        case MARLIN_EVT_StartProcessing:
            client->flags |= MARLIN_CFLG_PROCESS;
            break;
        case MARLIN_EVT_StopProcessing:
            client->flags &= ~MARLIN_CFLG_PROCESS;
            break;
        case MARLIN_EVT_Busy:
            client->flags |= MARLIN_CFLG_BUSY;
            break;
        case MARLIN_EVT_Ready:
            client->flags &= ~MARLIN_CFLG_BUSY;
            break;
        case MARLIN_EVT_Error:
            client->errors |= MARLIN_ERR_MSK(msg.ui32);
            break;
        case MARLIN_EVT_CommandBegin:
            client->command = msg.ui32;
            break;
        case MARLIN_EVT_CommandEnd:
            client->command = MARLIN_CMD_NONE;
            break;
        case MARLIN_EVT_Message:
            client->flags |= MARLIN_CFLG_MESSAGE;
            break;
        case MARLIN_EVT_Reheat:
            client->reheating = (uint8_t)msg.ui32;
            break;
        case MARLIN_EVT_Acknowledge:
            client->ack = msg.ui32;
            break;
        case MARLIN_EVT_FSM_Create:
            if (client->fsm_create_cb)
                client->fsm_create_cb((uint8_t)msg.ui32, (uint8_t)(msg.ui32 >> 8));
            break;
        case MARLIN_EVT_FSM_Destroy:
            if (client->fsm_destroy_cb)
                client->fsm_destroy_cb((uint8_t)msg.ui32);
            break;
        case MARLIN_EVT_FSM_Change:
            if (client->fsm_change_cb)
                client->fsm_change_cb((uint8_t)msg.ui32, (uint8_t)(msg.ui32 >> 8), (uint8_t)(msg.ui32 >> 16), (uint8_t)(msg.ui32 >> 24));
            break;
            //not handled events
            //do not use default, i want all events listed here, so new event will generate warning, when not added
        case MARLIN_EVT_Startup:
        case MARLIN_EVT_PrinterKilled:
        case MARLIN_EVT_MediaInserted:
        case MARLIN_EVT_MediaError:
        case MARLIN_EVT_MediaRemoved:
        case MARLIN_EVT_PlayTone:
        case MARLIN_EVT_PrintTimerStarted:
        case MARLIN_EVT_PrintTimerPaused:
        case MARLIN_EVT_PrintTimerStopped:
        case MARLIN_EVT_FilamentRunout:
        case MARLIN_EVT_UserConfirmRequired:
        case MARLIN_EVT_StatusChanged:
        case MARLIN_EVT_FactoryReset:
        case MARLIN_EVT_LoadSettings:
        case MARLIN_EVT_StoreSettings:
        case MARLIN_EVT_SafetyTimerExpired:
        case MARLIN_EVT_GFileChange:
            break;
        }
#ifdef DBG_EVT_MSK
        if (DBG_EVT_MSK & ((uint64_t)1 << id))
#endif
            switch (id) {
            // Event MARLIN_EVT_MeshUpdate - ui32 is float z, ui16 low byte is x index, high byte y index
            case MARLIN_EVT_MeshUpdate: {
                uint8_t x = msg.usr16 & 0xff;
                uint8_t y = msg.usr16 >> 8;
                float z = msg.flt;
                DBG_EVT("CL%c: EVT %s %d %d %.3f", '0' + client->id, marlin_events_get_name(id),
                    x, y, (double)z);
                x = x;
                y = y;
                z = z; //prevent warning
            } break;
            // Event MARLIN_EVT_CommandBegin/End - ui32 is encoded command
            case MARLIN_EVT_CommandBegin:
            case MARLIN_EVT_CommandEnd:
                DBG_EVT("CL%c: EVT %s %c%u", '0' + client->id, marlin_events_get_name(id),
                    (msg.ui32 >> 16) & 0xff, msg.ui32 & 0xffff);
                break;
            // Event MARLIN_EVT_Acknowledge - ui32 is result (not used in this time)
            case MARLIN_EVT_Reheat:
            case MARLIN_EVT_Acknowledge:
                DBG_EVT("CL%c: EVT %s %lu", '0' + client->id, marlin_events_get_name(id), msg.ui32);
                break;
            // Other events and events without arguments
            default:
                DBG_EVT("CL%c: EVT %s", '0' + client->id, marlin_events_get_name(id));
                break;
            }
    }
}

// returns client pointer for calling client thread (client thread)
marlin_client_t *_client_ptr(void) {
    osThreadId taskHandle = osThreadGetId();
    int client_id;
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if (taskHandle == marlin_client_task[client_id])
            return marlin_client + client_id;
    return 0;
}
