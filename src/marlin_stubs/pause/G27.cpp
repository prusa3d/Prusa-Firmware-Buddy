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

/** \addtogroup G-Codes
 * @{
 */

/**
 * G27: Park the nozzle
 *
 * ## Parameters
 *
 * - `X` - Park nozzle on X axis
 * - `Y` - Park nozzle on X axis
 * - `Z` - Park nozzle on X axis
 * - `P` - [value] Z action
 */
void GcodeSuite::G27() {
    GCodeParser2 parser { GCodeParser2::from_marlin_parser };
    G27Params params;
    parser.store_option('P', params.z_action);

    params.do_x = parser.store_option('X', params.park_position.x);
    params.do_y = parser.store_option('Y', params.park_position.y);
    params.do_z = parser.store_option('Z', params.park_position.z);

    G27_no_parser(params);
}

void G27_no_parser(const G27Params &params) {
    xyz_pos_t park_position { params.park_position };

    static xyz_pos_t default_park_pos { { XYZ_NOZZLE_PARK_POINT } };
    if (!params.do_x && !params.do_y && !params.do_z) {
        park_position = default_park_pos;
    }

    // If not homed and only Z clearance is requested, od just that, otherwise home and then park.
    if (axes_need_homing()) {
        if (!params.do_x && !params.do_y && params.do_z && params.z_action == 0) {
            // Only Z axis is given in P=0 mode, do Z clearance
            do_z_clearance(params.park_position.z);
            return;
        } else {
            // Don't allow nozzle parking without homing first
            axis_unhomed_error();
            return;
        }
    } // Regular park
    nozzle.park(params.z_action, park_position);
}

/** @}*/

#endif // NOZZLE_PARK_FEATURE
