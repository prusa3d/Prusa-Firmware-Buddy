// marlin_client.cpp

#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "bsod.h"
#include "ffconf.h"
#include "timing.h"
#include "log.h"
#include "../lib/Marlin/Marlin/src/core/macros.h"
#include "bsod_gui.hpp"

#if HAS_SELFTEST
    #include <selftest_types.hpp>
#endif

using namespace marlin_server;

LOG_COMPONENT_DEF(MarlinClient, LOG_SEVERITY_INFO);

//maximum string length for DBG_VAR
enum {
    DBG_VAR_STR_MAX_LEN = 128
};
static constexpr uint8_t max_retries = 5;

// client
typedef struct _marlin_client_t {
    uint64_t events; // event mask
    uint64_t errors;

    uint32_t ack;            // cached ack value from last Acknowledge event
    uint32_t command;        // processed command (G28,G29,M701,M702,M600)
    fsm_cb_t fsm_cb;         // to register callback for dialog or screen creation/destruction/change (M876), callback ensures M876 is processed asap, so there is no need for queue
    message_cb_t message_cb; // to register callback message
    warning_cb_t warning_cb; // to register callback for important message
    startup_cb_t startup_cb; // to register callback after marlin complete initialization

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
// forward declarations of private functions

static void _wait_server_started();
static void _send_request_to_server(uint8_t client_id, const char *request);
static uint32_t _wait_ack_from_server_with_callback(uint8_t client_id, void (*cb)());
static void _process_client_message(marlin_client_t *client, variant8_t msg);
static marlin_client_t *_client_ptr();

//-----------------------------------------------------------------------------
// client side public functions

void marlin_client_init() {
    int client_id;
    marlin_client_t *client = 0;
    _wait_server_started();
    osSemaphoreWait(server_semaphore, osWaitForever);
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
        marlin_clients++;
        client->flags |= (MARLIN_CFLG_STARTED | MARLIN_CFLG_PROCESS);
        client->errors = 0;
        client->command = MARLIN_CMD_NONE;
        client->reheating = 0;
        client->fsm_cb = NULL;
        client->message_cb = NULL;
        client->warning_cb = NULL;
        client->startup_cb = NULL;
        marlin_client_task[client_id] = osThreadGetId();
    }
    osSemaphoreRelease(server_semaphore);
}

void marlin_client_loop() {
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

int marlin_client_id() {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->id;
    return 0;
}

void marlin_client_wait_for_start_processing() {
    // wait for startup
    while (!marlin_event_clr(MARLIN_EVT_StartProcessing)) {
        marlin_client_loop();
        osDelay(100);
    }
}

//register callback to fsm creation
//return success
int marlin_client_set_fsm_cb(fsm_cb_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->fsm_cb = cb;
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

int marlin_processing() {
    marlin_client_t *client = _client_ptr();
    if (client)
        return (client->flags & MARLIN_CFLG_PROCESS) ? 1 : 0;
    return 0;
}

void _send_request_to_server_and_wait_with_callback(const char *request, void (*cb)()) {
    marlin_client_t *client = _client_ptr();
    if (client == 0)
        return;
    uint8_t retries_left = max_retries;

    do {
        _send_request_to_server(client->id, request);
        _wait_ack_from_server_with_callback(client->id, cb);
        if ((client->events & MARLIN_EVT_MSK(MARLIN_EVT_NotAcknowledge)) != 0) {
            // clear nack flag
            client->events &= ~MARLIN_EVT_MSK(MARLIN_EVT_NotAcknowledge);
            log_warning(MarlinClient, "Request %s to marlin server not acknowledged, retries left %u ", request, retries_left);
            // give marlin server time to process other requests
            osDelay(10);
        }
        retries_left--;
    } while ((client->events & MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge)) == 0 && retries_left > 0);

    if (retries_left <= 0) {
        fatal_error(ErrCode::ERR_SYSTEM_MARLIN_CLIENT_SERVER_REQUEST_TIMEOUT);
    }
    // clear the flag of ack events
    client->events &= ~MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge);
}

