/**
 * @file screen_menu_user_interface.cpp
 */

#include "screen_menu_user_interface.hpp"
#include "gcode_info.hpp"
#include "DialogMoveZ.hpp"
#include "touch_dependency.hpp"

void ScreenMenuUserInterface::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }
#if PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5
    if (last_touch_error_count != touch::get_touch_read_err_total()) {
        last_touch_error_count = touch::get_touch_read_err_total();
        Item<MI_TOUCH_ERR_COUNT>().SetVal(touch::get_touch_read_err_total());
    }
#endif

    SuperWindowEvent(sender, event, param);
}

ScreenMenuUserInterface::ScreenMenuUserInterface()
    : ScreenMenuUserInterface__(_(label)) {
    EnableLongHoldScreenAction();
}
