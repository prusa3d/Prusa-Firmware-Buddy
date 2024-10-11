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

#include "../../../../inc/MarlinConfigPre.h"

#if ENABLED(PRUSA_MMU2)

    #include "../../../gcode.h"
    #include "../../../../feature/prusa/MMU2/mmu2_mk4.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M403: Set filament type for MMU <a href="https://reprap.org/wiki/G-code#M403:_Set_filament_type_.28material.29_for_particular_extruder_and_notify_the_MMU">M403: Set filament type (material) for particular extruder and notify the MMU</a>
 *
 * Only MK3.5/S, MK3.9/S and MK4/S with MMU
 *
 *#### Usage
 *
 *    M403 [ E | F ]
 *
 *#### Parameters
 *
 * - `E` - Extruder
 * - `F` - filament type
 *   - `0` - Default
 *   - `1` - Flexible
 *   - `2` - PVA
 */
void GcodeSuite::M403() {
    int8_t index = parser.intval('E', -1),
           type = parser.intval('F', -1);

    if (WITHIN(index, 0, 4) && WITHIN(type, 0, 2)) {
        MMU2::mmu2.set_filament_type(index, type);
    } else {
        SERIAL_ECHO_MSG("M403 - bad arguments.");
    }
}

/** @}*/

#endif // PRUSA_MMU2