void marlin_client_set_event_notify(uint64_t notify_events, void (*cb)()) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%08lx %08lx", MARLIN_MSG_EVENT_MASK, (uint32_t)(notify_events & 0xffffffff), (uint32_t)(notify_events >> 32));
    _send_request_to_server_and_wait_with_callback(request, cb);
}

uint32_t marlin_command() {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->command;
    return MARLIN_CMD_NONE;
}

void _send_request_to_server_and_wait(const char *request) {
    _send_request_to_server_and_wait_with_callback(request, NULL);
}

void _send_request_id_to_server_and_wait(const marlin_msg_t id) {
    char request[3];
    marlin_msg_to_str(id, request);
    _send_request_to_server_and_wait(request);
}

void marlin_gcode(const char *gcode) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%s", MARLIN_MSG_GCODE, gcode);
    _send_request_to_server_and_wait(request);
}

int marlin_gcode_printf(const char *format, ...) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c", MARLIN_MSG_GCODE);
    va_list ap;
    va_start(ap, format);
    const int ret = vsnprintf(request + 2, MARLIN_MAX_REQUEST - 3, format, ap);
    va_end(ap);
    _send_request_to_server_and_wait(request);
    return ret;
}

void marlin_gcode_push_front(const char *gcode) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c0x%p", MARLIN_MSG_INJECT_GCODE, gcode);
    _send_request_to_server_and_wait(request);
}

int marlin_event(MARLIN_EVT_t evt_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << evt_id;
    if (client)
        ret = (client->events & msk) ? 1 : 0;
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

uint64_t marlin_events() {
    marlin_client_t *client = _client_ptr();
    return (client) ? client->events : 0;
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

uint64_t marlin_errors() {
    marlin_client_t *client = _client_ptr();
    return (client) ? client->errors : 0;
}

void marlin_do_babysteps_Z(float offs) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%.4f", MARLIN_MSG_BABYSTEP, (double)offs);
    _send_request_to_server_and_wait(request);
}

void marlin_move_axis(float pos, float feedrate, uint8_t axis) {
    char request[MARLIN_MAX_REQUEST];
    // check axis
    if (axis < 4) {
        snprintf(request, MARLIN_MAX_REQUEST, "!%c%.4f %.4f %u", MARLIN_MSG_MOVE, (double)pos, (double)feedrate, axis);
        _send_request_to_server_and_wait(request);
    }
}

void marlin_settings_save() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_CONFIG_SAVE);
}

void marlin_settings_load() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_CONFIG_LOAD);
}

void marlin_settings_reset() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_CONFIG_RESET);
}

#if HAS_SELFTEST
void marlin_test_start_for_tools(const uint64_t test_mask, const uint8_t tool_mask) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%08lx %08lx %08lx", MARLIN_MSG_TEST_START, (uint32_t)test_mask, (uint32_t)(test_mask >> 32), (uint32_t)tool_mask);
    _send_request_to_server_and_wait(request);
}

void marlin_test_start(const uint64_t test_mask) {
    marlin_test_start_for_tools(test_mask, AllTools);
}

void marlin_test_abort() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_TEST_ABORT);
}
#endif

void marlin_print_start(const char *filename, bool skip_preview) {
    char request[MARLIN_MAX_REQUEST];
    const int len = snprintf(request, sizeof(request), "!%c%c%s", MARLIN_MSG_PRINT_START, skip_preview ? '1' : '0', filename);
    if (len < 0)
        bsod("Error formatting request.");
    if ((size_t)len >= sizeof(request))
        bsod("Request too long.");
    _send_request_to_server_and_wait(request);
}

