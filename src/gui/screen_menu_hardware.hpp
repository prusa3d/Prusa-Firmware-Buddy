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
#include <option/has_sheet_profiles.h>
#include <option/has_toolchanger.h>
#include <option/has_side_fsensor.h>
#include <option/has_modularbed.h>
#include <common/extended_printer_type.hpp>
#include "MItem_basic_selftest.hpp"
#include "printers.h"

#if HAS_LOADCELL()
    #include "MItem_loadcell.hpp"
#endif
#if HAS_TOOLCHANGER()
    #include "screen_menu_tools.hpp"
#endif
#if HAS_MODULARBED()
    #include "screen_menu_modularbed.hpp"
#endif
#if HAS_MMU2()
    #include "MItem_mmu.hpp"
#endif

using ScreenMenuHardware__ = ScreenMenu<GuiDefaults::MenuFooter,
    MI_RETURN,
#if HAS_EXTENDED_PRINTER_TYPE()
    MI_EXTENDED_PRINTER_TYPE,
#endif
    MI_TOOLHEAD_SETTINGS,

// ================================
// Filament sensor related
// ================================
#if HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()
    MI_SIDE_FSENSOR_REMAP,
#endif /*HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()*/
    MI_FS_AUTOLOAD,

// ================================
// Motion related
// ================================
#if ENABLED(CRASH_RECOVERY)
    MI_CRASH_SENSITIVITY_XY, MI_CRASH_MAX_PERIOD_X, MI_CRASH_MAX_PERIOD_Y,
    #if HAS_DRIVER(TMC2130)
    MI_CRASH_FILTERING,
    #endif
#endif // ENABLED(CRASH_RECOVERY)
#if HAS_EMERGENCY_STOP()
    MI_EMERGENCY_STOP_ENABLE,
#endif

// ================================
// Bed related
// ================================
#if ENABLED(MODULAR_HEATBED)
    MI_HEAT_ENTIRE_BED,
#endif
#if HAS_SHEET_PROFILES()
    MI_STEEL_SHEETS,
#endif

    // ================================
    // Other
    // ================================
    MI_HARDWARE_G_CODE_CHECKS,
#if HAS_ILI9488_DISPLAY()
    MI_DISPLAY_BAUDRATE,
#endif

    // ================================
    // Dev-only items
    // ================================
    MI_PRINTER_SETUP,
    MI_EXPERIMENTAL_SETTINGS,
    MI_XFLASH_RESET, MI_EEPROM
#ifdef HAS_TMC_WAVETABLE
    ,
    MI_WAVETABLE_XYZ
#endif
#if HAS_LOADCELL()
    ,
    MI_LOADCELL_SCALE
#endif
#if (BOARD_IS_XLBUDDY())
    ,
    MI_RESTORE_CALIBRATION_FROM_USB, MI_BACKUP_CALIBRATION_TO_USB
#endif
    >;

class ScreenMenuHardware : public ScreenMenuHardware__ {
public:
    constexpr static const char *label = N_("HARDWARE");
    ScreenMenuHardware();

private:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
