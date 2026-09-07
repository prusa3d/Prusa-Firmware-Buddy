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
#pragma once

#include "../inc/MarlinConfigPre.h"
#include "../core/types.h"
#include <tool_index.hpp>

#if EXTRUDERS > 1

  typedef struct {
    float z_raise;
  } toolchange_settings_t;

  extern toolchange_settings_t toolchange_settings;

#endif

#if ENABLED(SINGLENOZZLE)
  extern uint16_t singlenozzle_temp[EXTRUDERS];
  extern uint8_t singlenozzle_fan_speed[EXTRUDERS];
#endif

enum class tool_return_t {
  no_return, // lift and/or retract as needed, but don't return to any position after toolchange
  to_current, // return to the current position
  to_destination, // return to destination
  dock_backoff, // INDX only: like no_return, but after docking (NoTool) backs off from the dock in +Y to clear it for access
};

/// Enum that configures what kind of Z lift will be done during toolchange
enum class tool_change_lift_t {
  no_lift,       // will not do any Z lift
  mbl_only_lift, // will lift by with maximal MBL
  full_lift,     // do full lift rutine (by M217 Z value & MBL)

  _last_item = full_lift,
};


/**
 * Perform a tool-change which may result in moving the previous tool out of the way and the new
 * tool into place. The return move is controlled by return_type.
 * Returns true on success, false if the toolchanger reported a failure.
 */
bool tool_change(const std::variant<VirtualToolIndex, PhysicalToolIndex, NoTool> new_tool,
                 tool_return_t return_type=tool_return_t::to_current,
                 tool_change_lift_t z_lift = tool_change_lift_t::full_lift,
                 bool z_return = true);