bool marlin_print_started() {
    // The above can't really return true/false if the print started, for two reasons:
    // * There doesn't seem to be a ready-made way to conveniently send a
    //   yes/no from the server to the client.
    // * Waiting for the answer could lead to a deadlock when called from the
    //   GUI thread, because the marlin server prepares the grounds and waits for
    //   GUI to ACK that everything is OK. But if GUI would be waiting for the
    //   server to answer, it couldn't answer.
    //
    // Therefore, we provide a separate function other threads may call
    // (connect and link) to find out if starting the print was processed or if
    // it was rejected.
    //
    // We also kind of ignore the possibility of the whole print successfully
    // happening before we can notice it. That would produce a false negative,
    // however that would likely result only in unexpected error message to the
    // user.

    while (true) {
        switch (marlin_vars()->print_state) {
        case mpsWaitGui:
            // We are still waiting for GUI to make up its mind. Do another round.
            osDelay(10);
            break;
        case mpsIdle:
        case mpsAborted:
        case mpsFinished:
            // Went to idle - refused by GUI
            return false;
        default:
            // Doing something else ‒ there's a lot of states where we are printing.
            return true;
        }
    }
}

void marlin_gui_ready_to_print() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_GUI_PRINT_READY);
}

void marlin_gui_cant_print() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_GUI_CANT_PRINT);
}

void marlin_print_abort() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_PRINT_ABORT);
}

void marlin_print_exit() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_PRINT_EXIT);
}

void marlin_print_pause() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_PRINT_PAUSE);
}

void marlin_print_resume() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_PRINT_RESUME);
}

void marlin_park_head() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_PARK);
}

void marlin_notify_server_about_encoder_move() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_KNOB_MOVE);
}

void marlin_notify_server_about_knob_click() {
    _send_request_id_to_server_and_wait(MARLIN_MSG_KNOB_CLICK);
}

// returns 1 if reheating is in progress, otherwise 0
int marlin_reheating() {
    marlin_client_t *client = _client_ptr();
    if (client)
        return client->reheating;
    return 0;
}

//-----------------------------------------------------------------------------
// responses from client finite state machine (like button click)
void marlin_encoded_response(uint32_t enc_phase_and_response) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%d", MARLIN_MSG_FSM, (int)enc_phase_and_response);
    _send_request_to_server_and_wait(request);
}
bool marlin_is_printing() {
    switch (marlin_vars()->print_state) {
    case mpsAborted:
    case mpsIdle:
    case mpsFinished:
    case mpsPrintPreviewInit:
    case mpsPrintPreviewImage:
        return false;
    default:
        return true;
    }
}
bool marlin_remote_print_ready(bool preview_only) {
    switch (marlin_vars()->print_state) {
    case mpsIdle:
        return true;
    case mpsPrintPreviewInit:
    case mpsPrintPreviewImage:
        // We want to replace the one-click print / preview when we want to
        // start printing. But we don't want to change one print preview to
        // another just by uploading stuff.
        return !preview_only;
    default:
        return false;
    }
}

//-----------------------------------------------------------------------------
// private functions

// wait while server not started (called from client thread in marlin_client_init)
static void _wait_server_started() {
    while (server_task == 0)
        osDelay(1);
}

// send request to server (called from client thread), infinite timeout
static void _send_request_to_server(uint8_t client_id, const char *request) {
    int ret = 0;
    int len = strlen(request);
    osMessageQId queue = 0;
    int i;
    osSemaphoreWait(server_semaphore, osWaitForever); // lock
    if ((queue = server_queue) != 0)                  // queue valid
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
                osSemaphoreRelease(server_semaphore); // unlock
                osDelay(10);
                osSemaphoreWait(server_semaphore, osWaitForever); // lock
            }
        }
    }
    osSemaphoreRelease(server_semaphore); // unlock

    log_info(MarlinClient, "Request (client %u): %s", client_id, request);
}

