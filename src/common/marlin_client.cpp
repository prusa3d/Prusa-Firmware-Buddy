// marlin_client.cpp

#include "marlin_client.hpp"
#include "marlin_events.h"
#include "marlin_server.hpp"
#include <cassert>
#include <stdio.h>
#include <string.h>
#include <cstdint>
#include "config.h"
#include "bsod.h"
#include "ffconf.h"
#include "log.h"
#include "../lib/Marlin/Marlin/src/core/macros.h"
#include <module/motion.h>
#include "bsod_gui.hpp"
#include "utility_extensions.hpp"
#include "variant8.h"
#include "tasks.hpp"

#if HAS_SELFTEST()
    #include <selftest_types.hpp>
#endif

using namespace marlin_server;

namespace marlin_client {

LOG_COMPONENT_DEF(MarlinClient, LOG_SEVERITY_INFO);

// maximum string length for DBG_VAR
enum {
    DBG_VAR_STR_MAX_LEN = 128
};
static constexpr uint8_t max_retries = 5;

// client
typedef struct _marlin_client_t {
    EventMask events; // event mask
    uint64_t errors;

    uint32_t ack; // cached ack value from last Acknowledge event
    uint32_t command; // processed command (G28,G29,M701,M702,M600)
    fsm_cb_t fsm_cb; // to register callback for dialog or screen creation/destruction/change (M876), callback ensures M876 is processed asap, so there is no need for queue
    message_cb_t message_cb; // to register callback message
    warning_cb_t warning_cb; // to register callback for important message
    startup_cb_t startup_cb; // to register callback after marlin complete initialization

    uint16_t flags; // client flags (MARLIN_CFLG_xxx)
    uint16_t last_count; // number of messages received in last client loop

    uint8_t id; // client id (0..MARLIN_MAX_CLIENTS-1)
    uint8_t reheating; // reheating in progress
} marlin_client_t;

//-----------------------------------------------------------------------------
// variables

osThreadId marlin_client_task[MARLIN_MAX_CLIENTS]; // task handles
osMessageQId marlin_client_queue[MARLIN_MAX_CLIENTS]; // input queue handles (uint32_t)

marlin_client_t clients[MARLIN_MAX_CLIENTS]; // client structure
uint8_t marlin_clients = 0; // number of connected clients

//-----------------------------------------------------------------------------
// forward declarations of private functions

static void _send_request_to_server(uint8_t client_id, const char *request);
static uint32_t _wait_ack_from_server_with_callback(uint8_t client_id, void (*cb)());
static void _process_client_message(marlin_client_t *client, variant8_t msg);
static marlin_client_t *_client_ptr();

//-----------------------------------------------------------------------------
// client side public functions

void init() {
    int client_id;
    marlin_client_t *client = 0;
    TaskDeps::wait(TaskDeps::Tasks::marlin_client);
    osSemaphoreWait(server_semaphore, osWaitForever);
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++) {
        if (marlin_client_task[client_id] == 0) {
            break;
        }
    }
    if (client_id < MARLIN_MAX_CLIENTS) {
        client = clients + client_id;
        memset(client, 0, sizeof(marlin_client_t));
        osMessageQDef(clientQueue, MARLIN_CLIENT_QUEUE * 2, uint32_t);
        marlin_client_queue[client_id] = osMessageCreate(osMessageQ(clientQueue), NULL);
        client->id = client_id;
        client->flags = 0;
        client->events = 0;
        marlin_clients++;
        client->flags |= (MARLIN_CFLG_STARTED | MARLIN_CFLG_PROCESS);
        client->errors = 0;
        client->command = ftrstd::to_underlying(Cmd::NONE);
        client->reheating = 0;
        client->fsm_cb = NULL;
        client->message_cb = NULL;
        client->warning_cb = NULL;
        client->startup_cb = NULL;
        marlin_client_task[client_id] = osThreadGetId();
    }
    osSemaphoreRelease(server_semaphore);
}

