#include <state/printer_state.hpp>

// The real implementation deals with marlin_vars,
// so we mock it to avoid dealing with it.
namespace printer_state {

DeviceState get_state(bool ready) {
    return DeviceState::Idle;
}

bool remote_print_ready(bool preview_only) {
    return true;
}

bool has_job() {
    return false;
}

const char *to_str(DeviceState state) {
    switch (state) {
    case DeviceState::Idle:
        return "IDLE";
    case DeviceState::Printing:
        return "PRINTING";
    case DeviceState::Paused:
        return "PAUSED";
    case DeviceState::Finished:
        return "FINISHED";
    case DeviceState::Stopped:
        return "STOPPED";
    case DeviceState::Ready:
        return "READY";
    case DeviceState::Error:
        return "ERROR";
    case DeviceState::Busy:
        return "BUSY";
    case DeviceState::Attention:
        return "ATTENTION";
    case DeviceState::Unknown:
    default:
        return "UNKNOWN";
    }
}

} // namespace printer_state
