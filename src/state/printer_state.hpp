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

struct Dialog {
    uint32_t dialog_id = 0;
    std::optional<ErrCode> code = std::nullopt;
    const char *title = nullptr;
    const char *text = nullptr;
};

struct StateWithDialog {
    DeviceState device_state;
    std::optional<Dialog> dialog = std::nullopt;
    StateWithDialog(DeviceState state)
        : device_state(state) {}
    StateWithDialog(DeviceState state, std::optional<ErrCode> code, std::optional<uint32_t> dialog_id)
        : device_state(state) {
        if (dialog_id.has_value()) {
            dialog = Dialog {
                *dialog_id,
                code,
            };
        }
    }
    static StateWithDialog attention(ErrCode code, uint32_t dialog_id) {
        return StateWithDialog(DeviceState::Attention, code, dialog_id);
    }
    // If there's a dialog with code
    bool has_code() const {
        return dialog.has_value() && dialog->code.has_value();
    }
    // The numeric value of the dialog's code if present, 0 otherwise.
    uint32_t code_num() const {
        if (has_code()) {
            return static_cast<uint32_t>(*dialog->code);
        } else {
            return 0;
        }
    }

    const char *title() const {
        return dialog.has_value() ? dialog->title : nullptr;
    }

    const char *text() const {
        return dialog.has_value() ? dialog->text : nullptr;
    }
};

DeviceState get_state(bool ready = false);
StateWithDialog get_state_with_dialog(bool ready = false);

bool remote_print_ready(bool preview_only);

bool has_job();

const char *to_str(DeviceState state);
} // namespace printer_state
