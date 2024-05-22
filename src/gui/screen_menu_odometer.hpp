/**
 * @file screen_menu_odometer.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_tools.hpp"
#if (HAS_TOOLCHANGER())
    #include "screen_menu_tools.hpp"
#endif
#include "src/module/prusa/toolchanger.h"

using ScreenMenuOdometer__ = ScreenMenu<EFooter::On, MI_RETURN, MI_ODOMETER_DIST_X, MI_ODOMETER_DIST_Y, MI_ODOMETER_DIST_Z, MI_ODOMETER_DIST_E,
#if HAS_TOOLCHANGER()
    MI_ODOMETER_DIST_E_N<0>,
    MI_ODOMETER_DIST_E_N<1>,
    MI_ODOMETER_DIST_E_N<2>,
    MI_ODOMETER_DIST_E_N<3>,
    MI_ODOMETER_DIST_E_N<4>,
    MI_ODOMETER_TOOL,
    MI_ODOMETER_TOOL_N<0>,
    MI_ODOMETER_TOOL_N<1>,
    MI_ODOMETER_TOOL_N<2>,
    MI_ODOMETER_TOOL_N<3>,
    MI_ODOMETER_TOOL_N<4>,
#endif /*HAS_TOOLCHANGER()*/
#if HAS_MMU2()
    MI_ODOMETER_MMU_CHANGES,
#endif
    MI_ODOMETER_TIME>;

class ScreenMenuOdometer : public ScreenMenuOdometer__ {
    static constexpr const char *label = N_("ODOMETER");

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

public:
    ScreenMenuOdometer();
};
