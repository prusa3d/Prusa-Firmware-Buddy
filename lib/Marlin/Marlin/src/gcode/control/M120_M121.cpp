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
#include "../../module/endstops.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M120: Enable endstops <a href="https://reprap.org/wiki/G-code#M120:_Enable_endstop_detection">M120: Enable endstop detection</a>
 *
 * and set non-homing endstop state to "enabled"
 *
 *#### Usage
 *
 *    M120
 *
 */
void GcodeSuite::M120() { endstops.enable_globally(true); }

/**
 *### M121: Disable endstops <a href="https://reprap.org/wiki/G-code#M121:_Disable_endstop_detection">M121: Disable endstop detection</a>
 *
 * and set non-homing endstop state to "disabled"
 *
 *#### Usage
 *
 *    M121
 *
 */
void GcodeSuite::M121() { endstops.enable_globally(false); }

/** @}*/
