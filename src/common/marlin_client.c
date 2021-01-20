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

//trace event notification (client side), to disable trace undef DBG_EVT_MSK
//#define DBG_EVT DBG
//#define DBG_EVT_MSK (MARLIN_EVT_MSK_ALL & ~MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge))

//trace variable change notifications (client side), to disable trace undef DBG_VAR_MSK
//#define DBG_VAR     DBG
//#define DBG_VAR_MSK (MARLIN_VAR_MSK_ALL & ~MARLIN_VAR_MSK_TEMP_ALL)

//maximum string length for DBG_VAR
enum {
    DBG_VAR_STR_MAX_LEN = 128
};

// client
typedef struct _marlin_client_t {
    uint64_t events;  // event mask
    uint64_t changes; // variable change mask
    uint64_t errors;
    marlin_mesh_t mesh; // meshbed leveling

    marlin_vars_t vars;           // cached variables
    uint32_t ack;                 // cached ack value from last Acknowledge event
    uint32_t command;             // processed command (G28,G29,M701,M702,M600)
    fsm_create_t fsm_create_cb;   // to register callback for screen creation (M876), callback ensures M876 is processed asap, so there is no need for queue
    fsm_destroy_t fsm_destroy_cb; // to register callback for screen destruction
    fsm_change_t fsm_change_cb;   // to register callback for change of state
    message_cb_t message_cb;      // to register callback message
    warning_cb_t warning_cb;      // to register callback for important message
    startup_cb_t startup_cb;      // to register callback after marlin complete initialization

    uint16_t flags;      // client flags (MARLIN_CFLG_xxx)
    uint16_t last_count; // number of messages received in last client loop

    uint8_t id;        // client id (0..MARLIN_MAX_CLIENTS-1)
    uint8_t reheating; // reheating in progress
} marlin_client_t;

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

static void _wait_server_started(void);
static void _send_request_to_server(uint8_t client_id, const char *request);
static uint32_t _wait_ack_from_server(uint8_t client_id);
static uint32_t _wait_ack_from_server_with_callback(uint8_t client_id, void (*cb)());
static void _process_client_message(marlin_client_t *client, variant8_t msg);
static marlin_client_t *_client_ptr(void);

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
        osMessageQDef(clientQueue, MARLIN_CLIENT_QUEUE * 2, uint32_t);
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
        client->fsm_change_cb = NULL;
        client->message_cb = NULL;
        client->warning_cb = NULL;
        client->startup_cb = NULL;
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
    variant8_t *pmsg = &msg;
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
                msg |= ((variant8_t)ose.value.v << 32); //store high dword
                _process_client_message(client, msg);   //call handler
                variant8_done(&pmsg);
                count++;
            } else
                msg = ose.value.v;                //store low dword
            client->flags ^= MARLIN_CFLG_LOWHIGH; //flip flag
        }
    client->last_count = count;
}

int marlin_client_id(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->id;
    return 0;
}

void marlin_client_wait_for_start_processing(void) {
    // wait for startup
    while (!marlin_event_clr(MARLIN_EVT_StartProcessing)) {
        marlin_client_loop();
        osDelay(100);
    }
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

//register callback to message
//return success
int marlin_client_set_message_cb(message_cb_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->message_cb = cb;
        return 1;
    }
    return 0;
}

//register callback to warning_cb_t (fan failure, heater timeout ...)
//return success
int marlin_client_set_warning_cb(warning_cb_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->warning_cb = cb;
        return 1;
    }
    return 0;
}

//register callback to startup_cb_t (complete initialization)
//return success
int marlin_client_set_startup_cb(startup_cb_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->startup_cb = cb;
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

void marlin_client_set_event_notify(uint64_t notify_events, void (*cb)()) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client) {
        snprintf(request, MARLIN_MAX_REQUEST, "!event_msk %08lx %08lx", (uint32_t)(notify_events & 0xffffffff), (uint32_t)(notify_events >> 32));
        _send_request_to_server(client->id, request);
        _wait_ack_from_server_with_callback(client->id, cb);
    }
}

void marlin_client_set_change_notify(uint64_t notify_changes, void (*cb)()) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client) {
        snprintf(request, MARLIN_MAX_REQUEST, "!change_msk %08lx %08lx", (uint32_t)(notify_changes & 0xffffffff), (uint32_t)(notify_changes >> 32));
        _send_request_to_server(client->id, request);
        _wait_ack_from_server_with_callback(client->id, cb);
    }
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

void marlin_set_exclusive_mode(int exclusive_mode) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client) {
        snprintf(request, MARLIN_MAX_REQUEST, "!exc %d", exclusive_mode);
        _send_request_to_server(client->id, request);
        _wait_ack_from_server(client->id);
    }
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
    strlcpy(request, "!g ", MARLIN_MAX_REQUEST);
    strlcat(request, gcode, MARLIN_MAX_REQUEST);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

