/**
 * @file screen_menu_odometer.cpp
 */

#include "screen_menu_odometer.hpp"
#include "screen_menu.hpp"
#include "odometer.hpp"
#include "MItem_tools.hpp"
#include "screen_move_z.hpp"
#include <option/has_toolchanger.h>
#include <option/has_mmu2.h>

void ScreenMenuOdometer::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        open_move_z_screen();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}

ScreenMenuOdometer::ScreenMenuOdometer()
    : ScreenMenuOdometer__(_(label)) {
    EnableLongHoldScreenAction();
    Odometer_s::instance().force_to_eeprom();
    Item<MI_ODOMETER_DIST_X>().UpdateValue(Odometer_s::instance().get_axis(Odometer_s::axis_t::X));
    Item<MI_ODOMETER_DIST_Y>().UpdateValue(Odometer_s::instance().get_axis(Odometer_s::axis_t::Y));
    Item<MI_ODOMETER_DIST_Z>().UpdateValue(Odometer_s::instance().get_axis(Odometer_s::axis_t::Z));
    Item<MI_ODOMETER_DIST_E>().UpdateValue(Odometer_s::instance().get_extruded_all());
#if HAS_TOOLCHANGER()
    Item<MI_ODOMETER_DIST_E_N<0>>().UpdateValue(Odometer_s::instance().get_extruded(0));
    Item<MI_ODOMETER_DIST_E_N<1>>().UpdateValue(Odometer_s::instance().get_extruded(1));
    Item<MI_ODOMETER_DIST_E_N<2>>().UpdateValue(Odometer_s::instance().get_extruded(2));
    Item<MI_ODOMETER_DIST_E_N<3>>().UpdateValue(Odometer_s::instance().get_extruded(3));
    Item<MI_ODOMETER_DIST_E_N<4>>().UpdateValue(Odometer_s::instance().get_extruded(4));
    Item<MI_ODOMETER_TOOL>().UpdateValue(Odometer_s::instance().get_toolpick_all());
    Item<MI_ODOMETER_TOOL_N<0>>().UpdateValue(Odometer_s::instance().get_toolpick(0));
    Item<MI_ODOMETER_TOOL_N<1>>().UpdateValue(Odometer_s::instance().get_toolpick(1));
    Item<MI_ODOMETER_TOOL_N<2>>().UpdateValue(Odometer_s::instance().get_toolpick(2));
    Item<MI_ODOMETER_TOOL_N<3>>().UpdateValue(Odometer_s::instance().get_toolpick(3));
    Item<MI_ODOMETER_TOOL_N<4>>().UpdateValue(Odometer_s::instance().get_toolpick(4));
#endif
#if HAS_MMU2()
    Item<MI_ODOMETER_MMU_CHANGES>().UpdateValue(Odometer_s::instance().get_mmu_changes());
#endif
    Item<MI_ODOMETER_TIME>().UpdateValue(Odometer_s::instance().get_time());
}