void loop() {
    uint16_t count = 0;
    osEvent ose;
    variant8_t msg;
    variant8_t *pmsg = &msg;
    int client_id;
    marlin_client_t *client;
    osMessageQId queue;
    osThreadId taskHandle = osThreadGetId();
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++) {
        if (taskHandle == marlin_client_task[client_id]) {
            break;
        }
    }
    if (client_id >= MARLIN_MAX_CLIENTS) {
        return;
    }
    client = clients + client_id;
    if ((queue = marlin_client_queue[client_id]) != 0) {
        while ((ose = osMessageGet(queue, 0)).status == osEventMessage) {
            if (client->flags & MARLIN_CFLG_LOWHIGH) {
                msg |= ((variant8_t)ose.value.v << 32); // store high dword
                _process_client_message(client, msg); // call handler
                variant8_done(&pmsg);
                count++;
            } else {
                msg = ose.value.v; // store low dword
            }
            client->flags ^= MARLIN_CFLG_LOWHIGH; // flip flag
        }
    }
    client->last_count = count;
}

int get_id() {
    marlin_client_t *client = _client_ptr();
    if (client) {
        return client->id;
    }
    return 0;
}

void wait_for_start_processing() {
    // wait for startup
    while (!event_clr(Event::StartProcessing)) {
        loop();
        osDelay(100);
    }
}

// register callback to fsm creation
// return success
bool set_fsm_cb(fsm_cb_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->fsm_cb = cb;
        return true;
    }
    return false;
}

// register callback to message
// return success
bool set_message_cb(message_cb_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->message_cb = cb;
        return true;
    }
    return false;
}

// register callback to warning_cb_t (fan failure, heater timeout ...)
// return success
bool set_warning_cb(warning_cb_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->warning_cb = cb;
        return true;
    }
    return false;
}

// register callback to startup_cb_t (complete initialization)
// return success
bool set_startup_cb(startup_cb_t cb) {
    marlin_client_t *client = _client_ptr();
    if (client && cb) {
        client->startup_cb = cb;
        return true;
    }
    return false;
}

bool is_processing() {
    marlin_client_t *client = _client_ptr();
    return client && client->flags & MARLIN_CFLG_PROCESS;
}

void _send_request_to_server_and_wait_with_callback(const char *request, void (*cb)()) {
    marlin_client_t *client = _client_ptr();
    if (client == 0) {
        return;
    }
    uint8_t retries_left = max_retries;

    do {
        _send_request_to_server(client->id, request);
        _wait_ack_from_server_with_callback(client->id, cb);
        if ((client->events & make_mask(Event::NotAcknowledge)) != 0) {
            // clear nack flag
            client->events &= ~make_mask(Event::NotAcknowledge);
            log_warning(MarlinClient, "Request %s to marlin server not acknowledged, retries left %u ", request, retries_left);
            // give marlin server time to process other requests
            osDelay(10);
        }
        retries_left--;
    } while ((client->events & make_mask(Event::Acknowledge)) == 0 && retries_left > 0);

    if (retries_left <= 0) {
        fatal_error(ErrCode::ERR_SYSTEM_MARLIN_CLIENT_SERVER_REQUEST_TIMEOUT);
    }
    // clear the flag of ack events
    client->events &= ~make_mask(Event::Acknowledge);
}

void set_event_notify(uint64_t notify_events, void (*cb)()) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(
        request,
        MARLIN_MAX_REQUEST,
        "!%c%08lx %08lx",
        ftrstd::to_underlying(Msg::EventMask),
        static_cast<uint32_t>(notify_events & 0xffffffff),
        static_cast<uint32_t>(notify_events >> 32));
    _send_request_to_server_and_wait_with_callback(request, cb);
}

marlin_server::Cmd get_command() {
    marlin_client_t *client = _client_ptr();
    if (client) {
        return marlin_server::Cmd(client->command);
    }
    return Cmd::NONE;
}

void _send_request_to_server_and_wait(const char *request) {
    _send_request_to_server_and_wait_with_callback(request, NULL);
}

void _send_request_id_to_server_and_wait(const Msg id) {
    char request[3];
    marlin_msg_to_str(id, request);
    _send_request_to_server_and_wait(request);
}

