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

#include <cmath>

#include <Marlin/src/gcode/gcode.h>
#include <Marlin/src/module/motion.h>
#include <common/gcode/gcode_parser.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include <common/mapi/parking.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G27: Park the nozzle <a href="https://reprap.org/wiki/G-code#G27:_Park_toolhead">G27: Park toolhead</a>
 *
 *#### Usage
 *
 * G27 [ X | Y | Z | P | R | V | A ]
 *
 *#### Parameters
 *
 * - `X` - X park position
 * - `Y` - Y park position
 * - `Z` - Z park position
 * - `P` - Z action. By default, done before the XY move when lifting Z and after the XY move when lowering Z.
 *   - `0` - (Default) Raise to at least Z above print
 *   - `1` - Absolute move to Z. This may move the nozzle down, so use with caution!
 *   - `2` - Relative move by Z.
 * - `W` - Use pre-defined park position (components can be overriden by X, Y and Z)
 *   - `0` - Park
 *   - `1` - Purge
 *   - `2` - Load
 *   - `3` - Tool park (toolchangers only)
 *
 * - `R[mm]` - Distance to retract during the parking moves
 * - `V[mm/s]` - Retraction feedrate (independent on the move feedrate)
 *
 * - A[°] - When provided, the Z move is done in parallel to the XY moves.
 *            The Z is moved with an angle A respective to the XY moves
 *            until the target Z position is reached (then only the XY move continues).
 */
void GcodeSuite::G27() {
    GCodeParser2 parser;
    if (!parser.parse_marlin_command()) {
        return;
    }

    mapi::ParkingPosition parking_position;
    mapi::ParkArgs args;

    static constexpr mapi::ParkPosition where_to_park_list[] {
        [0] = mapi::ParkPosition::park,
        [1] = mapi::ParkPosition::purge,
        [2] = mapi::ParkPosition::load,
#if HAS_TOOLCHANGER()
        [3] = mapi::ParkPosition::tool_park,
#else
        [3] = mapi::ParkPosition::park,
#endif
    };
    if (auto ix = parser.option<size_t>('W', (size_t)0, std::size(where_to_park_list) - 1)) {
        parking_position = mapi::get_parking_position(where_to_park_list[*ix], PhysicalToolIndex::currently_selected());
    }

    if (auto x = parser.option<float>('X')) {
        parking_position.x = *x;
    }

    if (auto y = parser.option<float>('Y')) {
        parking_position.y = *y;
    }

    if (auto z = parser.option<float>('Z')) {
        switch (parser.option<int>('P').value_or(0)) {

        case 0:
            parking_position.z = mapi::ParkingPosition::AtLeast { .above_print = *z };
            break;

        case 1:
            parking_position.z = *z;
            break;

        case 2:
            parking_position.z = mapi::ParkingPosition::Relative { *z };
            break;
        }
    }

    // If no axis has been specified (comparing against a position with all axes unchanged)
    if (parking_position == mapi::ParkingPosition {}) {
        parking_position = mapi::get_parking_position(mapi::ParkPosition::park);
    }

    parser.store_option_if_present('R', args.retract_distance_mm);

    float arg_feedrate;
    if (parser.store_option_if_present('V', arg_feedrate)) {
        args.retract_fr_mm_s = arg_feedrate;
    }

    if (auto a = parser.option<float>('A'); a > 0 && a < 90) {
        args.z_ramp_slope = tanf(*a * float(M_PI / 180));
    }

    mapi::home_if_needed_and_park(parking_position, args);
}

/** @}*/
