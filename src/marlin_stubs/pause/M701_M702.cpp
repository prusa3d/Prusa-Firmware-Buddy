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

#include "../../../lib/Marlin/Marlin/src/inc/MarlinConfigPre.h"

// clang-format off
#if (!ENABLED(FILAMENT_LOAD_UNLOAD_GCODES)) || \
    EXTRUDERS > 1 || \
    HAS_LCD_MENU || \
    ENABLED(PRUSA_MMU2) || \
    ENABLED(MIXING_EXTRUDER) || \
    ENABLED(NO_MOTION_BEFORE_HOMING)
    #error unsupported
#endif
// clang-format on

#include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "../../../lib/Marlin/Marlin/src/Marlin.h"
#include "../../../lib/Marlin/Marlin/src/module/motion.h"
#include "../../../lib/Marlin/Marlin/src/module/temperature.h"
#include "marlin_server.hpp"
#include "pause_stubbed.hpp"
#include "filament.h"

#define DO_NOT_RESTORE_Z_AXIS
#define Z_AXIS_LOAD_POS   40
#define Z_AXIS_UNLOAD_POS 20

typedef void (*load_unload_fnc)(const int8_t target_extruder);

/**
 * Shared code for load/unload filament
 */
static void load_unload(LoadUnloadMode type, load_unload_fnc f_load_unload, uint32_t min_Z_pos) {
    const int8_t target_extruder = GcodeSuite::get_target_extruder_from_command();
    if (target_extruder < 0)
        return;

    FSM_Holder D(ClientFSM::Load_unload, uint8_t(type));
    // Z axis lift
    if (parser.seenval('Z'))
        min_Z_pos = parser.linearval('Z');

    // Lift Z axis
    if (min_Z_pos > 0) {
        const float target_Z = _MIN(_MAX(current_position.z, min_Z_pos), Z_MAX_POS);
        Notifier_POS_Z N(ClientFSM::Load_unload, GetPhaseIndex(PhasesLoadUnload::Parking), current_position.z, target_Z, 0, 100);
        do_blocking_move_to_z(target_Z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
    }
    // Load/Unload filament
    f_load_unload(target_extruder);
#ifndef DO_NOT_RESTORE_Z_AXIS
    // Restore Z axis
    if (min_Z_pos > 0) {
        const float target_Z = _MAX(current_position.z - min_Z_pos, 0);
        Notifier_POS_Z N(ClientFSM::Load_unload, GetPhaseIndex(PhasesLoadUnload::Unparking), current_position.z, target_Z, 0, 100);
        do_blocking_move_to_z(target_Z, feedRate_t(NOZZLE_PARK_Z_FEEDRATE));
    }
#endif
}

/**
 * Load filament special code
 * filament type must be last parameter
 * type name cannot contain 'Z'
 * todo rewrite
 */
static void load(const int8_t target_extruder) {
    filament_to_load = DEFAULT_FILAMENT;
    const char *text_begin = 0;
    if (parser.seen('S')) {
        text_begin = strchr(parser.string_arg, '"');
        if (text_begin) {
            ++text_begin; //move pointer from '"' to first letter
            const char *text_end = strchr(text_begin, '"');
            if (text_end) {
                FILAMENT_t filament = get_filament_from_string(text_begin, text_end - text_begin);
                if (filament != FILAMENT_NONE) {
                    filament_to_load = filament;
                }
            }
        }
    }

    const float fast_load_length = ABS((parser.seen('L') && (!text_begin || strchr(parser.string_arg, 'L') < text_begin)) ? parser.value_axis_units(E_AXIS)
                                                                                                                          : pause.GetLoadLength());
    constexpr float purge_length = ADVANCED_PAUSE_PURGE_LENGTH;
    const float slow_load_length = fast_load_length > 0 ? FILAMENT_CHANGE_SLOW_LOAD_LENGTH : 0;

    pause.FilamentLoad(slow_load_length, fast_load_length, purge_length);
}

/**
 * Unload filament special code
 */
static void unload(const int8_t target_extruder) {
    const float unload_length = -ABS(parser.seen('U') ? parser.value_axis_units(E_AXIS) : pause.GetUnloadLength());

    pause.FilamentUnload(unload_length);
}

/**
 * M701: Load filament
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, current extruder if omitted.
 *  Z<distance> - Move the Z axis by this distance
 *  L<distance> - Extrude distance for insertion (positive value) (manual reload)
 *  S"Filament" - save filament by name, for example S"PLA". RepRap compatible.
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M701() {
    load_unload(LoadUnloadMode::Load, load, Z_AXIS_LOAD_POS);
}

/**
 * M702: Unload filament
 *
 *  T<extruder> - Extruder number. Required for mixing extruder.
 *                For non-mixing, if omitted, current extruder
 *                (or ALL extruders with FILAMENT_UNLOAD_ALL_EXTRUDERS).
 *  Z<distance> - Move the Z axis by this distance
 *  U<distance> - Retract distance for removal (manual reload)
 *
 *  Default values are used for omitted arguments.
 */
void GcodeSuite::M702() {
    load_unload(LoadUnloadMode::Unload, unload, Z_AXIS_UNLOAD_POS);
}
