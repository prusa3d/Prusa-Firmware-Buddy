// screen_menu_odometer.cpp

#include <stdlib.h>

#include "cmath_ext.h"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "i18n.h"
#include "odometer.hpp"
#include "MItem_tools.hpp"
#include "DialogMoveZ.hpp"

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

    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
        if (event == GUI_event_t::HELD_RELEASED) {
            DialogMoveZ::Show();
            return;
        }

        SuperWindowEvent(sender, event, param);
    }

public:
    ScreenOdometer()
        : OdometerScreen(_(label)) {
        EnableLongHoldScreenAction();
        Odometer_s::instance().force_to_eeprom();
        Item<MI_ODOMETER_DIST_X>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::X));
        Item<MI_ODOMETER_DIST_Y>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::Y));
        Item<MI_ODOMETER_DIST_Z>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::Z));
        Item<MI_ODOMETER_DIST_E>().UpdateValue(Odometer_s::instance().get_from_eeprom(Odometer_s::axis_t::E));
        Item<MI_ODOMETER_TIME>().UpdateValue(Odometer_s::instance().get_time());
    }
};

ScreenFactory::UniquePtr GetScreenMenuOdometer() {
    return ScreenFactory::Screen<ScreenOdometer>();
}
