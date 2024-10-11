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
#include "../../../feature/print_area.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M555: Set print area <a href="https://reprap.org/wiki/G-code#M555:_Set_Bounding_Box">M555: Set Bounding Box</a>
 *
 * Tells the printer about the size of the model.
 * The XL uses this information to save power for smaller models by only heating the area under it.
 *
 *#### Usage
 *
 *    M555 [ X | Y | W | H ]
 *
 *
 *#### Parameters
 *
 * - `X` - Set print area X coordinate
 * - `Y` - Set print area Y coordinate
 * - `W` - Set print width
 * - `H` - Set print height
 *
 *#### Examples
 *
 *    M555 X10 Y20 W30 H40 ; Defines the print area to `X` =10 and `Y` =20 with a `W`idth of 30 and `H`eight of 40.
 *                           X 10 to X 40
 *                           Y 20 to Y 60
 */
void GcodeSuite::M555() {
  auto area = PrintArea::rect_t::max();

  if (parser.seen('X')) {
    area.a.x = parser.floatval('X');
    if (parser.seen('W'))
      area.b.x = area.a.x + parser.floatval('W');
  }

  if (parser.seen('Y')) {
    area.a.y = parser.floatval('Y');
    if (parser.seen('H'))
      area.b.y = area.a.y + parser.floatval('H');
  }

  print_area.set_bounding_rect(area);
}

/** @}*/
