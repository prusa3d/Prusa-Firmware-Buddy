/**
 * @file screen_menu_odometer.cpp
 */

#include "screen_menu_odometer.hpp"
#include "screen_menu.hpp"
#include "odometer.hpp"
#include "MItem_tools.hpp"
#include "DialogMoveZ.hpp"

void ScreenMenuOdometer::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

ScreenMenuOdometer::ScreenMenuOdometer()
    : ScreenMenuOdometer__(_(label)) {
    EnableLongHoldScreenAction();
    Odometer_s::instance().force_to_eeprom();
    Item<MI_ODOMETER_DIST_X>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::X));
    Item<MI_ODOMETER_DIST_Y>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::Y));
    Item<MI_ODOMETER_DIST_Z>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::Z));
    Item<MI_ODOMETER_DIST_E>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::E));
    Item<MI_ODOMETER_TIME>().UpdateValue(Odometer_s::instance().get_time());
}
