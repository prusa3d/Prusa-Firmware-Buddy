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

#include "../../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"

// clang-format off
#if (!ENABLED(ADVANCED_PAUSE_FEATURE)) || \
    EXTRUDERS > 1 || \
    HAS_LCD_MENU || \
    ENABLED(MMU2_MENUS) || \
    ENABLED(MIXING_EXTRUDER) || \
    ENABLED(DUAL_X_CARRIAGE)
    #error unsupported
#endif
// clang-format on

#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/feature/pause.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/printcounter.h"
#include "marlin_server.hpp"

/**
 * M600: Pause for filament change
 *
 *  E[distance] - Retract the filament this far
 *  Z[distance] - Move the Z axis by this distance
 *  X[position] - Move to this X position, with Y
 *  Y[position] - Move to this Y position, with X
 *  U[distance] - Retract distance for removal (manual reload)
 *  L[distance] - Extrude distance for insertion (manual reload)
 *  B[count]    - Number of times to beep, -1 for indefinite (if equipped with a buzzer)
 *  T[toolhead] - Select extruder for filament change
 *
 *  Default values are used for omitted arguments.
 */

void GcodeSuite::M600() {
    const int8_t target_extruder = get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    FSM_Holder D(ClinetFSM::Load_unload, uint8_t(LoadUnloadMode::Change));
#if ENABLED(HOME_BEFORE_FILAMENT_CHANGE)
    // Don't allow filament change without homing first
    if (axes_need_homing())
        home_all_axes();
#endif

    // Initial retract before move to filament change position
    const float retract = -ABS(parser.seen('E') ? parser.value_axis_units(E_AXIS) : 0
#ifdef PAUSE_PARK_RETRACT_LENGTH
                + (PAUSE_PARK_RETRACT_LENGTH)
#endif
    );

    xyz_pos_t park_point NOZZLE_PARK_POINT;

    // Lift Z axis
    if (parser.seenval('Z'))
        park_point.z = parser.linearval('Z');

    // Move XY axes to filament change position or given position
    if (parser.seenval('X'))
        park_point.x = parser.linearval('X');
    if (parser.seenval('Y'))
        park_point.y = parser.linearval('Y');

#if HAS_HOTEND_OFFSET && NONE(DUAL_X_CARRIAGE, DELTA)
    park_point += hotend_offset[active_extruder];
#endif

    // Unload filament
    const float unload_length = -ABS(parser.seen('U') ? parser.value_axis_units(E_AXIS)
                                                      : fc_settings[active_extruder].unload_length);

    // Slow load filament
    constexpr float slow_load_length = FILAMENT_CHANGE_SLOW_LOAD_LENGTH;

    // Fast load filament
    const float fast_load_length = ABS(parser.seen('L') ? parser.value_axis_units(E_AXIS)
                                                        : fc_settings[active_extruder].load_length);

    const int beep_count = parser.intval('B',
#ifdef FILAMENT_CHANGE_ALERT_BEEPS
        FILAMENT_CHANGE_ALERT_BEEPS
#else
        -1
#endif
    );

    if (pause_print(retract, park_point, unload_length, true DXC_PASS)) {
        wait_for_confirmation(true, beep_count DXC_PASS);
        resume_print(slow_load_length, fast_load_length, ADVANCED_PAUSE_PURGE_LENGTH, beep_count DXC_PASS);
    }
}
