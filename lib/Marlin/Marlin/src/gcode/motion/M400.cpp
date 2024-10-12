/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../gcode.h"
#include "../../module/planner.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M400: Finish all moves <a href="https://reprap.org/wiki/G-code#M400:_Wait_for_current_moves_to_finish">M400: Wait for current moves to finish</a>
 *
 *This command causes G-code processing to pause and wait in a loop until all moves in the planner are completed.
 *
 *#### Usage
 *
 *    M400

 */
void GcodeSuite::M400() {

  planner.synchronize();

}

/** @}*/
