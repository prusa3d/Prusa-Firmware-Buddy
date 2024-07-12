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

static void M600_manual();

#include <config_store/store_instance.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 * M600: Pause for filament change
 *
 * Parameters:
 * - E[distance] - Retract the filament this far (initial retraction)
 * - Z[distance] - Move the Z axis by this distance
 * - X[position] - Move to this X position, with Y
 * - Y[position] - Move to this Y position, with X
 * - U[distance] - Retract distance for removal (manual reload)
 * - L[distance] - Extrude distance for insertion (manual reload)
 * - B[count]    - Number of times to beep, -1 for indefinite (if equipped with a buzzer)
 * - T[toolhead] - Select extruder for filament change
 * - A           - If automatic spool join is configured for this tool, do that instead, if not, do manual filament change
 * - C[color]    - Set color for filament change (color rgb value as integer)
 * - C"color"    - Set color for filament change (color name as string)
 * - S"filament" - Set filament type for filament change. RepRap compatible.
 * - N           - No return, don't return to previous position after fillament change
 *
 *  Default values are used for omitted arguments.
 */

void GcodeSuite::M600() {
    const bool is_auto_m600 = parser.seen('A');

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
        M600_manual();
    }

    if (is_auto_m600) {
        FSensors_instance().ClrM600Sent(); // reset filament sensor M600 sent flag
    }
}

/** @}*/

void M600_execute(xyz_pos_t park_point, uint8_t target_extruder,
    xyze_float_t resume_point, std::optional<float> unloadLength, std::optional<float> fastLoadLength,
    std::optional<float> retractLength, std::optional<Color> filament_colour,
    std::optional<FilamentType> filament_type, pause::Settings::CalledFrom);

void M600_manual() {
    char colourtype[16] = { '\0' };

    const auto filament_to_be_loaded = PrusaGcodeSuite::get_filament_type_from_command('S');

    if (parser.seen('C')) {
        const char *colourtype_ptr = nullptr;
        if ((colourtype_ptr = strstr(parser.string_arg, "C\"")) != nullptr) {
            const char *text_begin = strchr(colourtype_ptr, '"');
            if (text_begin) {
                ++text_begin;
                strlcpy(colourtype, text_begin, sizeof(colourtype));
                for (char *fn = colourtype; *fn; ++fn) {
                    if (*fn == '"') {
                        *fn = '\0';
                        break;
                    }
                }
            }
        } else if ((colourtype_ptr = strstr(parser.string_arg, "C ")) != nullptr) {
            const char *text_begin = strchr(colourtype_ptr, ' ');
            if (text_begin) {
                ++text_begin;
                strlcpy(colourtype, text_begin, sizeof(colourtype));
                for (char *fn = colourtype; *fn; ++fn) {
                    if (*fn == ' ') {
                        *fn = '\0';
                        break;
                    }
                }
            }
        }
    }

    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0) {
        return;
    }
#if HAS_LEDS()
    auto guard = PrinterStateAnimation::force_printer_state(PrinterState::Warning);
#endif

    xyz_pos_t park_point =
#ifdef NOZZLE_PARK_POINT_M600
        NOZZLE_PARK_POINT_M600;
#else
        NOZZLE_PARK_POINT;
#endif

    // Lift Z axis
    if (parser.seenval('Z')) {
        park_point.z = parser.linearval('Z');
        park_point.z = LOGICAL_TO_NATIVE(park_point.z, Z_AXIS);
    }

    // Move XY axes to filament change position or given position
    if (parser.seenval('X')) {
        park_point.x = parser.linearval('X');
        park_point.x = LOGICAL_TO_NATIVE(park_point.x, X_AXIS);
    }
    if (parser.seenval('Y')) {
        park_point.y = parser.linearval('Y');
        park_point.y = LOGICAL_TO_NATIVE(park_point.y, Y_AXIS);
    }

#if HAS_HOTEND_OFFSET && NONE(DUAL_X_CARRIAGE, DELTA) && DISABLED(PRUSA_TOOLCHANGER)
    park_point += hotend_offset[active_extruder];
#endif

    static const xyze_float_t no_return = { { { NAN, NAN, NAN, current_position.e } } };

    M600_execute(park_point,
        target_extruder,
        parser.seen('N') ? no_return : current_position,
        parser.seen('U') ? std::make_optional(parser.value_axis_units(E_AXIS)) : std::nullopt,
        parser.seen('L') ? std::make_optional(parser.value_axis_units(E_AXIS)) : std::nullopt,
        parser.seen('E') ? std::make_optional(std::abs(parser.value_axis_units(E_AXIS))) : std::nullopt,
        parser.seen('C') ? Color::from_string(colourtype) : std::nullopt,
        parser.seen('S') ? std::make_optional(filament_to_be_loaded) : std::nullopt,
        pause::Settings::CalledFrom::Pause);
}

void M600_execute(xyz_pos_t park_point, uint8_t target_extruder, xyze_float_t resume_point,
    std::optional<float> unloadLength, std::optional<float> fastLoadLength, std::optional<float> retractLength,
    std::optional<Color> filament_colour, std::optional<FilamentType> filament_type,
    pause::Settings::CalledFrom called_from) {

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
        const auto &filament_data = filament::get_description(config_store().get_filament_type(target_extruder));
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
    settings.SetCalledFrom(called_from);
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
    Pause::Instance().FilamentChange(settings);

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

/// Filament stuck detected during print
///
/// ## Parameters
/// none so far
///
/// ## Notes
/// Enabled for LoadCell equipped printers
#if HAS_LOADCELL()
void PrusaGcodeSuite::M1601() {
    M600_execute(
        NOZZLE_PARK_POINT_M600,
        active_extruder,
        current_position,
        std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt,
        pause::Settings::CalledFrom::FilamentStuck);

    EMotorStallDetector::Instance().ClearReported();
}
#else

void PrusaGcodeSuite::M1601() {
    log_error(PRUSA_GCODE, "M1601 unsupported");
}

#endif
