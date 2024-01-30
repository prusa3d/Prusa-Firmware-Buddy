#pragma once

#include <error_codes.hpp>

#include <optional>

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
    std::optional<uint32_t> dialog_id = std::nullopt;
    std::optional<ErrCode> code = std::nullopt;
    const char *title = nullptr;
    const char *text = nullptr;
    StateWithDialog(DeviceState state)
        : device_state(state) {}
    StateWithDialog(DeviceState state, std::optional<ErrCode> code)
        : device_state(state)
        // TODO: For now, we cheat. We reuse the numerical value of the error
        // code as the dialog ID too, for simplicity. This can't make a
        // distinction between one error shown twice, but until we implement
        // the control of dialogs remotely, it doesn't matter. Then we'll have
        // to somehow generate unique IDs for each shown instance so we can
        // check the remote touches the right buttons, etc.
        , code(code) {
        if (code.has_value()) {
            dialog_id = static_cast<uint32_t>(*code);
        }
    }
    static StateWithDialog attention(ErrCode code) {
        return StateWithDialog(DeviceState::Attention, code);
    }
};

DeviceState get_state(bool ready = false);
StateWithDialog get_state_with_dialog(bool ready = false);

bool remote_print_ready(bool preview_only);

bool has_job();

const char *to_str(DeviceState state);
} // namespace printer_state
