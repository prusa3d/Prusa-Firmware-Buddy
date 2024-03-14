/**
 * @file screen_menu_sensor_info_mini.cpp
 */

#include "screen_menu_sensor_info.hpp"
#include "DialogMoveZ.hpp"

void ScreenMenuSensorInfo::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {

    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
