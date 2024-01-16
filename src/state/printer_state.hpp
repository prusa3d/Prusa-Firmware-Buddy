#pragma once

#include <error_codes.hpp>

#include <tuple>
#include <optional>
#include <inttypes.h>

namespace printer_state {

enum class DeviceState {
    Unknown,
    Idle,
    Printing,
    Paused,
    Finished,
    Stopped,
    Ready,
    Busy,
    Attention,
    Error,
};

struct StateWithDialog {
    DeviceState device_state;
    // Due to the fact that we decided to "reuse" the Prusa-Error-Codes to
    // denote displayed dialogs to Connect, even though they are not errors,
    // the naming of the type vs the variable feels a bit off.
    std::optional<ErrCode> dialog_id = std::nullopt;
};

DeviceState get_state(bool ready = false);
StateWithDialog get_state_with_dialog(bool ready = false);

bool remote_print_ready(bool preview_only);

bool has_job();

const char *to_str(DeviceState state);
} // namespace printer_state
