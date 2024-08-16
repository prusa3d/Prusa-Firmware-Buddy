#include "error_printer.hpp"
#include "printer_common.hpp"
#include <config_store/store_instance.hpp>
#include <common/random.h>

using printer_state::DeviceState;
using printer_state::Dialog;
using printer_state::StateWithDialog;
using std::make_tuple;
using std::nullopt;
using std::optional;
using std::tuple;

namespace connect_client {

ErrorPrinter::ErrorPrinter()
    // Random at start to avoid collisions in case of several redscreens in a row.
    : dialog_id(rand_u()) {
    init_info(info);
    if (!crash_dump::message_is_valid() || !crash_dump::load_message(text, sizeof text, title, sizeof title)) {
        title[0] = '\0';
        text[0] = '\0';
    }
    error_code = crash_dump::load_message_error_code();
}

void ErrorPrinter::renew(std::optional<SharedBuffer::Borrow>) {}

void ErrorPrinter::drop_paths() {}

Printer::Params ErrorPrinter::params() const {
    Params params(nullopt);

    if (error_code == 0) {
        params.state = StateWithDialog(DeviceState::Error);
        // Set dialog ID, but keep code empty
        params.state.dialog = Dialog {
            dialog_id,
        };
    } else {
        params.state = StateWithDialog(DeviceState::Error, static_cast<ErrCode>(error_code), dialog_id);
    }

    if (title[0]) {
        params.state.dialog->title = title;
    }

    if (text[0]) {
        params.state.dialog->text = text;
    }

    // Version can change between MK4 and MK3.9 in runtime
    params.version = get_printer_version();

    // No tools available in this mode.
    params.slot_mask = 0;

    return params;
}

optional<Printer::NetInfo> ErrorPrinter::net_info(Printer::Iface) const {
    // Not true, we are likely connected somehow (or hope to be), but we just
    // don't provide info in error state for simplicity.
    return nullopt;
}

Printer::NetCreds ErrorPrinter::net_creds() const {
    NetCreds creds = { "", "", "" };

    return creds;
}

bool ErrorPrinter::job_control(JobControl) {
    return false;
}

const char *ErrorPrinter::start_print(const char *, const std::optional<ToolMapping> &) {
    return "Can't print in error";
}

const char *ErrorPrinter::delete_file(const char *) {
    return "In error state, not doing anything";
}

Printer::GcodeResult ErrorPrinter::submit_gcode(const char *) {
    return GcodeResult::Failed;
}

bool ErrorPrinter::set_ready(bool) {
    return false;
}

bool ErrorPrinter::is_printing() const {
    return false;
}

bool ErrorPrinter::is_in_error() const {
    return true;
}

bool ErrorPrinter::is_idle() const {
    return false;
}

void ErrorPrinter::init_connect(const char *token) {
    // Used from the SET_TOKEN command, so we make it work even in error state.
    auto &store = config_store();
    auto transaction = store.get_backend().transaction_guard();
    store.connect_token.set(token);
    store.connect_enabled.set(true);
}

uint32_t ErrorPrinter::cancelable_fingerprint() const {
    return 0;
}

#if ENABLED(CANCEL_OBJECTS)
void ErrorPrinter::cancel_object(uint8_t) {}
void ErrorPrinter::uncancel_object(uint8_t) {}

const char *ErrorPrinter::get_cancel_object_name(char *buffer, [[maybe_unused]] size_t size, size_t) const {
    assert(size > 0);
    *buffer = '\0';
    return buffer;
}
#endif

Printer::Config ErrorPrinter::load_config() {
    return load_eeprom_config();
}

const char *ErrorPrinter::dialog_action(uint32_t, Response) {
    return "Click not allowed.";
}

std::optional<ErrorPrinter::FinishedJobResult> ErrorPrinter::get_prior_job_result(uint16_t) const {
    return nullopt;
}

void ErrorPrinter::reset_printer() {
    NVIC_SystemReset();
}

void ErrorPrinter::set_slot_info(size_t, const SlotInfo &) {
    // Nothing in the error state
}

} // namespace connect_client
