/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config_features.h"
#include "module/motion.h"
#include "module/tool_change.h"
#include "marlin_stubs/PrusaGcodeSuite.hpp"
#include <logging/log.hpp>
#include <filament_to_load.hpp>

LOG_COMPONENT_REF(PRUSA_GCODE);

// clang-format off
#if (!ENABLED(ADVANCED_PAUSE_FEATURE)) || \
    HAS_LCD_MENU || \
    ENABLED(MMU2_MENUS) || \
    ENABLED(MIXING_EXTRUDER) || \
    ENABLED(DUAL_X_CARRIAGE) || \
    HAS_BUZZER
    #error unsupported
#endif
// clang-format on

#include "Marlin/src/gcode/gcode.h"
#include "Marlin/src/module/motion.h"
#include "Marlin/src/module/temperature.h"
#include "Marlin/src/feature/prusa/e-stall_detector.h"
#include "marlin_server.hpp"
#include "pause_stubbed.hpp"
#include <cmath>
#include "filament_sensors_handler.hpp"
#include "filament.hpp"
#include <gcode/gcode_parser.hpp>

#include <option/has_leds.h>
#if HAS_LEDS()
    #include "led_animations/printer_animation_state.hpp"
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
    #include "module/prusa/spool_join.hpp"
#endif
#if ENABLED(CRASH_RECOVERY)
    #include <feature/prusa/crash_recovery.hpp>
#endif /*ENABLED(CRASH_RECOVERY)*/
#include <option/has_toolchanger.h>
#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

static void M600_manual(const GCodeParser2 &);

#include <config_store/store_instance.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M600: Pause for filament change <a href="https://reprap.org/wiki/G-code#M600:_Filament_change_pause">M600: Filament change pause</a>
 *
 *
 *#### Usage
 *
 *    M [ E | X | Y | Z | U | L | B | T | A | C | S | N ]
 *
 *#### Parameters
 *
 * - `E` - Retract before moving to change position
 * - `Z` - Z relative lift for filament change position
 * - `X` - X position for filament change
 * - `Y` - Y position for filament change
 * - `U` - Amount of retraction for unload (negative)
 * - `L` - Load length, longer for bowden (positive)
 * - `B` - Number of beeps to alert user of filament change
 *   - `-1` - for indefinite
 * - `T` - Target extruder
 * - `A` - If automatic spool join is configured for this tool, do that instead, if not, do manual filament change
 * - `C` - Set color for filament change (color rgb value as integer)
 * - `C"color"` - Set color for filament change (color name as string)
 * - `S"filament"` - Set filament type for filament change. RepRap compatible.
 * - `N` - No return, don't return to previous position after fillament change
 *
 *  Default values are used for omitted arguments.
 */

void GcodeSuite::M600() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    const bool is_auto_m600 = p.option<bool>('A').value_or(false);

    bool do_manual_m600 = true;

#if ENABLED(PRUSA_SPOOL_JOIN)
    if (is_auto_m600) {

        uint8_t current_tool = 0;
    #if HAS_TOOLCHANGER()
        current_tool = active_extruder;
    #elif HAS_MMU2()
        // active_extruder variable is not altered by MMU2 (always 0)
        // We need to select current filament slot
        current_tool = MMU2::mmu2.get_current_tool();
    #endif

        if (spool_join.do_join(current_tool)) {
            // if automatic M600 succeeded, don't do manual M600, if not, do manual M600
            do_manual_m600 = false;
        }
    }
#endif

    if (do_manual_m600) {
        M600_manual(p);
    }

    if (is_auto_m600) {
        FSensors_instance().ClrM600Sent(); // reset filament sensor M600 sent flag
    }
}

/** @}*/

void M600_execute(xyz_pos_t park_point, uint8_t target_extruder,
    xyze_float_t resume_point, std::optional<float> unloadLength, std::optional<float> fastLoadLength,
    std::optional<float> retractLength, std::optional<Color> filament_colour,
    std::optional<FilamentType> filament_type, bool);

void M600_manual(const GCodeParser2 &p) {
    const int8_t target_extruder = PrusaGcodeSuite::get_target_extruder_from_command(p);
    if (target_extruder < 0) {
        return;
    }

#if HAS_LEDS()
    auto guard = PrinterStateAnimation::force_printer_state(PrinterState::Warning);
#endif

    xyz_pos_t park_point =
#ifdef XYZ_NOZZLE_PARK_POINT_M600
        XYZ_NOZZLE_PARK_POINT_M600;
#else
        XYZ_NOZZLE_PARK_POINT;
#endif

    // Lift Z axis
    if (p.store_option('Z', park_point.z)) {
        park_point.z = LOGICAL_TO_NATIVE(park_point.z, Z_AXIS);
    }

    // Move XY axes to filament change position or given position
    if (p.store_option('X', park_point.x)) {
        park_point.x = LOGICAL_TO_NATIVE(park_point.x, X_AXIS);
    }
    if (p.store_option('Y', park_point.y)) {
        park_point.y = LOGICAL_TO_NATIVE(park_point.y, Y_AXIS);
    }

#if HAS_HOTEND_OFFSET && NONE(DUAL_X_CARRIAGE, DELTA) && DISABLED(PRUSA_TOOLCHANGER)
    park_point += hotend_offset[active_extruder];
#endif

    const xyze_float_t no_return = { { { NAN, NAN, NAN, current_position.e } } };

    M600_execute(park_point,
        target_extruder,
        p.option<bool>('N') ? no_return : current_position,
        p.option<float>('U'),
        p.option<float>('L'),
        p.option<float>('E').transform(fabsf),
        p.option<Color>('C'),
        p.option<FilamentType>('S'),
        false);
}

