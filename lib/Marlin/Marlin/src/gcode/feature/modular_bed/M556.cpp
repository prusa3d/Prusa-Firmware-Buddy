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

#include "../../../inc/MarlinConfig.h"
#include "../../gcode.h"
#include "../../../module/temperature.h"

#if ENABLED(MODULAR_HEATBED)

#include "../../../module/modular_heatbed.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M556: Override modular bedled <a href="https://reprap.org/wiki/G-code#M556:_Override_modular_bedled">M556: Override modular bedled</a>
 *
 * Only iX and XL
 *
 *#### Usage
 *
 *    M556 [ X | Y | I | A | D]
 *
 *#### Parameters
 *
 * - `X` - Coordinate on X axis
 * - `Y` - Coordinate on Y axis
 * - `I` - Betlet number
 * - `A` - activate selected betlet
 * - `D` - deactivate selected betlet
 */
void GcodeSuite::M556() {

    // by default set all bedlets
    uint16_t affected_beds = std::numeric_limits<uint16_t>::max();

    // set bedlet based on XY coordinates
    if (parser.seen('X') && parser.seen('Y')) {
        uint8_t x = parser.byteval('X');
        uint8_t y = parser.byteval('Y');

        affected_beds = 1 << advanced_modular_bed->idx(x, y);

    } else if (parser.seen("I")) {
        // set bedlet based on its index
        affected_beds = 1 << (parser.byteval('I') - 1);
    }

    // now activate/deactivate selected bedlet
    uint16_t active = Temperature::getEnabledBedletMask();
    if (parser.seen("A")) {
        active |= affected_beds;
    } else if (parser.seen("D")) {
        active &= ~affected_beds;
    } else {
        SERIAL_ECHOPGM("No operation specified");
        return;
    }

    Temperature::setEnabledBedletMask(active);
}

/** @}*/

#endif
