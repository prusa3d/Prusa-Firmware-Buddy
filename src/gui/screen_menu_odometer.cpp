// screen_menu_odometer.cpp

#include <stdlib.h>

#include "cmath_ext.h"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "i18n.h"
#include "odometer.hpp"
#include "MItem_tools.hpp"

using MenuContainer = WinMenuContainer<MI_RETURN>;

using OdometerScreen = ScreenMenu<EFooter::On, MI_RETURN, MI_ODOMETER_DIST_X, MI_ODOMETER_DIST_Y, MI_ODOMETER_DIST_Z, MI_ODOMETER_DIST_E, MI_ODOMETER_TIME>;

class ScreenOdometer : public OdometerScreen {
    static constexpr const char *label = N_("ODOMETER");

    static float getVal(Odometer_s::axis_t axis) {
        Odometer_s::instance().force_to_eeprom();
        return Odometer_s::instance().get(axis) * .001f;
    }
    static uint32_t getTime() {
        Odometer_s::instance().force_to_eeprom();
        return Odometer_s::instance().get_time();
    }

public:
    ScreenOdometer()
        : OdometerScreen(_(label)) {
        Odometer_s::instance().force_to_eeprom();
        Item<1>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::X));
        Item<2>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::Y));
        Item<3>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::Z));
        Item<4>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::E));
        Item<5>().UpdateValue(Odometer_s::instance().get_time());
    }
};

ScreenFactory::UniquePtr GetScreenMenuOdometer() {
    return ScreenFactory::Screen<ScreenOdometer>();
}
