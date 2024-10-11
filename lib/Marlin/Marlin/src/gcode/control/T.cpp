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
#include "../../module/tool_change.h"
#include "bsod.h"
#if ENABLED(PRUSA_TOOL_MAPPING)
  #include "module/prusa/tool_mapper.hpp"
#endif

#if ENABLED(DEBUG_LEVELING_FEATURE) || EXTRUDERS > 1
  #include "../../module/motion.h"
#endif

#if ENABLED(PRUSA_MMU2)
  #include "../../feature/prusa/MMU2/mmu2_mk4.h"
#endif

#if ENABLED(PRUSA_TOOLCHANGER)
  #include "../../module/prusa/toolchanger.h"
#endif

#define DEBUG_OUT ENABLED(DEBUG_LEVELING_FEATURE)
#include "../../core/debug_out.h"

/** \addtogroup G-Codes
 * @{
 */

/**
 *### T: Select Tool <a href="https://reprap.org/wiki/G-code#T:_Select_Tool">T: Select Tool</a>
 *
 *#### Usage
 *
 *    T [ F | S | M | L | D ]
 *
 *#### Parameters
 *
 * - `T<value>` - Tool
 *   - `<number>` - Tool number starting with 0
 * - `F` - Set the movement feedrate
 * - `S` - Don't move the tool in XY after change
 * - `M` - Use tool mapping or not (default is yes)
 * - `L` - Z Lift settings
 *   - `0` - no lift
 *   - `1` - lift by max MBL diff
 *   - `2` - full lift(default)
 * - `D` - Z lift return settings
 *   - `0` - Do not return in Z after lift
 *   - `1` - Normal return
 *
 * For PRUSA_MMU2:
 * - `T[n]` - Gcode to extrude at least 38.10 mm at feedrate 19.02 mm/s must follow immediately to load to extruder wheels.
 * - `T?` - Gcode to extrude shouldn't have to follow. Load to extruder wheels is done automatically.
 * - `Tx` - Same as T?, but nozzle doesn't have to be preheated. Tc requires a preheated nozzle to finish filament load.
 * - `Tc` - Load to nozzle after filament was prepared by Tc and nozzle is already heated.
 */
void GcodeSuite::T(uint8_t tool_index) {

#if ENABLED(PRUSA_TOOL_MAPPING)
  const bool map = !parser.seen('M') || parser.boolval('M', true);
  if (map) {
    tool_index = tool_mapper.to_physical(tool_index);
    if (tool_index == tool_mapper.NO_TOOL_MAPPED) {
      raise_redscreen(ErrCode::ERR_UNDEF, "Toolchange to tool that is disabled by tool mapping", "PrusaToolChanger");
    }
  }
#endif

  if (DEBUGGING(LEVELING)) {
    DEBUG_ECHOLNPAIR(">>> T(", tool_index, ")");
    DEBUG_POS("BEFORE", current_position);
  }

  #if ENABLED(PRUSA_MMU2)
    if (parser.string_arg) {
      // @@TODO MMU2::mmu2.tool_change(parser.string_arg);   // Special commands T?/Tx/Tc
      return;
    }
  #endif

  #if EXTRUDERS < 2

    tool_change(tool_index);

  #else

    get_destination_from_command(); // sets destination = current position or user request

    // by default, Tx goes to specified destination or current position, unless following:
    tool_return_t return_type = tool_return_t::to_destination;

    // S1 was provided => do not return
    int move_type = !parser.seen('S') ? 0 : parser.intval('S', 1);
    if (move_type >= 1) return_type = tool_return_t::no_return;
    #if ENABLED(PRUSA_TOOLCHANGER)
    // toolchange to or from no tool is no_return, but if user provided X, Y or Z, return to that position
    if (((tool_index == PrusaToolChanger::MARLIN_NO_TOOL_PICKED || active_extruder == PrusaToolChanger::MARLIN_NO_TOOL_PICKED)) && destination == current_position) {
    return_type = tool_return_t::no_return;
    }
    #endif

    auto z_lift = static_cast<tool_change_lift_t>(parser.byteval('L', static_cast<uint8_t>(tool_change_lift_t::full_lift)));
    if (z_lift > tool_change_lift_t::_last_item) z_lift = tool_change_lift_t::full_lift; // invalid input, use full_lift
    bool z_down = parser.byteval('D', 1);

    tool_change(tool_index, return_type, z_lift, z_down);

  #endif

  if (DEBUGGING(LEVELING)) {
    DEBUG_POS("AFTER", current_position);
    DEBUG_ECHOLNPGM("<<< T()");
  }
}

/** @}*/
