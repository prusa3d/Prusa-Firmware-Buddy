#pragma once

#include <error_codes.hpp>
#include <error_code_mangle.hpp>
#include <cstdint>

#include <optional>

#include <common/marlin_server_types/client_fsm_types.h>
#include <common/marlin_server_types/marlin_server_state.h>

enum class Response : uint8_t;

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
    // Buttons for the given dialog (if any, may be nullptr)
    //
    // It might look "wrong" to have a dialog without any buttons, but we know of two cases:
    // * Not-implemented part where we don't yet provide the buttons (that should improve over time).
    // * Redscreens/bluescreens are considered a "dialog" towards Connect, but don't have buttons.
    //
    // This is "pointer to array"; we don't use PhaseResponses, because it's
    // impossible to forward-declare. But it's a std::array<Response>, so we
    // can point to that without introducing another (fat) header dependency.
    // There's at most MAX_RESPONSES responses.
    const Response *buttons = nullptr;
};

struct StateWithDialog {
    DeviceState device_state;
    std::optional<Dialog> dialog = std::nullopt;
    StateWithDialog(DeviceState state)
        : device_state(state) {}
    StateWithDialog(DeviceState state, std::optional<ErrCode> code, std::optional<uint32_t> dialog_id, const Response *buttons = nullptr)
        : device_state(state) {
        if (dialog_id.has_value()) {
            dialog = Dialog {
                *dialog_id,
                code,
            };
            dialog->buttons = buttons;
        }
    }
    static StateWithDialog attention(ErrCode code, uint32_t dialog_id, const Response *buttons = nullptr) {
        return StateWithDialog(DeviceState::Attention, code, dialog_id, buttons);
    }
    // If there's a dialog with code
    bool has_code() const {
        return dialog.has_value() && dialog->code.has_value();
    }
    // The numeric value of the dialog's code if present, 0 otherwise.
    uint32_t code_num() const {
        return has_code() ? map_error_code(*dialog->code) : 0;
    }

    const char *title() const {
        return dialog.has_value() ? dialog->title : nullptr;
    }

    const char *text() const {
        return dialog.has_value() ? dialog->text : nullptr;
    }

    const Response *buttons() const {
        return dialog.has_value() ? dialog->buttons : nullptr;
    }
};

ErrCode warning_type_to_error_code(WarningType wtype);

DeviceState get_state(bool ready = false);
DeviceState get_print_state(marlin_server::State state, bool ready);
StateWithDialog get_state_with_dialog(bool ready = false);

bool remote_print_ready(bool preview_only);

bool has_job();

const char *to_str(DeviceState state);
} // namespace printer_state
