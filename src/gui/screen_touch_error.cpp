/**
 * @file screen_touch_error.cpp
 */
#include "screen_touch_error.hpp"
#include "ScreenHandler.hpp"

#include "touch_get.hpp"

ScreenTouchError::ScreenTouchError()
    : AddSuperWindow<screen_t>()
    , header(this, _("TOUCHSCREEN ERROR")) {}

void ScreenTouchError::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, [[maybe_unused]] void *param) {
    if (event_in_progress) {
        return;
    }

    event_in_progress = true;

    touch::disable();
    MsgBoxWarning(_("Touch driver failed to initialize, touch functionality disabled"), Responses_Ok);
    Screens::Access()->Close();
}
