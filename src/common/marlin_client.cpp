#include "marlin_client.hpp"

#include "marlin_client_queue.hpp"
#include "marlin_server_request.hpp"
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
#include "bsod.h"
#include "utility_extensions.hpp"
#include "variant8.h"
#include "tasks.hpp"

#if HAS_SELFTEST()
    #include <selftest_types.hpp>
#endif

using namespace marlin_server;

namespace marlin_client {

LOG_COMPONENT_DEF(MarlinClient, LOG_SEVERITY_INFO);

static constexpr uint8_t max_retries = 5;

// client
typedef struct _marlin_client_t {
    EventMask events; // event mask
    uint64_t errors;

    uint32_t ack; // cached ack value from last Acknowledge event
    uint32_t command; // processed command (G28,G29,M701,M702,M600)
    fsm_cb_t fsm_cb; // to register callback for dialog or screen creation/destruction/change (M876), callback ensures M876 is processed asap, so there is no need for queue
    message_cb_t message_cb; // to register callback message
    uint8_t id; // client id (0..MARLIN_MAX_CLIENTS-1)
} marlin_client_t;

//-----------------------------------------------------------------------------
// variables

osThreadId marlin_client_task[MARLIN_MAX_CLIENTS]; // task handles
ClientQueue marlin_client_queue[MARLIN_MAX_CLIENTS];

marlin_client_t clients[MARLIN_MAX_CLIENTS]; // client structure
uint8_t marlin_clients = 0; // number of connected clients

//-----------------------------------------------------------------------------
// forward declarations of private functions

static uint32_t _wait_ack_from_server(marlin_client_t *client);
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
        client->id = client_id;
        client->events = 0;
        marlin_clients++;
        client->errors = 0;
        client->command = ftrstd::to_underlying(Cmd::NONE);
        client->fsm_cb = NULL;
        client->message_cb = NULL;
        marlin_client_task[client_id] = osThreadGetId();
    }
    osSemaphoreRelease(server_semaphore);
}

static bool receive_and_process_client_message(marlin_client_t *client, TickType_t ticks_to_wait) {
    variant8_t msg;
    variant8_t *pmsg = &msg;
    ClientQueue &queue = marlin_client_queue[client->id];
    if (queue.receive(msg, ticks_to_wait)) {
        _process_client_message(client, msg);
        variant8_done(&pmsg);
        return true;
    } else {
        return false;
    }
}

void loop() {
    if (marlin_client_t *client = _client_ptr()) {
        while (receive_and_process_client_message(client, 0)) {
        }
    }
}

int get_id() {
    marlin_client_t *client = _client_ptr();
    if (client) {
        return client->id;
    }
    return 0;
}