// wait for ack event, blocking - used for synchronization, called typically at end of client request functions
static uint32_t _wait_ack_from_server_with_callback(uint8_t client_id, void (*cb)()) {
    while ((marlin_client[client_id].events & MARLIN_EVT_MSK(MARLIN_EVT_Acknowledge)) == 0 && (marlin_client[client_id].events & MARLIN_EVT_MSK(MARLIN_EVT_NotAcknowledge)) == 0) {
        marlin_client_loop();
        if (marlin_client[client_id].last_count == 0) {
            if (cb)
                cb();
            osDelay(10);
        }
    }
    return marlin_client[client_id].ack;
}

// process message on client side (set flags, update vars etc.)
static void _process_client_message(marlin_client_t *client, variant8_t msg) {
    uint8_t id = variant8_get_usr8(msg) & MARLIN_USR8_MSK_ID;
    if (variant8_get_type(msg) == VARIANT8_USER) // event received
    {
        client->events |= ((uint64_t)1 << id);
        switch ((MARLIN_EVT_t)id) {
        case MARLIN_EVT_MeshUpdate: {
            uint8_t _UNUSED x = variant8_get_usr16(msg) & 0xff;
            uint8_t _UNUSED y = variant8_get_usr16(msg) >> 8;
            float _UNUSED z = variant8_get_flt(msg);
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
        case MARLIN_EVT_NotAcknowledge:
        case MARLIN_EVT_Acknowledge:
            client->ack = variant8_get_ui32(msg);
            break;
        case MARLIN_EVT_FSM:
            if (client->fsm_cb)
                client->fsm_cb(variant8_get_ui32(msg), variant8_get_usr16(msg));
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
                client->warning_cb(static_cast<WarningType>(variant8_get_i32(msg)));
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
static marlin_client_t *_client_ptr() {
    osThreadId taskHandle = osThreadGetId();
    int client_id;
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++)
        if (taskHandle == marlin_client_task[client_id])
            return marlin_client + client_id;
    return 0;
}

template <typename T>
void marlin_set_variable(MarlinVariable<T> &variable, T value) {
    char request[MARLIN_MAX_REQUEST];

    const int n = snprintf(request, MARLIN_MAX_REQUEST, "!%c%d ", MARLIN_MSG_SET_VARIABLE, variable.get_identifier());
    if (n < 0)
        bsod("Error formatting var name.");

    int v;
    if constexpr (std::is_floating_point<T>::value) {
        v = snprintf(request + n, sizeof(request) - n, "%f", (double)value);
    } else if constexpr (std::is_integral<T>::value) {
        v = snprintf(request + n, sizeof(request) - n, "%d", value);
    } else {
        bsod("no conversion");
    }
    if (v < 0)
        bsod("Error formatting var value.");
    if (((size_t)v + (size_t)n) >= sizeof(request))
        bsod("Request too long.");

    _send_request_to_server_and_wait(request);
}

void marlin_set_target_nozzle(float val, uint8_t hotend) {
    return marlin_set_variable(marlin_vars()->hotend(hotend).target_nozzle, val);
}
void marlin_set_display_nozzle(float val, uint8_t hotend) {
    return marlin_set_variable(marlin_vars()->hotend(hotend).display_nozzle, val);
}
void marlin_set_target_bed(float val) {
    return marlin_set_variable(marlin_vars()->target_bed, val);
}
void marlin_set_fan_speed(uint8_t val) {
    return marlin_set_variable(marlin_vars()->print_fan_speed, val);
}
void marlin_set_print_speed(uint16_t val) {
    return marlin_set_variable(marlin_vars()->print_speed, val);
}
void marlin_set_flow_factor(uint16_t val, uint8_t hotend) {
    return marlin_set_variable(marlin_vars()->hotend(hotend).flow_factor, val);
}
void marlin_set_z_offset(float val) {
    return marlin_set_variable(marlin_vars()->z_offset, val);
}
void marlin_set_fan_check(bool val) {
    return marlin_set_variable(marlin_vars()->fan_check_enabled, static_cast<uint8_t>(val));
}
void marlin_set_fs_autoload(bool val) {
    return marlin_set_variable(marlin_vars()->fs_autoload_enabled, static_cast<uint8_t>(val));
}
