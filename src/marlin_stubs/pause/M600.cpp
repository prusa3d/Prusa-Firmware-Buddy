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
#include "utils/variant_utils.hpp"
#include <logging/log.hpp>
#include <filament_to_load.hpp>

LOG_COMPONENT_REF(PRUSA_GCODE);

#include <option/has_crash_detection.h>
#include <option/has_pause.h>
static_assert(HAS_PAUSE());

#include "Marlin/src/gcode/gcode.h"
#include "Marlin/src/module/motion.h"
#include "Marlin/src/module/temperature.h"
#include "Marlin/src/feature/prusa/e-stall_detector.h"
#include "marlin_server.hpp"
#include "pause_stubbed.hpp"
#include <algorithm>
#include <cmath>
#include <feature/safety_timer/safety_timer.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include "filament.hpp"
#include <gcode/gcode_parser.hpp>

#include <option/has_leds.h>
#if HAS_LEDS()
    #include "leds/status_leds_handler.hpp"
#endif

#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

#if HAS_CRASH_DETECTION()
    #include <feature/prusa/crash_recovery.hpp>
#endif

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "Marlin/src/feature/prusa/MMU2/mmu2_mk4.h"
#endif

static void M600_manual(const GCodeParser2 &);

#include <common/mapi/parking.hpp>
#include <config_store/store_instance.hpp>
#include <bsod/bsod.h>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M600: Pause for filament change <a href="https://reprap.org/wiki/G-code#M600:_Filament_change_pause">M600: Filament change pause</a>
 *
 *
 *#### Usage
 *
 *    M600 [ E | X | Y | Z | U | L | B | T | A | C | S | N | P ]
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
 * - `P` - If set, the parameter 'T' is interpreted as a VirtualToolIndex (tool mapping is not applied)
 *
 *  Default values are used for omitted arguments.
 *
 *  It needs to be noted that M600's S"filament" parameter is currently not actually setting the target temperature for the desired filament type.
 *  In fact M600 never sets a target temperature for filament change (only when picking and inactive toolhead on a printer with toolchanger).
 *  Temperature that is currently set will be used for both unloading and loading.
 */

void GcodeSuite::M600() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    const bool is_auto_m600 = p.option<bool>('A').value_or(false);

    bool do_manual_m600 = true;

