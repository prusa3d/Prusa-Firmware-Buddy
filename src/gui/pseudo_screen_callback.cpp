#include "pseudo_screen_callback.hpp"

#include "ScreenHandler.hpp"
#include <hw/touchscreen/touchscreen.hpp>

PseudoScreenCallback::PseudoScreenCallback(Callback callback)
    : screen_t()
    , header(this, {})
    , callback(callback) {}

void PseudoScreenCallback::windowEvent([[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, [[maybe_unused]] void *param) {
    if (callback_called) {
        return;
    }

    callback_called = true;
    callback();
    Screens::Access()->Close();
}