void marlin_json_gcode(const char *gcode) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    strlcpy(request, "!g ", MARLIN_MAX_REQUEST);
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
    strlcpy(request, "!g ", MARLIN_MAX_REQUEST);
    va_list ap;
    va_start(ap, format);
    ret = vsnprintf(request + 3, MARLIN_MAX_REQUEST - 3, format, ap);
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
    marlin_client_t *client = _client_ptr();
    if (client) {
        retval = marlin_vars_get_var(&(client->vars), var_id);
        marlin_vars_set_var(&(client->vars), var_id, val);
        const int n = snprintf(request, MARLIN_MAX_REQUEST, "!var %s ", marlin_vars_get_name(var_id));
        if (n < 0)
            bsod("Error formatting var name.");
        const int v = marlin_vars_value_to_str(&(client->vars), var_id, request + n, sizeof(request) - n);
        if (v < 0)
            bsod("Error formatting var value.");
        if (((size_t)v + (size_t)n) >= sizeof(request))
            bsod("Request too long.");
        _send_request_to_server(client->id, request);
        _wait_ack_from_server(client->id);
    }
    return retval;
}

marlin_vars_t *marlin_vars(void) {
    marlin_client_t *client = _client_ptr();
    if (client)
        return &(client->vars);
    //TODO call PSOD - fatal error
    return 0;
}

marlin_vars_t *marlin_update_vars(uint64_t msk) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return 0;
    marlin_client_loop();
    client->changes &= ~msk;
    snprintf(request, MARLIN_MAX_REQUEST, "!update %08lx %08lx", (uint32_t)(msk & 0xffffffff), (uint32_t)(msk >> 32));
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
    return &(client->vars);
}

uint8_t marlin_get_gqueue(void) {
    return variant_get_ui8(marlin_get_var(MARLIN_VAR_GQUEUE));
}

uint8_t marlin_get_gqueue_max(void) {
    //TODO: variable gqueue_max should be part of marlin_consts structure transmited from server
    //Marlin/queue, BUFSIZE - 1
    return 8 - 1;
}

uint8_t marlin_get_pqueue(void) {
    return variant_get_ui8(marlin_get_var(MARLIN_VAR_PQUEUE));
}

uint8_t marlin_get_pqueue_max(void) {
    //TODO: variable pqueue_max should be part of marlin_consts structure transmited from server
    //Marlin/planner, BLOCK_BUFFER_SIZE - 1
    return 16 - 1;
}

float marlin_set_target_nozzle(float val) {
    return variant8_get_flt(marlin_set_var(MARLIN_VAR_TTEM_NOZ, variant8_flt(val)));
}

float marlin_set_display_nozzle(float val) {
    return variant8_get_flt(marlin_set_var(MARLIN_VAR_DTEM_NOZ, variant8_flt(val)));
}

float marlin_set_target_bed(float val) {
    return variant8_get_flt(marlin_set_var(MARLIN_VAR_TTEM_BED, variant8_flt(val)));
}

float marlin_set_z_offset(float val) {
    return variant8_get_flt(marlin_set_var(MARLIN_VAR_Z_OFFSET, variant8_flt(val)));
}

uint8_t marlin_set_fan_speed(uint8_t val) {
    return variant_get_ui8(marlin_set_var(MARLIN_VAR_FANSPEED, variant8_ui8(val)));
}

uint16_t marlin_set_print_speed(uint16_t val) {
    return variant_get_ui16(marlin_set_var(MARLIN_VAR_PRNSPEED, variant8_ui16(val)));
}

uint16_t marlin_set_flow_factor(uint16_t val) {
    return variant_get_ui16(marlin_set_var(MARLIN_VAR_FLOWFACT, variant8_ui16(val)));
}

uint8_t marlin_set_wait_heat(uint8_t val) {
    return variant_get_ui8(marlin_set_var(MARLIN_VAR_WAITHEAT, variant8_ui8(val)));
}

uint8_t marlin_set_wait_user(uint8_t val) {
    return variant_get_ui8(marlin_set_var(MARLIN_VAR_WAITUSER, variant8_ui8(val)));
}

void marlin_do_babysteps_Z(float offs) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    snprintf(request, MARLIN_MAX_REQUEST, "!babystep_Z %.4f", (double)offs);
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

void marlin_test_start(uint32_t mask) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    snprintf(request, MARLIN_MAX_REQUEST, "!test %u", (unsigned int)mask);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

void marlin_test_abort(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!tabort");
    _wait_ack_from_server(client->id);
}

void marlin_print_start(const char *filename) {
    char request[MARLIN_MAX_REQUEST];
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    const int len = snprintf(request, sizeof(request), "!pstart %s", filename);
    if (len < 0)
        bsod("Error formatting request.");
    if ((size_t)len >= sizeof(request))
        bsod("Request too long.");
    _send_request_to_server(client->id, request);
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

void marlin_notify_server_about_encoder_move(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!kmove");
    _wait_ack_from_server(client->id);
}

void marlin_notify_server_about_knob_click(void) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    _send_request_to_server(client->id, "!kclick");
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
    snprintf(request, MARLIN_MAX_REQUEST, "!fsm_r %d", (int)enc_phase_and_response);
    _send_request_to_server(client->id, request);
    _wait_ack_from_server(client->id);
}