void gcode(const char *gcode) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%s", ftrstd::to_underlying(Msg::Gcode), gcode);
    _send_request_to_server_and_wait(request);
}

int gcode_printf(const char *format, ...) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c", ftrstd::to_underlying(Msg::Gcode));
    va_list ap;
    va_start(ap, format);
    const int ret = vsnprintf(request + 2, MARLIN_MAX_REQUEST - 3, format, ap);
    va_end(ap);
    _send_request_to_server_and_wait(request);
    return ret;
}

void gcode_push_front(const char *gcode) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c0x%p", ftrstd::to_underlying(Msg::InjectGcode), gcode);
    _send_request_to_server_and_wait(request);
}

int event(Event evt_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << ftrstd::to_underlying(evt_id);
    if (client) {
        ret = (client->events & msk) ? 1 : 0;
    }
    return ret;
}

int event_clr(Event evt_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << ftrstd::to_underlying(evt_id);
    if (client) {
        ret = (client->events & msk) ? 1 : 0;
        client->events &= ~msk;
    }
    return ret;
}

uint64_t events() {
    marlin_client_t *client = _client_ptr();
    return (client) ? client->events : 0;
}

int error(uint8_t err_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << err_id;
    if (client) {
        ret = (client->errors & msk) ? 1 : 0;
    }
    return ret;
}

int error_set(uint8_t err_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << err_id;
    if (client) {
        ret = (client->errors & msk) ? 1 : 0;
        client->errors |= msk;
    }
    return ret;
}

int error_clr(uint8_t err_id) {
    int ret = 0;
    marlin_client_t *client = _client_ptr();
    uint64_t msk = (uint64_t)1 << err_id;
    if (client) {
        ret = (client->errors & msk) ? 1 : 0;
        client->errors &= ~msk;
    }
    return ret;
}

uint64_t errors() {
    marlin_client_t *client = _client_ptr();
    return (client) ? client->errors : 0;
}

void do_babysteps_Z(float offs) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(
        request,
        MARLIN_MAX_REQUEST,
        "!%c%.4f",
        ftrstd::to_underlying(Msg::Babystep),
        static_cast<double>(offs));
    _send_request_to_server_and_wait(request);
}

void move_axis(float logical_pos, float feedrate, uint8_t axis) {
    char request[MARLIN_MAX_REQUEST];
    // check axis
    if (axis <= E_AXIS) {
        snprintf(
            request,
            MARLIN_MAX_REQUEST,
            "!%c%.4f %.4f %u",
            ftrstd::to_underlying(Msg::Move),
            static_cast<double>(LOGICAL_TO_NATIVE(logical_pos, axis)),
            static_cast<double>(feedrate),
            axis);
        _send_request_to_server_and_wait(request);
    }
}

void settings_save() {
    _send_request_id_to_server_and_wait(Msg::ConfigSave);
}

void settings_load() {
    _send_request_id_to_server_and_wait(Msg::ConfigLoad);
}

void settings_reset() {
    _send_request_id_to_server_and_wait(Msg::ConfigReset);
}

#if HAS_SELFTEST()
void test_start_for_tools(const uint64_t test_mask, const uint8_t tool_mask) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(
        request,
        MARLIN_MAX_REQUEST,
        "!%c%08lx %08lx %08lx",
        ftrstd::to_underlying(Msg::TestStart),
        static_cast<uint32_t>(test_mask),
        static_cast<uint32_t>(test_mask >> 32),
        static_cast<uint32_t>(tool_mask));
    _send_request_to_server_and_wait(request);
}

void test_start(const uint64_t test_mask) {
    test_start_for_tools(test_mask, AllTools);
}

void test_abort() {
    _send_request_id_to_server_and_wait(Msg::TestAbort);
}
#endif

