/**
 * @file screen_menu_sensor_info.cpp
 */

#include "screen_menu_sensor_info.hpp"
#include "ScreenHandler.hpp"
#include "MItem_tools.hpp"
#include "DialogMoveZ.hpp"

ScreenMenuSensorInfo::ScreenMenuSensorInfo()
    : ScreenMenuSensorInfo__(_(label)) {
    EnableLongHoldScreenAction();
    ClrMenuTimeoutClose();
}

void ScreenMenuSensorInfo::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
