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
#include "../../module/motion.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M220: Set speed percentage factor <a href="https://reprap.org/wiki/G-code#M220:_Set_speed_factor_override_percentage">M220: Set speed factor override percentage</a>
 *
 *#### Usage
 *
 *    M220 [ S | B | R ]
 *
 *#### Parameters
 *
 * - `S` - Set the feed rate percentage factor
 * - `B` - Flag to back up the current factor (MMU)
 * - `R` - Flag to restore the last-saved factor (MMU)
 */
void GcodeSuite::M220() {

  static int16_t backup_feedrate_percentage = 100;
  const int16_t now_feedrate_perc = feedrate_percentage;
  if (parser.boolval('R')) feedrate_percentage = backup_feedrate_percentage;
  if (parser.boolval('B')) backup_feedrate_percentage = now_feedrate_perc;
  if (parser.seenval('S')) feedrate_percentage = parser.value_int();

}

/** @}*/
