#pragma once

#include <error_codes.hpp>
#include <cstdint>

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
    StateWithDialog(DeviceState state, std::optional<ErrCode> code, std::optional<uint32_t> dialog_id)
        : device_state(state)
        , dialog_id(dialog_id)
        , code(code) {
    }
    static StateWithDialog attention(ErrCode code, uint32_t dialog_id) {
        return StateWithDialog(DeviceState::Attention, code, dialog_id);
    }
};

DeviceState get_state(bool ready = false);
StateWithDialog get_state_with_dialog(bool ready = false);

bool remote_print_ready(bool preview_only);

bool has_job();

const char *to_str(DeviceState state);
} // namespace printer_state