void wait_for_start_processing() {
    if (marlin_client_t *client = _client_ptr()) {
        while (!event_clr(Event::StartProcessing)) {
            receive_and_process_client_message(client, portMAX_DELAY);
        }
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

static void _send_request_to_server_and_wait(Request &request) {
    marlin_client_t *client = _client_ptr();
    if (client == 0) {
        return;
    }
    uint8_t retries_left = max_retries;

    request.client_id = client->id;
    do {
        // Note: no need to lock here, we are the only client who accesses this
        clients[client->id].events &= ~make_mask(Event::Acknowledge);
        server_queue.send(request);
        _wait_ack_from_server(client);
        if ((client->events & make_mask(Event::NotAcknowledge)) != 0) {
            // clear nack flag
            client->events &= ~make_mask(Event::NotAcknowledge);
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

void set_event_notify(uint64_t event_mask) {
    Request request;
    request.type = Request::Type::EventMask;
    request.event_mask = event_mask;
    _send_request_to_server_and_wait(request);
}

marlin_server::Cmd get_command() {
    marlin_client_t *client = _client_ptr();
    if (client) {
        return marlin_server::Cmd(client->command);
    }
    return Cmd::NONE;
}

void _send_request_id_to_server_and_wait(const Request::Type type) {
    Request request;
    request.type = type;
    _send_request_to_server_and_wait(request);
}

void gcode(const char *gcode) {
    Request request;
    request.type = Request::Type::Gcode;
    if (strlcpy(request.gcode, gcode, sizeof(request.gcode)) >= sizeof(request.gcode)) {
        // TODO It would be much better to ensure gcode always points
        //      to some static buffer and only serialize the pointer.
        log_error(MarlinClient, "ignoring truncated gcode");
    } else {
        _send_request_to_server_and_wait(request);
    }
}

void gcode_printf(const char *format, ...) {
    Request request;
    request.type = Request::Type::Gcode;
    va_list ap;
    va_start(ap, format);
    const int ret = vsnprintf(request.gcode, sizeof(request.gcode), format, ap);
    va_end(ap);
    if (ret == -1 || ret >= (int)sizeof(request.gcode)) {
        // TODO It would be much better to remove gcode_printf() altogether
        //      and instead craft individual request types.
        log_error(MarlinClient, "ignoring truncated gcode");
    } else {
        _send_request_to_server_and_wait(request);
    }
}

void gcode_push_front(const char *gcode) {
    Request request;
    request.type = Request::Type::InjectGcode;
    if (strlcpy(request.inject_gcode, gcode, sizeof(request.inject_gcode)) >= sizeof(request.inject_gcode)) {
        // TODO It would be much better to ensure gcode always points
        //      to some static buffer and only serialize the pointer.
        log_error(MarlinClient, "ignoring truncated gcode");
    } else {
        _send_request_to_server_and_wait(request);
    }
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
    Request request;
    request.type = Request::Type::Babystep;
    request.babystep = offs;
    _send_request_to_server_and_wait(request);
}

void move_axis(float logical_pos, float feedrate, uint8_t axis) {
    Request request;
    request.type = Request::Type::Move;
    request.move.position = LOGICAL_TO_NATIVE(logical_pos, axis);
    request.move.feedrate = feedrate;
    request.move.axis = axis;
    _send_request_to_server_and_wait(request);
}

void move_xyz_axes_to(const xyz_float_t &position, float feedrate) {
    Request request;
    request.type = Request::Type::MoveMultiple;
    request.move_multiple.x = LOGICAL_TO_NATIVE(position.x, X_AXIS);
    request.move_multiple.y = LOGICAL_TO_NATIVE(position.y, Y_AXIS);
    request.move_multiple.z = LOGICAL_TO_NATIVE(position.z, Z_AXIS);
    request.move_multiple.feedrate = feedrate;
    _send_request_to_server_and_wait(request);
}

#if HAS_SELFTEST()
void test_start_with_data(const uint64_t test_mask, const ::selftest::TestData test_data) {
    Request request;
    request.type = Request::Type::TestStart;
    request.test_start.test_mask = test_mask;
    request.test_start.test_data_index = test_data.index();
    request.test_start.test_data_data = ::selftest::serialize_test_data_to_int(test_data);
    _send_request_to_server_and_wait(request);
}

void test_start(const uint64_t test_mask) {
    test_start_with_data(test_mask, ::selftest::TestData {});
}

void test_abort() {
    _send_request_id_to_server_and_wait(Request::Type::TestAbort);
}
#endif

void print_start(const char *filename, marlin_server::PreviewSkipIfAble skip_preview) {
    Request request;
    request.type = Request::Type::PrintStart;
    request.print_start.skip_preview = skip_preview;
    if (strlcpy(request.print_start.filename, filename, sizeof(request.print_start.filename)) >= sizeof(request.print_start.filename)) {
        log_error(MarlinClient, "ignoring truncated filename");
    } else {
        _send_request_to_server_and_wait(request);
    }
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
    _send_request_id_to_server_and_wait(Request::Type::PrintReady);
}

void marlin_gui_cant_print() {
    _send_request_id_to_server_and_wait(Request::Type::GuiCantPrint);
}

void print_abort() {
    _send_request_id_to_server_and_wait(Request::Type::PrintAbort);
}

void print_exit() {
    _send_request_id_to_server_and_wait(Request::Type::PrintExit);
}

void print_pause() {
    _send_request_id_to_server_and_wait(Request::Type::PrintPause);
}

void print_resume() {
    _send_request_id_to_server_and_wait(Request::Type::PrintResume);
}

void park_head() {
    _send_request_id_to_server_and_wait(Request::Type::Park);
}

void notify_server_about_encoder_move() {
    _send_request_id_to_server_and_wait(Request::Type::KnobMove);
}

void notify_server_about_knob_click() {
    _send_request_id_to_server_and_wait(Request::Type::KnobClick);
}

//-----------------------------------------------------------------------------
// responses from client finite state machine (like button click)
void encoded_response(uint32_t enc_phase_and_response) {
    Request request;
    request.type = Request::Type::FSM;
    request.fsm = enc_phase_and_response;
    _send_request_to_server_and_wait(request);
}
bool is_printing() {
    switch (marlin_vars()->print_state) {
    case State::Aborted:
    case State::Idle:
    case State::Finished:
    case State::PrintPreviewInit:
    case State::PrintPreviewImage:
#if HAS_TOOLCHANGER() || HAS_MMU2()
    case State::PrintPreviewToolsMapping:
#endif
        return false;
    default:
        return true;
    }
}

bool is_paused() {
    switch (marlin_vars()->print_state) {
    case State::Paused:
        return true;
    default:
        return false;
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

// wait for ack event, blocking - used for synchronization, called typically at end of client request functions
static uint32_t _wait_ack_from_server(marlin_client_t *client) {
    while ((client->events & make_mask(Event::Acknowledge)) == 0 && (client->events & make_mask(Event::NotAcknowledge)) == 0) {
        receive_and_process_client_message(client, portMAX_DELAY);
    }
    return client->ack;
}

// process message on client side (set flags, update vars etc.)
static void _process_client_message(marlin_client_t *client, variant8_t msg) {
    uint8_t id = variant8_get_usr8(msg) & MARLIN_USR8_MSK_ID;
    if (variant8_get_type(msg) == VARIANT8_USER) // event received
    {
        client->events |= ((uint64_t)1 << id);
        switch ((Event)id) {
        case Event::Error:
            client->errors |= MARLIN_ERR_MSK(variant8_get_ui32(msg));
            break;
        case Event::CommandBegin:
            client->command = variant8_get_ui32(msg);
            break;
        case Event::CommandEnd:
            client->command = ftrstd::to_underlying(Cmd::NONE);
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
            // not handled events
            // do not use default, i want all events listed here, so new event will generate warning, when not added
        case Event::MeshUpdate:
        case Event::Startup:
        case Event::StartProcessing:
        case Event::StopProcessing:
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
            break;
        case Event::_count:
            assert(false);
        }
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
    Request request;
    request.type = Request::Type::SetVariable;
    request.set_variable.variable = reinterpret_cast<uintptr_t>(&variable);

    if constexpr (std::is_floating_point<T>::value) {
        request.set_variable.float_value = static_cast<float>(value);
    } else if constexpr (std::is_integral<T>::value) {
        request.set_variable.uint32_value = static_cast<uint32_t>(value);
    } else {
        bsod("no conversion");
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
    Request request;
    request.type = Request::Type::CancelObjectID;
    request.cancel_object_id = object_id;
    _send_request_to_server_and_wait(request);
}

void uncancel_object(int object_id) {
    Request request;
    request.type = Request::Type::UncancelObjectID;
    request.uncancel_object_id = object_id;
    _send_request_to_server_and_wait(request);
}

void cancel_current_object() {
    _send_request_id_to_server_and_wait(Request::Type::CancelCurrentObject);
}
#endif

} // namespace marlin_client
