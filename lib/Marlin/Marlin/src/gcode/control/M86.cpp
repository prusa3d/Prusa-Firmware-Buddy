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

#include "../gcode.h"
#include "../../feature/safety_timer.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M86: Set Safety Timer expiration time <a href="https://reprap.org/wiki/G-code#M86:_Set_Safety_Timeout">M86: Set Safety Timeout</a>
 *
 *#### Usage
 *
 *     M86 [ S ]
 *
 *#### Parameters
 *
 *  - `S` - Safety timer interval [seconds]
 *
 */
void GcodeSuite::M86() {
  if (parser.seen('S'))
    safety_timer_set_interval(parser.value_millis_from_seconds());
}

/** @}*/
