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

static constexpr EnumArray<G27Params::ParkPosition, xyz_pos_t, G27Params::ParkPosition::_cnt> park_positions {
    { G27Params::ParkPosition::park, xyz_pos_t({ { XYZ_NOZZLE_PARK_POINT } }) },
    #if HAS_WASTEBIN()
        { G27Params::ParkPosition::purge, xyz_pos_t({ X_WASTEBIN_POINT, Y_WASTEBIN_POINT, Z_NOZZLE_PARK_POINT }) },
    #else
        { G27Params::ParkPosition::purge, xyz_pos_t({ { XYZ_NOZZLE_PARK_POINT } }) },
    #endif
        { G27Params::ParkPosition::load, xyz_pos_t({ X_AXIS_LOAD_POS, Y_AXIS_LOAD_POS, Z_NOZZLE_PARK_POINT }) },
};

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
 * - 'W' - [0-2] Use pre-defined park position. Usable only if X, Y and Z are not present as they override pre-defined behaviour.
 */
void GcodeSuite::G27() {
    GCodeParser2 parser { GCodeParser2::from_marlin_parser };
    G27Params params;
    parser.store_option('P', params.z_action);

    params.where_to_park = G27Params::ParkPosition { parser.option<uint8_t>('W').transform([](uint8_t val) -> uint8_t { return val < ftrstd::to_underlying(G27Params::ParkPosition::_cnt) ? val : 0; }).value_or(0) };

    params.do_x = parser.store_option('X', params.park_position.x).has_value();
    params.do_y = parser.store_option('Y', params.park_position.y).has_value();
    params.do_z = parser.store_option('Z', params.park_position.z).has_value();

    G27_no_parser(params);
}

void G27_no_parser(const G27Params &params) {
    xyz_pos_t park_position { params.park_position };

    if (!params.do_x && !params.do_y && !params.do_z) {
        park_position = park_positions[params.where_to_park];
    }

    // If not homed and only Z clearance is requested, od just that, otherwise home and then park.
    if (axes_need_homing()) {
        if (!params.do_x && !params.do_y && params.do_z && params.z_action == 0) {
            // Only Z axis is given in P=0 mode, do Z clearance
            do_z_clearance(park_position.z);
            return;
        } else {
            GcodeSuite::G28_no_parser(true, true, 3, false, params.do_x, params.do_y, params.do_z);
        }
    } // Regular park
    nozzle.park(params.z_action, park_position);
}

/** @}*/

#endif // NOZZLE_PARK_FEATURE
