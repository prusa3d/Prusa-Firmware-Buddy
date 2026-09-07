/// @file
#include <gui/screen/initial/screen_touch_driver_failed.hpp>

#include <option/has_touch.h>
static_assert(HAS_TOUCH());

#include <hw/touchscreen/touchscreen.hpp>
#include <lang/i18n.h>
#include <window_msgbox.hpp>

bool ScreenTouchDriverFailed::should_show() {
    return touchscreen.is_enabled() && !touchscreen.is_hw_ok();
}

ScreenTouchDriverFailed::ScreenTouchDriverFailed()
    : PseudoScreenCallback {
        [] {
            touchscreen.set_enabled(false);
            MsgBoxWarning(_("Touch driver failed to initialize, touch functionality disabled"), Responses_Ok);
        },
    } {}
