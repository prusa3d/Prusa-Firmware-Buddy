/**
 * @file screen_menu_tune.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_print.hpp"
#include "MItem_tools.hpp"
#include "MItem_crash.hpp"
#include "MItem_menus.hpp"
#include <device/board.h>
#include "config_features.h"
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#include "MItem_bsod.hpp"
#include "MItem_redscreen.hpp"

/*****************************************************************************/
// parent alias
using ScreenMenuTune__ = ScreenMenu<EFooter::On, MI_RETURN,
#if !HAS_LOADCELL()
    MI_LIVE_ADJUST_Z, // position without loadcell
#endif
    MI_M600,
#if ENABLED(CANCEL_OBJECTS)
    MI_CO_CANCEL_OBJECT,
#endif
    MI_SPEED,
    MI_NOZZLE<0>,
#if HAS_TOOLCHANGER()
    MI_NOZZLE<1>, MI_NOZZLE<2>, MI_NOZZLE<3>, MI_NOZZLE<4>,
#endif
    MI_HEATBED, MI_PRINTFAN,
#if HAS_LOADCELL()
    MI_LIVE_ADJUST_Z, // position with loadcell
#endif
    MI_FLOWFACT, MI_FILAMENT_SENSOR, MI_SOUND_MODE,
#if PRINTER_IS_PRUSA_MINI
    MI_SOUND_VOLUME,
#endif
#if PRINTER_IS_PRUSA_MK4
    MI_INPUT_SHAPER,
#endif
    MI_FAN_CHECK
#if ENABLED(CRASH_RECOVERY)
    ,
    MI_CRASH_DETECTION,
    MI_CRASH_SENSITIVITY_XY
#endif // ENABLED(CRASH_RECOVERY)
    ,
    MI_USER_INTERFACE, MI_NETWORK,
#if (!PRINTER_IS_PRUSA_MINI) || defined(_DEBUG) // Save space in MINI release
    MI_HARDWARE_TUNE,
#endif                                          /*(!PRINTER_IS_PRUSA_MINI) || defined(_DEBUG)*/
    MI_TIMEZONE, MI_INFO, MI_TRIGGER_POWER_PANIC,

#ifdef _DEBUG
    MI_TEST,
#endif                       //_DEBUG
    /* MI_FOOTER_SETTINGS,*/ // currently experimental, but we want it in future
    MI_MESSAGES, MI_TRIGGER_BSOD, MI_TRIGGER_REDSCREEN>;

class ScreenMenuTune : public ScreenMenuTune__ {
public:
    constexpr static const char *label = N_("TUNE");
    ScreenMenuTune();

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
