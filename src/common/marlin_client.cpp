#include "marlin_client.hpp"

#include "marlin_client_queue.hpp"
#include "marlin_server_request.hpp"
#include "marlin_events.h"
#include "marlin_server.hpp"
#include <cassert>
#include <freertos/mutex.hpp>
#include <stdio.h>
#include <string.h>
#include <cstdint>
#include "config.h"
#include "bsod.h"
#include <buddy/ffconf.h>
#include <logging/log.hpp>
#include "../lib/Marlin/Marlin/src/core/macros.h"
#include <module/motion.h>
#include "bsod.h"
#include "utility_extensions.hpp"
#include "tasks.hpp"

#if HAS_SELFTEST()
    #include <selftest_types.hpp>
    #include "printer_selftest.hpp"
#endif

using namespace marlin_server;
using std::nullopt;
using std::optional;

namespace marlin_client {

LOG_COMPONENT_DEF(MarlinClient, logging::Severity::info);

static constexpr uint8_t max_retries = 5;

// client
typedef struct _marlin_client_t {
    EventMask events; // event mask

    uint32_t ack; // cached ack value from last Acknowledge event
    uint32_t command; // processed command (G28,G29,M701,M702,M600)
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

static bool receive_and_process_client_message(marlin_client_t *client, size_t milliseconds_to_wait);
static marlin_client_t *_client_ptr();

//-----------------------------------------------------------------------------
// client side public functions

static freertos::Mutex mutex;

void init_maybe() {
    if (!_client_ptr()) {
        init();
    }
}

void init() {
    // If the marlin has already been initialized, don't call init again
    assert(!_client_ptr());

    int client_id;
    marlin_client_t *client = 0;
    TaskDeps::wait(TaskDeps::Tasks::marlin_client);
    std::unique_lock lock { mutex };
    for (client_id = 0; client_id < MARLIN_MAX_CLIENTS; client_id++) {
        if (marlin_client_task[client_id] == 0) {
            break;
        }
    }
    assert(client_id < MARLIN_MAX_CLIENTS);
    if (client_id < MARLIN_MAX_CLIENTS) {
        client = clients + client_id;
        memset(client, 0, sizeof(marlin_client_t));
        client->id = client_id;
        client->events = 0;
        marlin_clients++;
        client->command = ftrstd::to_underlying(Cmd::NONE);
        client->message_cb = NULL;
        marlin_client_task[client_id] = osThreadGetId();
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

static bool try_send(Request &request) {
    marlin_client_t *client = _client_ptr();
    if (client == nullptr) {
        bsod("Marlin client used before init");
    }
    request.client_id = client->id;
    request.response_required = 1;

    client->events &= ~(make_mask(Event::Acknowledge) | make_mask(Event::NotAcknowledge));
    server_queue.send(request);
    for (;;) {
        receive_and_process_client_message(client, 1000);
        if (client->events & make_mask(Event::Acknowledge)) {
            client->events &= ~make_mask(Event::Acknowledge);
            return true;
        }
        if (client->events & make_mask(Event::NotAcknowledge)) {
            client->events &= ~make_mask(Event::NotAcknowledge);
            return false;
        }
    }
}

static void _send_request_to_server_and_wait(Request &request) {
    marlin_client_t *client = _client_ptr();
    if (client == nullptr) {
        return;
    }
    uint8_t retries_left = max_retries;
    do {
        if (try_send(request)) {
            return;
        } else {
            // give marlin server time to process other requests
            osDelay(10);
            retries_left--;
        }
    } while (retries_left > 0);
    fatal_error(ErrCode::ERR_SYSTEM_MARLIN_CLIENT_SERVER_REQUEST_TIMEOUT);
}

/// send the request to the marlin server and don't ask for acknowledgement
static void _send_request_to_server_noreply(Request &request) {
    marlin_client_t *client = _client_ptr();
    if (client == nullptr) {
        return;
    }
    request.client_id = client->id;
    request.response_required = 0;
    server_queue.send(request);
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

namespace {

    optional<Request> gcode_request(const char *gcode) {
        Request request;
        request.type = Request::Type::Gcode;
        if (strlcpy(request.gcode, gcode, sizeof(request.gcode)) >= sizeof(request.gcode)) {
            // TODO It would be much better to ensure gcode always points
            //      to some static buffer and only serialize the pointer.
            log_error(MarlinClient, "ignoring truncated gcode");
            return nullopt;
        } else {
            return request;
        }
    }

} // namespace

void gcode(const char *gcode) {
    if (auto request = gcode_request(gcode); request.has_value()) {
        _send_request_to_server_and_wait(*request);
    }
}

GcodeTryResult gcode_try(const char *gcode) {
    if (auto request = gcode_request(gcode); request.has_value()) {
        if (try_send(*request)) {
            return GcodeTryResult::Submitted;
        } else {
            return GcodeTryResult::QueueFull;
        }
    } else {
        return GcodeTryResult::GcodeTooLong;
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

void inject(InjectQueueRecord record) {
    Request request;
    request.type = Request::Type::Inject;
    request.inject = record;
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

void do_babysteps_Z(float offs) {
    Request request;
    request.type = Request::Type::Babystep;
    request.babystep = offs;
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
        switch (marlin_vars().print_state) {
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
        switch (marlin_vars().print_state) {
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

void try_recover_from_media_error() {
    _send_request_id_to_server_and_wait(Request::Type::TryRecoverFromMediaError);
}

void notify_server_about_encoder_move() {
    _send_request_id_to_server_and_wait(Request::Type::KnobMove);
}

void notify_server_about_knob_click() {
    _send_request_id_to_server_and_wait(Request::Type::KnobClick);
}

void set_warning(WarningType type) {
    Request request;
    request.type = Request::Type::SetWarning;
    request.warning_type = type;
    _send_request_to_server_noreply(request);
}

//-----------------------------------------------------------------------------
// responses from client finite state machine (like button click)
void FSM_encoded_response(EncodedFSMResponse encoded_fsm_response) {
    Request request;
    request.type = Request::Type::FSM;
    request.encoded_fsm_response = encoded_fsm_response;
    _send_request_to_server_and_wait(request);
}
bool is_printing() {
    switch (marlin_vars().print_state) {
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
    switch (marlin_vars().print_state) {
    case State::Paused:
        return true;
    default:
        return false;
    }
}

bool is_idle() {
    switch (marlin_vars().print_state) {
    case State::Idle:
        return true;
    default:
        return false;
    }
}

//-----------------------------------------------------------------------------
// private functions

// process message on client side (set flags, update vars etc.)
static bool receive_and_process_client_message(marlin_client_t *client, size_t milliseconds_to_wait) {
    ClientEvent client_event;
    ClientQueue &queue = marlin_client_queue[client->id];
    if (!queue.try_receive(client_event, milliseconds_to_wait)) {
        return false;
    }

    client->events |= make_mask(client_event.event);
    switch (client_event.event) {
    case Event::CommandBegin:
        client->command = client_event.usr32;
        break;
    case Event::CommandEnd:
        client->command = ftrstd::to_underlying(Cmd::NONE);
        break;
    case Event::NotAcknowledge:
    case Event::Acknowledge:
        client->ack = client_event.usr32;
        break;
    case Event::Message: {
        if (client->message_cb) {
            client->message_cb(client_event.message); // callback takes ownership
        } else {
            free(client_event.message);
        }
        break;
    }
        // not handled events
        // do not use default, i want all events listed here, so new event will generate warning, when not added
    case Event::MeshUpdate:
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
    return true;
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
    return marlin_set_variable(marlin_vars().hotend(hotend).target_nozzle, val);
}
void set_display_nozzle(float val, uint8_t hotend) {
    return marlin_set_variable(marlin_vars().hotend(hotend).display_nozzle, val);
}
void set_target_bed(float val) {
    return marlin_set_variable(marlin_vars().target_bed, val);
}
void set_fan_speed(uint8_t val) {
    return marlin_set_variable(marlin_vars().print_fan_speed, val);
}
void set_print_speed(uint16_t val) {
    return marlin_set_variable(marlin_vars().print_speed, val);
}
void set_flow_factor(uint16_t val, uint8_t hotend) {
    return marlin_set_variable(marlin_vars().hotend(hotend).flow_factor, val);
}
void set_z_offset(float val) {
    return marlin_set_variable(marlin_vars().z_offset, std::clamp(val, Z_OFFSET_MIN, Z_OFFSET_MAX));
}
void set_fan_check(bool val) {
    return marlin_set_variable(marlin_vars().fan_check_enabled, static_cast<uint8_t>(val));
}
void set_fs_autoload(bool val) {
    return marlin_set_variable(marlin_vars().fs_autoload_enabled, static_cast<uint8_t>(val));
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
