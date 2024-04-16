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
#if HAS_LOADCELL()
    #include "MItem_loadcell.hpp"
#endif
#if (HAS_TOOLCHANGER())
    #include "screen_menu_tools.hpp"
#endif
#include <option/has_toolchanger.h>
#include <option/has_side_fsensor.h>
#include <option/has_modularbed.h>
#include <option/has_loadcell.h>
#if HAS_MODULARBED()
    #include "screen_menu_modularbed.hpp"
#endif
#if HAS_MMU2()
    #include "MItem_mmu.hpp"
#endif
#include <screen_menu_hw_setup.hpp>
#include <option/has_phase_stepping.h>

class MI_MK4_MK39 : public WI_SWITCH_t<2> {
    static constexpr const char *const label = "Current Printer Type";
    constexpr static const char *const str_mk4 = "MK4";
    constexpr static const char *const str_mk39 = "MK3.9";

    static bool is_mk4();

public:
    MI_MK4_MK39();

protected:
    virtual void OnChange(size_t old_index) override;
};

using ScreenMenuHardware__ = ScreenMenu<GuiDefaults::MenuFooter,
    MI_RETURN,
    MI_NOZZLE_DIAMETER,
    MI_HARDWARE_G_CODE_CHECKS
#if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX
    ,
    MI_HOTEND_SOCK_OR_TYPE
#endif
#if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_iX
    ,
    MI_NOZZLE_TYPE
#endif
#if HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()
    ,
    MI_SIDE_FSENSOR_REMAP
#endif /*HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()*/
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
#if HAS_PHASE_STEPPING()
    ,
    MI_PHASE_STEPPING
#endif
    ,
    MI_FS_AUTOLOAD, MI_EXPERIMENTAL_SETTINGS, MI_XFLASH_RESET, MI_EEPROM
#if HAS_LOADCELL()
    ,
    MI_LOADCELL_SCALE
#else
    ,
    MI_STEEL_SHEETS
#endif
#if (BOARD_IS_XLBUDDY)
    ,
    MI_RESTORE_CALIBRATION_FROM_USB, MI_BACKUP_CALIBRATION_TO_USB,
    MI_FS_SPAN<0, false>, MI_FS_SPAN<0, true>,
    MI_FS_SPAN<1, false>, MI_FS_SPAN<1, true>,
    MI_FS_SPAN<2, false>, MI_FS_SPAN<2, true>,
    MI_FS_SPAN<3, false>, MI_FS_SPAN<3, true>,
    MI_FS_SPAN<4, false>, MI_FS_SPAN<4, true>,
    MI_FS_SPAN<5, false>, MI_FS_SPAN<5, true>,
    MI_FS_REF<0, false>, MI_FS_REF<0, true>,
    MI_FS_REF<1, false>, MI_FS_REF<1, true>,
    MI_FS_REF<2, false>, MI_FS_REF<2, true>,
    MI_FS_REF<3, false>, MI_FS_REF<3, true>,
    MI_FS_REF<4, false>, MI_FS_REF<4, true>,
    MI_FS_REF<5, false>, MI_FS_REF<5, true>
#endif
#if PRINTER_IS_PRUSA_MK4
    ,
    MI_MK4_MK39
#endif
#if HAS_MMU2()
    ,
    MI_MMU_NEXTRUDER_REWORK
    #if HAS_LOADCELL()
    ,
    MI_DONE_EXTRUDER_MAINTENANCE
    #endif // HAS_LOADCELL()
#endif
    >;

class ScreenMenuHardware : public ScreenMenuHardware__ {
public:
    constexpr static const char *label = N_("HARDWARE");
    ScreenMenuHardware();

private:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
