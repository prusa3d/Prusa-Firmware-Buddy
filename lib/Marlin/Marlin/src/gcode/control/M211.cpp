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

#include "../../inc/MarlinConfigPre.h"

#if HAS_SOFTWARE_ENDSTOPS

#include "../gcode.h"
#include "../../module/motion.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 * ### M211: Enable, Disable, and/or Report software endstops <a href="https://reprap.org/wiki/G-code#M211:_Disable.2FEnable_software_endstops">M211: Disable/Enable software endstops</a>
 *
 *#### Usage
 *
 *    M211 [ S ]
 *
 *#### Parameters
 *
 * - `S` - enable = 1, disable = 0
 *
 * Without parameters prints the current software endstops
 */
void GcodeSuite::M211() {
  const xyz_pos_t l_soft_min = soft_endstop.min.asLogical(),
                  l_soft_max = soft_endstop.max.asLogical();
  SERIAL_ECHO_START();
  SERIAL_ECHOPGM(MSG_SOFT_ENDSTOPS);
  if (parser.seen('S')) soft_endstops_enabled = parser.value_bool();
  serialprint_onoff(soft_endstops_enabled);
  print_xyz(l_soft_min, PSTR(MSG_SOFT_MIN), PSTR(" "));
  print_xyz(l_soft_max, PSTR(MSG_SOFT_MAX));
}

/** @}*/

#endif
