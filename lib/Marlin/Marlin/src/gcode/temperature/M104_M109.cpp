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

#include "M104_M109.hpp"

#include "../gcode.h"
#include "../../module/temperature.h"
#include "../../module/motion.h"
#include "../../module/planner.h"
#include "../../lcd/ultralcd.h"
#include <utils/variant_utils.hpp>
#include "../../Marlin.h"

#if ENABLED(PRINTJOB_TIMER_AUTOSTART)
  #include "../../module/printcounter.h"
#endif

#if ENABLED(SINGLENOZZLE)
  #include "../../module/tool_change.h"
#endif

#include "marlin_server.hpp"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M104: Set hot end temperature <a href="https://reprap.org/wiki/G-code#M104:_Set_Extruder_Temperature">M104: Set Extruder Temperature</a>
 *
 *#### Usage
 *
 *    M104 [ S | D | T ]
 *
 * #### Parameters
 *
 * - `S` - Set temperature
 * - `D` - Display temperature (otherwise actual temp will be displayed)
 * - `T` - Tool
 */
void GcodeSuite::M104() {

  if (DEBUGGING(DRYRUN)) return;

  const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(get_target_physical_from_command());
  if (!tool.has_value()) return;

  if (parser.seenval('S')) {
    const int16_t temp = static_cast<int16_t>(parser.value_celsius());
    #if ENABLED(SINGLENOZZLE)
      singlenozzle_temp[tool->to_raw()] = temp;
      if (!stdext::holds_value(PhysicalToolIndex::currently_selected(), *tool)) return;
    #endif
    thermalManager.setTargetHotend(temp, *tool);
  }

  if(parser.seenval('D')) {
    // Override display_temp set by setTargetHotend
    // This is a legit use
    marlin_server::call_manually::set_temp_to_display( parser.value_celsius(), *tool);
  }
}

/**
 *### M109: Set hot end temperature and wait <a href="https://reprap.org/wiki/G-code#M109:_Set_Extruder_Temperature_and_Wait">M109: Set Extruder Temperature and Wait</a>
 *
 *#### Usage
 *
 *    M109 [ S | R | C | D | F | T ]
 *
 * #### Parameters
 *
 * - `S` - Wait for extruder(s) to reach temperature. Waits only when heating.
 * - `R` - Wait for extruder(s) to reach temperature. Waits when heating and cooling.
 * - `C` - Wait until the hotend reaches this temperature without changing the target.
 *         Waits only when heating, at most as long as a regular target wait.
 * - `D` - Display temperature (otherwise actual temp will be displayed)
 * - `F` - Autotemp flag.
 * - `T` - Tool
 */
void GcodeSuite::M109() {
  const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(get_target_physical_from_command());
  if (!tool.has_value()) return;
  M109Flags flags {
    .target_temp = static_cast<int16_t>(
      parser.seenval('S') ? parser.value_celsius() :
      parser.seenval('R') ? parser.value_celsius() : 0
    ),
    .wait_heat = parser.seenval('S'),
    .wait_heat_or_cool = parser.seenval('R'),
    .autotemp = parser.boolval('F'),
    .display_temp = parser.seenval('D') ? std::optional<float>(parser.value_celsius()) : std::nullopt,
    .early_return_temperature = parser.seenval('C') ? std::optional<float>(parser.value_celsius()) : std::nullopt,
  };
  M109_no_parser(*tool, flags);
}

void M109_no_parser(PhysicalToolIndex tool, const M109Flags& flags) {

  if (DEBUGGING(DRYRUN)) return;

  const bool no_wait_for_cooling = flags.wait_heat && !flags.wait_heat_or_cool;
  const bool set_temp = no_wait_for_cooling || flags.wait_heat_or_cool;
  if (set_temp) {
    const int16_t temp = flags.target_temp;
    #if ENABLED(SINGLENOZZLE)
      singlenozzle_temp[tool.to_raw()] = temp;
      if (!stdext::holds_value(PhysicalToolIndex::currently_selected(), tool)) return;
    #endif
    thermalManager.setTargetHotend(temp, tool);
    if(flags.display_temp.has_value()) {
      // Override display_temp set by setTargetHotend
      // This is a legit use
      marlin_server::call_manually::set_temp_to_display(*flags.display_temp, tool);
    }
  }

  if (set_temp || flags.early_return_temperature.has_value()) {
    (void)thermalManager.wait_for_hotend(tool, { .no_wait_for_cooling = no_wait_for_cooling, .fan_cooling = flags.autotemp, .early_return_temperature = flags.early_return_temperature });
  }

  return;
}
/** @}*/
