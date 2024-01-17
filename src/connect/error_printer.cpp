#include "error_printer.hpp"
#include "printer_common.hpp"

using printer_state::DeviceState;
using printer_state::StateWithDialog;
using std::make_tuple;
using std::nullopt;
using std::optional;
using std::tuple;

namespace connect_client {

ErrorPrinter::ErrorPrinter() {
    init_info(info);
    char msg[crash_dump::MSG_MAX_LEN + 1];
    char title[crash_dump::MSG_TITLE_MAX_LEN + 1];
    if (crash_dump::message_is_valid() && crash_dump::load_message(msg, sizeof msg, title, sizeof title)) {
        snprintf(message, sizeof message, "%s: %s", msg, title);
    } else {
        memset(message, 0, sizeof message);
    }
    error_code = crash_dump::load_message_error_code();
}

void ErrorPrinter::renew(std::optional<SharedBuffer::Borrow>) {}

void ErrorPrinter::drop_paths() {}

Printer::Params ErrorPrinter::params() const {
    Params params(nullopt);

    auto [err_txt, err_code] = err_details();

    optional<ErrCode> code = nullopt;
    if (err_code != 0) {
        code = static_cast<ErrCode>(err_code);
    }
    params.state = StateWithDialog { DeviceState::Error, code };
    // Version can change between MK4 and MK3.9 in runtime
    params.version = get_printer_version();

    return params;
}

optional<Printer::NetInfo> ErrorPrinter::net_info(Printer::Iface) const {
    // Not true, we are likely connected somehow (or hope to be), but we just
    // don't provide info in error state for simplicity.
    return nullopt;
}

Printer::NetCreds ErrorPrinter::net_creds() const {
    NetCreds creds = { "", "" };

    return creds;
}

bool ErrorPrinter::job_control(JobControl) {
    return false;
}

bool ErrorPrinter::start_print(const char *) {
    return false;
}

const char *ErrorPrinter::delete_file(const char *) {
    return "In error state, not doing anything";
}

void ErrorPrinter::submit_gcode(const char *) {}

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
    config_store().connect_token.set(token);
    config_store().connect_enabled.set(true);
}

uint32_t ErrorPrinter::cancelable_fingerprint() const {
    return 0;
}

#if ENABLED(CANCEL_OBJECTS)
const char *ErrorPrinter::get_cancel_object_name(char *buffer, [[maybe_unused]] size_t size, size_t) const {
    assert(size > 0);
    *buffer = '\0';
    return buffer;
}
#endif

Printer::Config ErrorPrinter::load_config() {
    return load_eeprom_config();
}

tuple<const char *, uint16_t> ErrorPrinter::err_details() const {
    return make_tuple(message, error_code);
}

void ErrorPrinter::reset_printer() {
    NVIC_SystemReset();
}

} // namespace connect_client
