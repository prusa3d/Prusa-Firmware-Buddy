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

#if ENABLED(NOZZLE_PARK_FEATURE)
    #include "G27.hpp"

    #include <Marlin/src/gcode/gcode.h>
    #include <Marlin/src/libs/nozzle.h>
    #include <Marlin/src/module/motion.h>
    #include <common/gcode/gcode_parser.hpp>
    #include <common/filament_sensors_handler.hpp>
    #include <common/mapi/parking.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G27: Park the nozzle <a href="https://reprap.org/wiki/G-code#G27:_Park_toolhead">G27: Park toolhead</a>
 *
 *#### Usage
 *
 * G27 [ X | Y | Z | P ]
 *
 *#### Parameters
 *
 * - `X` - X park position
 * - `Y` - Y park position
 * - `Z` - Z park position
 * - `P` - Z action
 *   - `0` - (Default) Relative raise by NOZZLE_PARK_Z_RAISE_MIN before XY parking
 *   - `1` - Absolute move to NOZZLE_PARK_POINT.z before XY parking. This may move the nozzle down, so use with caution!
 *   - `2` - Relative raise by NOZZLE_PARK_POINT.z before XY parking.
 * - `W` - Use pre-defined park position. Usable only if X, Y and Z are not present as they override pre-defined behaviour.
 *   - `0` - Park
 *   - `1` - Purge
 *   - `2` - Load
 */
void GcodeSuite::G27() {
    GCodeParser2 parser;
    if (!parser.parse_marlin_command()) {
        return;
    }

    G27Params params;
    parser.store_option('P', params.z_action, mapi::ZAction::_cnt);
    parser.store_option('W', params.where_to_park, mapi::ParkPosition::_cnt);

    parser.store_option('X', params.park_position.x);
    parser.store_option('Y', params.park_position.y);
    parser.store_option('Z', params.park_position.z);

    G27_no_parser(params);
}

void G27_no_parser(const G27Params &params) {
    xyz_pos_t park_position { mapi::park_positions[params.where_to_park] };
    xyz_bool_t do_axis = { { { true, true, true } } };

    // If any park position was given, move only specified axes
    if (!(isnan(params.park_position.x) && isnan(params.park_position.y) && isnan(params.park_position.z))) {
        for (uint8_t i = 0; i < 3; i++) {
            do_axis.pos[i] = !isnan(params.park_position.pos[i]);
            park_position.pos[i] = do_axis.pos[i] ? params.park_position.pos[i] : current_position.pos[i];
        }
    }

    // If not homed and only Z clearance is requested, od just that, otherwise home and then park.
    if (axes_need_homing(X_AXIS | Y_AXIS | Z_AXIS)) {
        if (do_axis == xyz_bool_t { { { false, false, true } } } && params.z_action == mapi::ZAction::move_to_at_least) {
            // Only Z axis is given in P=0 mode, do Z clearance
            do_z_clearance(park_position.z);
            return;
        }
    }

    mapi::park_move_with_conditional_home(park_position, params.z_action);
}

/** @}*/

#endif // NOZZLE_PARK_FEATURE
