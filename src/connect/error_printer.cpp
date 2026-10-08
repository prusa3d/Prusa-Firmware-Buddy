#include "error_printer.hpp"
#include "printer_common.hpp"
#include <config_store/store_instance.hpp>
#include <random/random.h>
#include <common/sys.hpp>

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

    // Do not send error code of BSOD (message should be shown and it does not match error text)
    if (error_code == std::to_underlying(ErrCode::ERR_SYSTEM_INTERNAL_ERROR)) {
        error_code = 0;
    }
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

bool ErrorPrinter::is_valid_file_or_transfer(const char *) const {
    return false;
}

Printer::StartPrintResult ErrorPrinter::start_print(const char *, const std::optional<ToolMapping> &) {
    return std::unexpected("Can't print in error");
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

#if HAS_CANCEL_OBJECT()
void ErrorPrinter::set_object_cancelled(uint16_t, bool) {}
#endif

Printer::Config ErrorPrinter::load_config() {
    return load_eeprom_config();
}

const char *ErrorPrinter::dialog_action(printer_state::DialogId, Response) {
    return "Click not allowed.";
}

std::optional<ErrorPrinter::FinishedJobResult> ErrorPrinter::get_prior_job_result(uint16_t) const {
    return nullopt;
}

void ErrorPrinter::reset_printer() {
    sys_reset();
}

// The error state reports no tools, so no material either
std::optional<FilamentTypeParameters> ErrorPrinter::material(VirtualToolIndex) const {
    return nullopt;
}

void ErrorPrinter::set_physical_tool_info(PhysicalToolIndex, const PhysicalToolInfo &) {
    // Nothing in the error state
}

} // namespace connect_client