void print_start(const char *filename, marlin_server::PreviewSkipIfAble skip_preview) {
    char request[MARLIN_MAX_REQUEST];
    assert(skip_preview < marlin_server::PreviewSkipIfAble::_count);
    static_assert(ftrstd::to_underlying(marlin_server::PreviewSkipIfAble::_count) < ('9' - '0'), "Too many skip preview options.");
    const int len = snprintf(request, sizeof(request), "!%c%c%s", ftrstd::to_underlying(Msg::PrintStart), '0' + ftrstd::to_underlying(skip_preview), filename);
    if (len < 0) {
        bsod("Error formatting request.");
    }
    if ((size_t)len >= sizeof(request)) {
        bsod("Request too long.");
    }
    _send_request_to_server_and_wait(request);
}

bool is_print_started() {
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
        case State::WaitGui:
        // We also need to wait these two out, because they are not considered printing
        // and if connect want to send JOB_INFO before marlin_server goes through them
        // it falsely rejects the print. There should be no chance to get an infinit loop
        // because we only call this function right after calling print_start with skip
        // preview enabled, so it either starts printing or goes into PrintPreviewQuestions.
        case State::PrintPreviewInit:
        case State::PrintPreviewImage:
            // We are still waiting for GUI to make up its mind. Do another round.
            osDelay(10);
            break;
        case State::Idle:
        case State::Aborted:
        case State::Finished:
            // Went to idle - refused by GUI
            return false;
        default:
            // Doing something else ‒ there's a lot of states where we are printing.
            return true;
        }
    }
}

bool is_print_exited() {
    while (true) {
        switch (marlin_vars()->print_state) {
        case State::Finished:
        case State::Aborted:
        case State::Exit:
            // We are still waiting
            osDelay(10);
            break;
        case State::Idle:
            return true;
        default:
            return false;
        }
    }
}

void marlin_gui_ready_to_print() {
    _send_request_id_to_server_and_wait(Msg::PrintReady);
}

void marlin_gui_cant_print() {
    _send_request_id_to_server_and_wait(Msg::GuiCantPrint);
}

void print_abort() {
    _send_request_id_to_server_and_wait(Msg::PrintAbort);
}

void print_exit() {
    _send_request_id_to_server_and_wait(Msg::PrintExit);
}

void print_pause() {
    _send_request_id_to_server_and_wait(Msg::PrintPause);
}

void print_resume() {
    _send_request_id_to_server_and_wait(Msg::PrintResume);
}

void park_head() {
    _send_request_id_to_server_and_wait(Msg::Park);
}

void notify_server_about_encoder_move() {
    _send_request_id_to_server_and_wait(Msg::KnobMove);
}

void notify_server_about_knob_click() {
    _send_request_id_to_server_and_wait(Msg::KnobClick);
}

// returns true if reheating is in progress, otherwise false
bool is_reheating() {
    marlin_client_t *client = _client_ptr();
    return client && client->reheating;
}

//-----------------------------------------------------------------------------
// responses from client finite state machine (like button click)
void encoded_response(uint32_t enc_phase_and_response) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%d", ftrstd::to_underlying(Msg::FSM), (int)enc_phase_and_response);
    _send_request_to_server_and_wait(request);
}
bool is_printing() {
    switch (marlin_vars()->print_state) {
    case State::Aborted:
    case State::Idle:
    case State::Finished:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
    case State::PrintPreviewToolsMapping:
        return false;
    default:
        return true;
    }
}

bool is_idle() {
    switch (marlin_vars()->print_state) {
    case State::Idle:
        return true;
    default:
        return false;
    }
}

//-----------------------------------------------------------------------------
// private functions

