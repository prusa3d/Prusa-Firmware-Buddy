/**
 * @file screen_menu_hardware.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_menus.hpp"
#include "MItem_crash.hpp"
#include "MItem_hardware.hpp"
#include "printers.h"
#include <option/has_loadcell.h>
#include "MItem_basic_selftest.hpp"
#include "printers.h"

using ScreenMenuHardware__ = ScreenMenu<GuiDefaults::MenuFooter,
    MI_RETURN,
    MI_NOZZLE_DIAMETER,
    MI_HARDWARE_G_CODE_CHECKS,
#if PRINTER_TYPE == PRINTER_PRUSA_MK4
    MI_NOZZLE_SOCK,
    MI_NOZZLE_TYPE,
#endif
    MI_INDEPT_STEP
#if ENABLED(MODULAR_HEATBED)
    ,
    MI_HEAT_ENTIRE_BED
#endif
#ifdef HAS_TMC_WAVETABLE
    ,
    MI_WAVETABLE_XYZ
#endif
#if ENABLED(CRASH_RECOVERY)
    ,
    MI_CRASH_SENSITIVITY_XY, MI_CRASH_MAX_PERIOD_X, MI_CRASH_MAX_PERIOD_Y
    #if HAS_DRIVER(TMC2130)
    ,
    MI_CRASH_FILTERING
    #endif
#endif // ENABLED(CRASH_RECOVERY)
    ,
    MI_FS_AUTOLOAD, MI_EXPERIMENTAL_SETTINGS, MI_XFLASH_RESET, MI_HF_TEST_0, MI_HF_TEST_1, MI_EEPROM
#if HAS_LOADCELL()
    ,
    MI_LOADCELL_SCALE
#endif
#if (BOARD_IS_XLBUDDY)
    ,
    MI_RESTORE_CALIBRATION_FROM_USB, MI_BACKUP_CALIBRATION_TO_USB,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_0>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_0>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_1>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_1>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_2>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_2>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_3>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_3>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_4>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_4>,
    MI_FS_SPAN<EEVAR_FS_VALUE_SPAN_5>, MI_FS_SPAN<EEVAR_SIDE_FS_VALUE_SPAN_5>
#endif
    >;

class ScreenMenuHardware : public ScreenMenuHardware__ {
public:
    constexpr static const char *label = N_("HARDWARE");
    ScreenMenuHardware();

private:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
