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

#include <config_store/store_instance.hpp>

#if ENABLED(CNC_COORDINATE_SYSTEMS)

#include "../../module/stepper.h"

//#define DEBUG_M53

/**
 * Select a coordinate system and update the workspace offset.
 * System index -1 is used to specify machine-native.
 */
bool GcodeSuite::select_coordinate_system(const int8_t _new) {
  if (active_coordinate_system == _new) return false;
  active_coordinate_system = _new;
  xyz_float_t new_offset{0};
  if (WITHIN(_new, 0, MAX_COORDINATE_SYSTEMS - 1))
    new_offset = coordinate_system[_new];

    #ifdef Z_SHIFTED_COOR_SYS //Load Z axis size to workspace offset
      if(_new == Z_SHIFTED_COOR_SYS){
        float Z_size =  config_store().axis_z_max_pos_mm.get();
        new_offset[Z_AXIS] = -Z_size;
        SERIAL_ECHOLNPAIR("Load Z axis size ", Z_size);
      }
    #endif

  LOOP_XYZ(i) {
    if (position_shift[i] != new_offset[i]) {
      position_shift[i] = new_offset[i];
      update_workspace_offset((AxisEnum)i);
    }
  }
  return true;
}

int8_t GcodeSuite::get_coordinate_system(){
  return active_coordinate_system;
}

void GcodeSuite::set_coordinate_system_offset(int8_t system, AxisEnum axis, float offset){
  coordinate_system[system][axis] = offset;
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### G53: Apply native workspace to the current move <a href="https://reprap.org/wiki/G-code#G53..59:_Coordinate_System_Select_.28CNC_specific.29">Coordinate System Select (CNC specific)</a>
 *
 * Only iX
 *
 * In CNC G-code G53 is a modifier.
 * It precedes a movement command (or other modifiers) on the same line.
 * This is the first command to use parser.chain() to make this possible.
 *
 * Marlin also uses G53 on a line by itself to go back to native space.
 *
 *#### Usage
 *
 *    G53
 */
void GcodeSuite::G53() {
  const int8_t old_system = active_coordinate_system;
  select_coordinate_system(-1);   // Always remove workspace offsets
  #ifdef DEBUG_M53
    SERIAL_ECHOLNPGM("Go to native space");
    report_current_position();
  #endif

  if (parser.chain()) {       // Command to chain?
    process_parsed_command(); // ...process the chained command
    select_coordinate_system(old_system);
    #ifdef DEBUG_M53
      SERIAL_ECHOLNPAIR("Go back to workspace ", old_system);
      report_current_position();
    #endif
  }
}

/**
 *### G54-G59.3: Select a new workspace <a href="https://reprap.org/wiki/G-code#G53..59:_Coordinate_System_Select_.28CNC_specific.29">Coordinate System Select (CNC specific)</a>
 *
 * Only iX
 *
 * A workspace is an XYZ offset to the machine native space.
 * All workspaces default to 0,0,0 at start, or with EEPROM
 * support they may be restored from a previous session.
 *
 * G92 is used to set the current workspace's offset.
 *
 *#### Usage
 *
 *    G54 - use coordinate system 1
 *    G55 - use coordinate system 2
 *    G56 - use coordinate system 3
 *    G57 - use coordinate system 4
 *    G58 - use coordinate system 5
 *    G59 - use coordinate system 6
 *    G59.1 - use coordinate system 7
 *    G59.2 - use coordinate system 8
 *    G59.3 - use coordinate system 9
 */
void G54_59(uint8_t subcode=0) {
  const int8_t _space = parser.codenum - 54 + subcode;
  if (gcode.select_coordinate_system(_space)) {
    SERIAL_ECHOLNPAIR("Select workspace ", _space);
    report_current_position();
  }
}
void GcodeSuite::G54() { G54_59(); }
void GcodeSuite::G55() { G54_59(); }
void GcodeSuite::G56() { G54_59(); }
void GcodeSuite::G57() { G54_59(); }
void GcodeSuite::G58() { G54_59(); }
void GcodeSuite::G59() { G54_59(parser.subcode); }

/** @}*/

#endif // CNC_COORDINATE_SYSTEMS