// send request to server (called from client thread), infinite timeout
static void _send_request_to_server(uint8_t client_id, const char *request) {
    int ret = 0;
    int len = strlen(request);
    osMessageQId queue = 0;
    int i;
    osSemaphoreWait(server_semaphore, osWaitForever); // lock
    if ((queue = server_queue) != 0) // queue valid
    {
        clients[client_id].events &= ~make_mask(Event::Acknowledge);
        while (ret == 0) {
            if (osMessageAvailableSpace(queue) >= static_cast<uint32_t>(len + 1)) // check available space
            {
                osMessagePut(queue, '0' + client_id, osWaitForever); // one character client id
                for (i = 0; i < len; i++) { // loop over every characters
                    osMessagePut(queue, request[i], osWaitForever); //
                }
                if ((i > 0) && (request[i - 1] != '\n')) { // automatically terminate with '\n'
                    osMessagePut(queue, '\n', osWaitForever);
                }
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
    while ((clients[client_id].events & make_mask(Event::Acknowledge)) == 0 && (clients[client_id].events & make_mask(Event::NotAcknowledge)) == 0) {
        loop();
        if (clients[client_id].last_count == 0) {
            if (cb) {
                cb();
            }
            osDelay(10);
        }
    }
    return clients[client_id].ack;
}

// process message on client side (set flags, update vars etc.)
static void _process_client_message(marlin_client_t *client, variant8_t msg) {
    uint8_t id = variant8_get_usr8(msg) & MARLIN_USR8_MSK_ID;
    if (variant8_get_type(msg) == VARIANT8_USER) // event received
    {
        client->events |= ((uint64_t)1 << id);
        switch ((Event)id) {
        case Event::MeshUpdate: {
            uint8_t _UNUSED x = variant8_get_usr16(msg) & 0xff;
            uint8_t _UNUSED y = variant8_get_usr16(msg) >> 8;
            float _UNUSED z = variant8_get_flt(msg);
            break;
        }
        case Event::StartProcessing:
            client->flags |= MARLIN_CFLG_PROCESS;
            break;
        case Event::StopProcessing:
            client->flags &= ~MARLIN_CFLG_PROCESS;
            break;
        case Event::Error:
            client->errors |= MARLIN_ERR_MSK(variant8_get_ui32(msg));
            break;
        case Event::CommandBegin:
            client->command = variant8_get_ui32(msg);
            break;
        case Event::CommandEnd:
            client->command = ftrstd::to_underlying(Cmd::NONE);
            break;
        case Event::Reheat:
            client->reheating = (uint8_t)variant8_get_ui32(msg);
            break;
        case Event::NotAcknowledge:
        case Event::Acknowledge:
            client->ack = variant8_get_ui32(msg);
            break;
        case Event::FSM:
            if (client->fsm_cb) {
                client->fsm_cb(variant8_get_ui32(msg), variant8_get_usr16(msg));
            }
            break;
        case Event::Message: {
            variant8_t *pvar = &msg;
            variant8_set_type(pvar, VARIANT8_PCHAR);
            const char *str = variant8_get_pch(msg);
            if (client->message_cb) {
                client->message_cb(str);
            }
            variant8_done(&pvar);
            break;
        }
        case Event::Warning:
            if (client->warning_cb) {
                client->warning_cb(static_cast<WarningType>(variant8_get_i32(msg)));
            }
            break;
        case Event::Startup:
            if (client->startup_cb) {
                client->startup_cb();
            }
            break;
            // not handled events
            // do not use default, i want all events listed here, so new event will generate warning, when not added
        case Event::PrinterKilled:
        case Event::MediaInserted:
        case Event::MediaError:
        case Event::MediaRemoved:
        case Event::PlayTone:
        case Event::PrintTimerStarted:
        case Event::PrintTimerPaused:
        case Event::PrintTimerStopped:
        case Event::FilamentRunout:
        case Event::UserConfirmRequired:
        case Event::StatusChanged:
        case Event::FactoryReset:
        case Event::LoadSettings:
        case Event::StoreSettings:
        case Event::SafetyTimerExpired:
            break;
        case Event::_count:
            assert(false);
        }
#ifdef DBG_EVT_MSK
        if (DBG_EVT_MSK & ((uint64_t)1 << id)) {
            switch (id) {
            // Event Event::MeshUpdate - ui32 is float z, ui16 low byte is x index, high byte y index
            case Event::MeshUpdate: {
                uint8_t x = msg.usr16 & 0xff;
                uint8_t y = msg.usr16 >> 8;
                float z = msg.flt;
                DBG_EVT("CL%c: EVT %s %d %d %.3f", '0' + client->id, marlin_events_get_name(id),
                    x, y, static_cast<double>(z));
                x = x;
                y = y;
                z = z; // prevent warning
            } break;
            // Event Event::CommandBegin/End - ui32 is encoded command
            case Event::CommandBegin:
            case Event::CommandEnd:
                DBG_EVT("CL%c: EVT %s %c%u", '0' + client->id, marlin_events_get_name(id),
                    (msg.ui32 >> 16) & 0xff, msg.ui32 & 0xffff);
                break;
            // Event Event::Acknowledge - ui32 is result (not used in this time)
            case Event::Reheat:
            case Event::Acknowledge:
                DBG_EVT("CL%c: EVT %s %lu", '0' + client->id, marlin_events_get_name(id), msg.ui32);
                break;
            // Other events and events without arguments
            default:
                DBG_EVT("CL%c: EVT %s", '0' + client->id, marlin_events_get_name(id));
                break;
            }
        }
#endif // DBG_EVT_MSK
    }
}

// returns client pointer for calling client thread (client thread)
static marlin_client_t *_client_ptr() {
    osThreadId taskHandle = osThreadGetId();
    int client_id;
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++) {
        if (taskHandle == marlin_client_task[client_id]) {
            return clients + client_id;
        }
    }
    return 0;
}

template <typename T>
void marlin_set_variable(MarlinVariable<T> &variable, T value) {
    char request[MARLIN_MAX_REQUEST];

    const int n = snprintf(request, MARLIN_MAX_REQUEST, "!%c%d ", ftrstd::to_underlying(Msg::SetVariable), reinterpret_cast<uintptr_t>(&variable));
    if (n < 0) {
        bsod("Error formatting var name.");
    }
    if (size_t(n) >= sizeof(request)) {
        bsod("Request too long.");
    }

    int v;
    if constexpr (std::is_floating_point<T>::value) {
        v = snprintf(request + n, sizeof(request) - n, "%f", static_cast<double>(value));
    } else if constexpr (std::is_integral<T>::value) {
        v = snprintf(request + n, sizeof(request) - n, "%d", value);
    } else {
        bsod("no conversion");
    }

    if (v < 0) {
        bsod("Error formatting var value.");
    }
    if (((size_t)v + (size_t)n) >= sizeof(request)) {
        bsod("Request too long.");
    }

    _send_request_to_server_and_wait(request);
}

void set_target_nozzle(float val, uint8_t hotend) {
    return marlin_set_variable(marlin_vars()->hotend(hotend).target_nozzle, val);
}
void set_display_nozzle(float val, uint8_t hotend) {
    return marlin_set_variable(marlin_vars()->hotend(hotend).display_nozzle, val);
}
void set_target_bed(float val) {
    return marlin_set_variable(marlin_vars()->target_bed, val);
}
void set_fan_speed(uint8_t val) {
    return marlin_set_variable(marlin_vars()->print_fan_speed, val);
}
void set_print_speed(uint16_t val) {
    return marlin_set_variable(marlin_vars()->print_speed, val);
}
void set_flow_factor(uint16_t val, uint8_t hotend) {
    return marlin_set_variable(marlin_vars()->hotend(hotend).flow_factor, val);
}
void set_z_offset(float val) {
    return marlin_set_variable(marlin_vars()->z_offset, val);
}
void set_fan_check(bool val) {
    return marlin_set_variable(marlin_vars()->fan_check_enabled, static_cast<uint8_t>(val));
}
void set_fs_autoload(bool val) {
    return marlin_set_variable(marlin_vars()->fs_autoload_enabled, static_cast<uint8_t>(val));
}

#if ENABLED(CANCEL_OBJECTS)
void cancel_object(int object_id) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%d", ftrstd::to_underlying(Msg::CancelObjectID), object_id);
    _send_request_to_server_and_wait(request);
}

void uncancel_object(int object_id) {
    char request[MARLIN_MAX_REQUEST];
    snprintf(request, MARLIN_MAX_REQUEST, "!%c%d", ftrstd::to_underlying(Msg::UncancelObjectID), object_id);
    _send_request_to_server_and_wait(request);
}

void cancel_current_object() {
    _send_request_id_to_server_and_wait(Msg::CancelCurrentObject);
}
#endif

} // namespace marlin_client