#if HAS_SPOOL_JOIN()
    if (is_auto_m600) {
        auto virtual_tool = stdext::get_optional<VirtualToolIndex>(VirtualToolIndex::currently_selected());
        if (!virtual_tool.has_value()) {
            bsod("Spool join to notool");
        }

        if (spool_join.do_join(*virtual_tool)) {
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

void M600_execute(mapi::ParkingPosition park_position, VirtualToolIndex target_tool,
    xyze_float_t resume_point, std::optional<float> unloadLength, std::optional<float> fastLoadLength,
    std::optional<float> retractLength, std::optional<Color> filament_colour,
    std::optional<FilamentType> filament_type, bool);

void M600_manual(const GCodeParser2 &p) {
    const std::optional<VirtualToolIndex> virtual_tool = stdext::get_optional<VirtualToolIndex>(PrusaGcodeSuite::get_target_virtual_from_command_p(p));
    if (!virtual_tool.has_value()) {
        return;
    }
    VirtualToolIndex target_tool = *virtual_tool;

    XYZval<float, LogicalPosTag> logical_park_point { NAN, NAN, NAN };

    // Lift Z axis
    p.store_option_if_present('Z', logical_park_point.z);

    // Move XY axes to filament change position or given position
    p.store_option_if_present('X', logical_park_point.x);
    p.store_option_if_present('Y', logical_park_point.y);

    const auto park_point = logical_park_point.asNative();

    auto park_position = mapi::get_parking_position(mapi::ParkPosition::filament_change);
    if (!std::isnan(park_point.x)) {
        park_position.x = park_point.x;
    }
    if (!std::isnan(park_point.y)) {
        park_position.y = park_point.y;
    }
    if (!std::isnan(park_point.z)) {
        park_position.z = park_point.z;
    }

    const xyze_float_t no_return = { { { NAN, NAN, NAN, current_position.e } } };

    M600_execute(park_position,
        target_tool,
        p.option<bool>('N') ? no_return : current_position,
        p.option<float>('U'),
        p.option<float>('L'),
        p.option<float>('E').transform(fabsf),
        p.option<Color>('C'),
        p.option<FilamentType>('S'),
        false);
}

void M600_execute(mapi::ParkingPosition park_position, VirtualToolIndex target_tool, xyze_float_t resume_point,
    std::optional<float> unloadLength, std::optional<float> fastLoadLength, std::optional<float> retractLength,
    std::optional<Color> filament_colour, std::optional<FilamentType> filament_type,
    bool is_filament_stuck) {

    // Ignore estalls during filament change
    BlockEStallDetection estall_blocker;

    auto physical_target_tool = target_tool.to_physical();

#if HAS_TOOLCHANGER()
    struct ToolChangeData {
        XYZEval<float, LogicalPosTag> original_resume_point;
        int16_t target_extruder_original_temperature;
        std::variant<VirtualToolIndex, NoTool> original_extruder;
    };

    // Check if we need to do a toolchange
    std::optional<ToolChangeData> tool_change_data {};
    if (!stdext::holds_value(VirtualToolIndex::currently_selected(), target_tool)) {
        // Since the native coordinates contain hotend_currently_applied_offset we need to store the logical
        // version of these coordinates to make it easier to convert to the target_extruder's native coordinates.
        const auto logical_resume = resume_point.asLogical();
        tool_change_data = ToolChangeData {
            .original_resume_point = logical_resume,
            .target_extruder_original_temperature = Temperature::degTargetHotend(physical_target_tool),
            .original_extruder = marlin_vars().active_extruder,
        };

        tool_change(target_tool, tool_return_t::no_return, tool_change_lift_t::mbl_only_lift, true);

        resume_point = logical_resume.asNative(); // Convert original resume point to the new native coordinates
        resume_point.set(prusa_toolchanger.get_tool_dock_position(physical_target_tool)); // Sets only x, y coordinates

        // Sets the target temperature based on the current filament type
        // M600 generally should not set target temperature, this is an exception for specific scenario where user wants to change filament on currently unused toolhead during print
        const auto filament_data = FilamentType::for_tool_heuristic(target_tool).parameters();
        Temperature::setTargetHotend(filament_data.nozzle_temperature, physical_target_tool);
    }
#endif
    // X/Y are taken as-is; an absolute Z becomes an AtLeast (never-go-down).
    if (auto *z = std::get_if<float>(&park_position.z)) {
        park_position.z = mapi::ParkingPosition::AtLeast { .above_print = Z_NOZZLE_PARK_RISE_M600, .absolute = *z };
    } else {
        // Other alternatives express the intended park behavior on their own; flag a caller not asking for any Z handling
        debug_assert(!std::holds_alternative<mapi::ParkingPosition::Unchanged>(park_position.z));
    }
    pause::Settings settings;
    settings.SetParkPoint(park_position);
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
    settings.SetExtruder(target_tool);

    // A pause that outlives the safety timer leaves the heaters disabled, which zeroes
    // both the target and the displayed temperature. Restore them before snapshotting,
    // otherwise the resume temperature captured below is 0 and turns the nozzle off.
    // Pause::filament_change() restores anyway; this only moves it ahead of the read.
    buddy::safety_timer().reset_restore_nonblocking();

    const float disp_temp = marlin_vars().hotend(physical_target_tool).display_nozzle;
    const float targ_temp = Temperature::degTargetHotend(physical_target_tool);

    if (disp_temp > targ_temp) {
        Temperature::setTargetHotend(static_cast<int16_t>(disp_temp), physical_target_tool);
    }

    // Loading drops the target to the new filament's default; restore the print temperature
    // on resume. Snapshot what the branch above left in effect, not the pre-raise target.
    settings.SetResumeNozzleTemperature(static_cast<int16_t>(std::max(disp_temp, targ_temp)));

    if (filament_type.has_value()) {
        config_store().set_filament_type(target_tool, filament_type.value());
    } else {
        filament_type = FilamentType::for_tool_heuristic(target_tool);
    }

    filament::set_type_to_load(*filament_type);
    filament::set_color_to_load(filament_colour);
    Pause::Instance().filament_change(settings, is_filament_stuck);

#if HAS_TOOLCHANGER()
    if (tool_change_data.has_value()) {
        const auto &change_data = *tool_change_data;

        if (std::isfinite(change_data.target_extruder_original_temperature)) {
            Temperature::setTargetHotend(change_data.target_extruder_original_temperature, physical_target_tool);
        }

        destination = current_position;
        if (std::isfinite(change_data.original_resume_point.x) && std::isfinite(change_data.original_resume_point.y) && std::isfinite(change_data.original_resume_point.z)) {
            destination = change_data.original_resume_point.asNative();
        } else {
            destination.set(match(
                change_data.original_extruder,
                [](VirtualToolIndex virtual_tool) { return prusa_toolchanger.get_tool_dock_position(virtual_tool.to_physical()); },
                [](NoTool) -> xy_float_t { return current_position.xy(); }));
        }
        tool_change(stdext::to_variant(change_data.original_extruder), tool_return_t::to_destination, tool_change_lift_t::mbl_only_lift, true);
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
 *    M1601 V<virtual tool>
 *
 */
#if HAS_LOADCELL()
void PrusaGcodeSuite::M1601() {
    M600_execute(
        mapi::get_parking_position(mapi::ParkPosition::filament_change),
        VirtualToolIndex::from_raw(parser.intval('V', -1)),
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