//-----------------------------------------------------------------------------
// private functions

// wait while server not started (called from client thread in marlin_client_init)
static void _wait_server_started(void) {
    while (marlin_server_task == 0)
        osDelay(1);
}

// send request to server (called from client thread), infinite timeout
static void _send_request_to_server(uint8_t client_id, const char *request) {
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
static uint32_t _wait_ack_from_server_with_callback(uint8_t client_id, void (*cb)()) {
    while ((marlin_client[client_id].events & MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge)) == 0) {
        marlin_client_loop();
        if (marlin_client[client_id].last_count == 0) {
            if (cb)
                cb();
            osDelay(10);
        }
    }
    marlin_client[client_id].events &= ~MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge);
    return marlin_client[client_id].ack;
}

static uint32_t _wait_ack_from_server(uint8_t client_id) {
    return _wait_ack_from_server_with_callback(client_id, NULL);
}

// process message on client side (set flags, update vars etc.)
static void _process_client_message(marlin_client_t *client, variant8_t msg) {
    uint8_t id = variant8_get_usr8(msg) & MARLIN_USR8_MSK_ID;
    if (variant8_get_usr8(msg) & MARLIN_USR8_VAR_FLG) // variable change received
    {
        marlin_vars_set_var(&(client->vars), id, msg);
        client->changes |= ((uint64_t)1 << id);
#ifdef DBG_VAR_MSK
        char var_str[DBG_VAR_STR_MAX_LEN + 1];
        marlin_vars_value_to_str(&(client->vars), id, var_str, sizeof(var_str));
        if (DBG_VAR_MSK & ((uint64_t)1 << id))
            DBG_VAR("CL%c: VAR %s %s", '0' + client->id, marlin_vars_get_name(id), var_str);
#endif                                                  //DBG_VAR_MSK
    } else if (variant8_get_type(msg) == VARIANT8_USER) // event received
    {
        client->events |= ((uint64_t)1 << id);
        switch ((MARLIN_EVT_t)id) {
        case MARLIN_EVT_MeshUpdate: {
            uint8_t x = variant8_get_usr16(msg) & 0xff;
            uint8_t y = variant8_get_usr16(msg) >> 8;
            float z = variant8_get_flt(msg);
            client->mesh.z[x + client->mesh.xc * y] = z;
            break;
        }
        case MARLIN_EVT_StartProcessing:
            client->flags |= MARLIN_CFLG_PROCESS;
            break;
        case MARLIN_EVT_StopProcessing:
            client->flags &= ~MARLIN_CFLG_PROCESS;
            break;
        case MARLIN_EVT_Error:
            client->errors |= MARLIN_ERR_MSK(variant8_get_ui32(msg));
            break;
        case MARLIN_EVT_CommandBegin:
            client->command = variant8_get_ui32(msg);
            break;
        case MARLIN_EVT_CommandEnd:
            client->command = MARLIN_CMD_NONE;
            break;
        case MARLIN_EVT_Reheat:
            client->reheating = (uint8_t)variant8_get_ui32(msg);
            break;
        case MARLIN_EVT_Acknowledge:
            client->ack = variant8_get_ui32(msg);
            break;
        case MARLIN_EVT_FSM_Create:
            if (client->fsm_create_cb)
                client->fsm_create_cb((uint8_t)variant8_get_ui32(msg), (uint8_t)(variant8_get_ui32(msg) >> 8));
            break;
        case MARLIN_EVT_FSM_Destroy:
            if (client->fsm_destroy_cb)
                client->fsm_destroy_cb((uint8_t)variant8_get_ui32(msg));
            break;
        case MARLIN_EVT_FSM_Change:
            if (client->fsm_change_cb)
                client->fsm_change_cb((uint8_t)variant8_get_ui32(msg), (uint8_t)(variant8_get_ui32(msg) >> 8), (uint8_t)(variant8_get_ui32(msg) >> 16), (uint8_t)(variant8_get_ui32(msg) >> 24));
            break;
        case MARLIN_EVT_Message: {
            variant8_t *pvar = &msg;
            variant8_set_type(pvar, VARIANT8_PCHAR);
            const char *str = variant8_get_pch(msg);
            if (client->message_cb) {
                client->message_cb(str);
            }
            variant8_done(&pvar);
            break;
        }
        case MARLIN_EVT_Warning:
            if (client->warning_cb)
                client->warning_cb(variant8_get_i32(msg));
            break;
        case MARLIN_EVT_Startup:
            if (client->startup_cb) {
                client->startup_cb();
            }
            break;
            //not handled events
            //do not use default, i want all events listed here, so new event will generate warning, when not added
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
            break;
        }
#ifdef DBG_EVT_MSK
        if (DBG_EVT_MSK & ((uint64_t)1 << id))
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
#endif //DBG_EVT_MSK
    }
}

// returns client pointer for calling client thread (client thread)
static marlin_client_t *_client_ptr(void) {
    osThreadId taskHandle = osThreadGetId();
    int client_id;
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if (taskHandle == marlin_client_task[client_id])
            return marlin_client + client_id;
    return 0;
}
