/**
 * @file screen_menu_odometer.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"

using ScreenMenuOdometer__ = ScreenMenu<EFooter::On, MI_RETURN, MI_ODOMETER_DIST_X, MI_ODOMETER_DIST_Y, MI_ODOMETER_DIST_Z, MI_ODOMETER_DIST_E, MI_ODOMETER_TIME>;

class ScreenMenuOdometer : public ScreenMenuOdometer__ {
    static constexpr const char *label = N_("ODOMETER");

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    ScreenMenuOdometer();
};
