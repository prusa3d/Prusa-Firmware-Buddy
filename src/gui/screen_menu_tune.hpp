/**
 * @file screen_menu_tune.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_print.hpp"
#include "MItem_tools.hpp"
#include "MItem_crash.hpp"
#include "MItem_menus.hpp"
#include "MItem_mmu.hpp"
#include <device/board.h>
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#include <option/developer_mode.h>
#include <option/has_mmu2.h>
#include <device/board.h>
#if XL_ENCLOSURE_SUPPORT()
    #include "MItem_enclosure.hpp"
#endif
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
    MI_FLOWFACT<0>,
#if HAS_TOOLCHANGER()
    MI_FLOWFACT<1>, MI_FLOWFACT<2>, MI_FLOWFACT<3>, MI_FLOWFACT<4>,
#endif /*HAS_TOOLCHANGER()*/
#if HAS_FILAMENT_SENSORS_MENU()
    MI_FILAMENT_SENSORS,
#else
    MI_FILAMENT_SENSOR,
#endif
#if HAS_LOADCELL()
    MI_STUCK_FILAMENT_DETECTION,
#endif
#if XL_ENCLOSURE_SUPPORT()
    MI_ENCLOSURE_ENABLE,
    MI_ENCLOSURE,
#endif
    MI_STEALTH_MODE,
    MI_SOUND_MODE,
#if PRINTER_IS_PRUSA_MINI
    MI_SOUND_VOLUME,
#endif
    MI_INPUT_SHAPER,
#if HAS_PHASE_STEPPING()
    MI_PHASE_STEPPING,
#endif
    MI_FAN_CHECK,
    MI_GCODE_VERIFY
#if HAS_MMU2()
    ,
    MI_MMU_CUTTER
#endif
#if ENABLED(CRASH_RECOVERY)
    ,
    MI_CRASH_DETECTION,
    MI_CRASH_SENSITIVITY_XY
#endif // ENABLED(CRASH_RECOVERY)
    ,
    MI_USER_INTERFACE, MI_NETWORK,
#if (!PRINTER_IS_PRUSA_MINI) || defined(_DEBUG) // Save space in MINI release
    MI_HARDWARE_TUNE,
#endif /*(!PRINTER_IS_PRUSA_MINI) || defined(_DEBUG)*/
    MI_TIMEZONE, MI_TIMEZONE_MIN, MI_TIMEZONE_SUMMER, MI_INFO, MI_TRIGGER_POWER_PANIC,

/* MI_FOOTER_SETTINGS,*/ // currently experimental, but we want it in future
#if DEVELOPER_MODE()
    MI_ERROR_TEST,
#endif /*DEVELOPMENT_ITEMS()*/
    MI_MESSAGES>;

class ScreenMenuTune : public ScreenMenuTune__ {
public:
    constexpr static const char *label = N_("TUNE");
    ScreenMenuTune();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