void M600_execute(xyz_pos_t park_point, uint8_t target_extruder, xyze_float_t resume_point,
    std::optional<float> unloadLength, std::optional<float> fastLoadLength, std::optional<float> retractLength,
    std::optional<Color> filament_colour, std::optional<FilamentType> filament_type,
    bool is_filament_stuck) {

    // Ignore estalls during filament change
    BlockEStallDetection estall_blocker;

#if ENABLED(CRASH_RECOVERY)
    if (crash_s.get_state() != Crash_s::PRINTING && crash_s.get_state() != Crash_s::IDLE) {
        return; // Ignore M600 if crash recovery is in progress
    }
#endif /*ENABLED(CRASH_RECOVERY)*/

#if HAS_TOOLCHANGER()
    struct ToolChangeData {
        xyze_float_t original_resume_point;
        int16_t target_extruder_original_temperature;
        uint8_t original_extruder;
    };

    // Check if we need to do a toolchange
    std::optional<ToolChangeData> tool_change_data {};
    if (target_extruder != marlin_vars().active_extruder) {
        // Since the native coordinates contain hotend_currently_applied_offset we need to store the logical
        // version of these coordinates to make it easier to convert to the target_extruder's native coordinates.
        const auto logical_resume = resume_point.asLogical();
        tool_change_data = ToolChangeData {
            .original_resume_point = logical_resume,
            .target_extruder_original_temperature = Temperature::degTargetHotend(target_extruder),
            .original_extruder = marlin_vars().active_extruder,
        };

        tool_change(target_extruder, tool_return_t::no_return, tool_change_lift_t::mbl_only_lift, true);

        resume_point = logical_resume.asNative(); // Convert original resume point to the new native coordinates
        resume_point = prusa_toolchanger.get_tool_dock_position(target_extruder); // Sets only x, y coordinates

        // Preheat the tool for filament change -> normally we don't do that for M600. But the slicer team wanted this.
        const auto filament_data = config_store().get_filament_type(target_extruder).parameters();
        Temperature::setTargetHotend(filament_data.nozzle_temperature, target_extruder);
        Temperature::wait_for_hotend(target_extruder);
    }
#endif
    park_point.z += current_position.z;

    pause::Settings settings;
    settings.SetParkPoint(park_point);
    settings.SetResumePoint(resume_point);
    if (unloadLength.has_value()) {
        settings.SetUnloadLength(unloadLength.value());
    }
    if (fastLoadLength.has_value()) {
        settings.SetFastLoadLength(fastLoadLength.value());
    }
    if (retractLength.has_value()) {
        settings.SetRetractLength(retractLength.value());
    } // Initial retract before move to filament change position
    settings.SetExtruder(target_extruder);

    // If paused restore nozzle temperature from pre-paused state
    if (marlin_server::printer_paused()) {
        marlin_server::unpause_nozzle(target_extruder);
    }

    const float disp_temp = marlin_vars().hotend(target_extruder).display_nozzle;
    const float targ_temp = Temperature::degTargetHotend(target_extruder);

    marlin_server::nozzle_timeout_off();
    if (disp_temp > targ_temp) {
        Temperature::setTargetHotend(disp_temp, target_extruder);
    }

    if (filament_type.has_value()) {
        config_store().set_filament_type(target_extruder, filament_type.value());
    }

    filament::set_type_to_load(config_store().get_filament_type(target_extruder));
    filament::set_color_to_load(filament_colour);
    Pause::Instance().filament_change(settings, is_filament_stuck);

    marlin_server::nozzle_timeout_on();
    if (disp_temp > targ_temp) {
        Temperature::setTargetHotend(targ_temp, target_extruder);
    }

#if HAS_TOOLCHANGER()
    if (tool_change_data.has_value()) {
        const auto &change_data = *tool_change_data;

        if (std::isfinite(change_data.target_extruder_original_temperature)) {
            Temperature::setTargetHotend(change_data.target_extruder_original_temperature, target_extruder);
        }

        if (std::isfinite(change_data.original_resume_point.x) && std::isfinite(change_data.original_resume_point.y) && std::isfinite(change_data.original_resume_point.z)) {
            destination = change_data.original_resume_point.asNative();
        } else {
            destination = prusa_toolchanger.get_tool_dock_position(change_data.original_extruder);
        }
        tool_change(change_data.original_extruder, tool_return_t::to_destination, tool_change_lift_t::mbl_only_lift, true);
        report_current_position();
    }
#endif
}

/**
 *### M1601: Filament stuck detected during print <a href=" "> </a>
 *
 * Internal GCode
 *
 * Enabled for LoadCell equipped printers
 *
 * Only MK3.9/S, MK4/S and XL
 *#### Usage
 *
 *    M1601
 *
 */
#if HAS_LOADCELL()
void PrusaGcodeSuite::M1601() {
    M600_execute(
        XYZ_NOZZLE_PARK_POINT_M600,
        active_extruder,
        current_position,
        std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt,
        true);

    EMotorStallDetector::Instance().ClearReported();
}
#else

void PrusaGcodeSuite::M1601() {
    log_error(PRUSA_GCODE, "M1601 unsupported");
}

#endif
