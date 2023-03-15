/**
 * @file screen_touch_error.cpp
 */
#include "screen_touch_error.hpp"
#include "ScreenHandler.hpp"

#include "touch_get.hpp"

ScreenTouchError::ScreenTouchError()
    : AddSuperWindow<screen_t>()
    , header(this, _("TOUCHSCREEN ERROR")) {}

void ScreenTouchError::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event_in_progress)
        return;

    event_in_progress = true;

    touch::disable();
    MsgBoxWarning(_("Touch driver failed to initialize, touch functionality disabled"), Responses_Ok);
    Screens::Access()->Close();
}
