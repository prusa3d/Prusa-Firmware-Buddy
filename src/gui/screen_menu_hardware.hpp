/// @file
#pragma once

#include "MItem_hardware.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "MItem_crash.hpp"
#include <basic_screen_menu.hpp>
#include <common/extended_printer_type.hpp>
#include <common/printer_variant/printer_variant.hpp>
#include <option/has_auto_retract.h>
#include <option/has_crash_detection.h>
#include <option/has_emergency_stop.h>
#include <option/has_chamber_vents.h>
#include <option/has_mmu2.h>
#include <option/has_phase_stepping.h>
#include <option/has_15gt_belts.h>
#include <option/has_sheet_profiles.h>
#include <option/has_side_fsensor_remap.h>
#include <option/has_toolchanger.h>
#include <option/has_expansion_joints_gen_2.h>
#include <option/has_nozzle_cleaner_lite.h>
#include <option/has_switchable_homing_calibration.h>

#include <option/has_modular_bed.h>
#if HAS_MODULAR_BED()
    #include "screen_menu_modular_bed.hpp"
#endif

#include <option/xbuddy_extension_variant.h>
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include "menu_item/specific/menu_items_xbuddy_extension.hpp"
#endif

using ScreenMenuHardwareBase = BasicScreenMenu<
#if HAS_EXTENDED_PRINTER_TYPE()
    MI_EXTENDED_PRINTER_TYPE,
#endif
#if HAS_PRINTER_VARIANT()
    MI_PRINTER_VARIANT,
#endif

// ================================
// Filament sensor related
// ================================
#if HAS_SIDE_FSENSOR_REMAP()
    MI_SIDE_FSENSOR_REMAP,
#endif
    MI_FS_AUTOLOAD,

// ================================
// Motion related
// ================================
#if HAS_CRASH_DETECTION()
    MI_CRASH_SENSITIVITY_XY, MI_CRASH_MAX_PERIOD_X, MI_CRASH_MAX_PERIOD_Y,
    #if HAS_DRIVER(TMC2130)
    MI_CRASH_FILTERING,
    #endif
#endif
#if HAS_EMERGENCY_STOP()
    MI_EMERGENCY_STOP_ENABLE,
#endif
#if HAS_15GT_BELTS()
    MI_BELTS_15GT,
#endif

// ================================
// Bed related
// ================================
#if HAS_MODULAR_BED()
    MI_HEAT_ENTIRE_BED,
#endif
#if HAS_SHEET_PROFILES()
    MI_STEEL_SHEETS,
#endif
#if HAS_EXPANSION_JOINTS_GEN_2()
    MI_EXPANSION_JOINTS_GEN_2,
#endif

// ================================
// MMU related
// ================================
#if HAS_MMU2()
    MI_HW_MMU,
#endif

#if HAS_CHAMBER_VENTS()
    MI_SWITCH_VENT_MECHANISM,
#endif
#if HAS_AUTO_RETRACT()
    MI_PRE_NOZZLE_CLEANING_RETRACT,
#endif
    MI_GCODE_CHECKS,
#if HAS_SWITCHABLE_HOMING_CALIBRATION()
    MI_AUTO_PRECISE_HOMING_CALIBRATION,
#endif
    MI_INPUT_SHAPER,
#if HAS_PHASE_STEPPING()
    MI_PHASE_STEPPING_SCREEN,
#endif
#if HAS_NOZZLE_CLEANER_LITE()
    MI_NOZZLE_CLEANER_LITE,
#endif
    MI_ALWAYS_HIDDEN>;

class ScreenMenuHardware final : public ScreenMenuHardwareBase {
public:
    ScreenMenuHardware();

private:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
