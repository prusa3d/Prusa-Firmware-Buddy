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

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M82: Set E codes absolute (default) <a href="https://reprap.org/wiki/G-code#M82:_Set_extruder_to_absolute_mode">M82: Set extruder to absolute mode</a>
 *
 *#### Usage
 *
 *    M82
 *
 */
void GcodeSuite::M82() { set_e_absolute(); }

/**
 *### M83: Set E codes relative while in Absolute Coordinates (G90) mode <a href="https://reprap.org/wiki/G-code#M83:_Set_extruder_to_relative_mode">M83: Set extruder to relative mode</a>
 *
 *#### Usage
 *
 *    M83
 *
 */
void GcodeSuite::M83() { set_e_relative(); }

/** @}*/
