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

/**
 * unified.cpp - Unified Bed Leveling
 */

#include "../../../inc/MarlinConfig.h"

#if ENABLED(AUTO_BED_LEVELING_UBL)

#include "../../gcode.h"
#include "../../../feature/bedlevel/bedlevel.h"

#if ENABLED(EXTENSIBLE_UI)
  #include "../../../lcd/extensible_ui/ui_api.h"
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M421: Set a single Mesh Bed Leveling Z coordinate <a href="https://reprap.org/wiki/G-code#M421:_Set_a_Mesh_Bed_Leveling_Z_coordinate">M421: Set a Mesh Bed Leveling Z coordinate</a>
 *
 *#### Usage
 *
 *    M421 [ I | J | C | N | Z | Q ]
 *
 *#### Parameters
 *
 * - `I` - X index into the mesh array
 * - `J` - Y index into the mesh array
 * - `C` - Set the mesh point closest to the current nozzle position
 * - `N` - Set the mesh point to undefined
 * - `Z` - The new Z value to set
 * - `Q` - A value to add to the existing Z value
 */
void GcodeSuite::M421() {
  xy_int8_t ij = { int8_t(parser.intval('I', -1)), int8_t(parser.intval('J', -1)) };
  const bool hasI = ij.x >= 0,
             hasJ = ij.y >= 0,
             hasC = parser.seen('C'),
             hasN = parser.seen('N'),
             hasZ = parser.seen('Z'),
             hasQ = !hasZ && parser.seen('Q');

  if (hasC) ij = ubl.find_closest_mesh_point_of_type(REAL, current_position);

  if (int(hasC) + int(hasI && hasJ) != 1 || !(hasZ || hasQ || hasN))
    SERIAL_ERROR_MSG(MSG_ERR_M421_PARAMETERS);
  else if (!WITHIN(ij.x, 0, GRID_MAX_POINTS_X - 1) || !WITHIN(ij.y, 0, GRID_MAX_POINTS_Y - 1))
    SERIAL_ERROR_MSG(MSG_ERR_MESH_XY);
  else {
    float &zval = ubl.z_values[ij.x][ij.y];
    zval = hasN ? NAN : parser.value_linear_units() + (hasQ ? zval : 0);
    #if ENABLED(EXTENSIBLE_UI)
      ExtUI::onMeshUpdate(ij.x, ij.y, zval);
    #endif
  }
}

/** @}*/

#endif // AUTO_BED_LEVELING_UBL
