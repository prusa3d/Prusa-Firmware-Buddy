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

    #include "../../../lib/Marlin/Marlin/src/gcode/gcode.h"
    #include "../../../lib/Marlin/Marlin/src/libs/nozzle.h"
    #include "../../../lib/Marlin/Marlin/src/module/motion.h"

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

    const uint16_t P = parser.ushortval('P');
    const bool doX = parser.seen('X');
    const bool doY = parser.seen('Y');
    const bool doZ = parser.seen('Z');

    constexpr xyz_pos_t default_park_pos = { { XYZ_NOZZLE_PARK_POINT } };
    xyz_pos_t park_pos;

    if (doX || doY || doZ) {
        // Any one of XYZ was given, move only named axes
        park_pos = current_position;

        // Axis letter without number moves to default park position
        if (doX) {
            park_pos.x = parser.floatval('X', default_park_pos.x);
        }
        if (doY) {
            park_pos.y = parser.floatval('Y', default_park_pos.y);
        }
        if (doZ) {
            park_pos.z = parser.floatval('Z', default_park_pos.z);
        }
    } else {
        // No axis was given, move all axes to default
        park_pos = default_park_pos;
    }

    // If not homed, allow only Z clearance
    if (axes_need_homing()) {
        if (!doX && !doY && doZ && P == 0) {
            // Only Z axis is given in P=0 mode, do Z clearance
            do_z_clearance(park_pos.z);
        } else {
            // Don't allow nozzle parking without homing first
            axis_unhomed_error();
            return;
        }
    } else {
        // Regular park
        nozzle.park(P, park_pos);
    }
}

/** @}*/

#endif // NOZZLE_PARK_FEATURE
