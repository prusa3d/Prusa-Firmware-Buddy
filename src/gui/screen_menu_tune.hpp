/**
 * @file screen_menu_tune.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_print.hpp"
#include "MItem_tools.hpp"
#include "MItem_crash.hpp"
#include "menu_items_open.hpp"

/*****************************************************************************/
//parent alias
using ScreenMenuTune__ = ScreenMenu<EFooter::On, MI_RETURN, MI_LIVE_ADJUST_Z, MI_M600, MI_SPEED, MI_NOZZLE,
    MI_HEATBED, MI_PRINTFAN, MI_FLOWFACT, MI_FILAMENT_SENSOR, MI_SOUND_MODE, MI_SOUND_VOLUME, MI_FAN_CHECK

#if ENABLED(CRASH_RECOVERY)
    ,
    MI_CRASH_DETECTION, MI_CRASH_SENSITIVITY_X, MI_CRASH_MAX_PERIOD_X, MI_CRASH_SENSITIVITY_Y, MI_CRASH_MAX_PERIOD_Y
    #if HAS_DRIVER(TMC2130)
    ,
    MI_CRASH_FILTERING
    #endif // HAS_DRIVER(TMC2130)
#endif     // ENABLED(CRASH_RECOVERY)

    ,
    MI_NETWORK, MI_TIMEZONE, MI_VERSION_INFO,

#ifdef _DEBUG
    MI_TEST,
#endif                       //_DEBUG
    /* MI_FOOTER_SETTINGS,*/ //currently experimental, but we want it in future
    MI_MESSAGES>;

class ScreenMenuTune : public ScreenMenuTune__ {
public:
    constexpr static const char *label = N_("TUNE");
    ScreenMenuTune();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
