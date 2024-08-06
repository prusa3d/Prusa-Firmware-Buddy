/**
 * @file screen_menu_sensor_info_mini.cpp
 */

#include "screen_menu_sensor_info.hpp"
#include "DialogMoveZ.hpp"

void ScreenMenuSensorInfo::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        Item<MI_INFO_MCU_TEMP>().UpdateValue(sensor_data().MCUTemp);
    }

    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}
