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

//#if ENABLED(LIN_ADVANCE)

#include "../../gcode.h"
#include "../../../module/planner.h"
#include "../../../feature/pressure_advance/pressure_advance_config.hpp"

#if ENABLED(EXTRA_LIN_ADVANCE_K)
  float saved_extruder_advance_K[EXTRUDERS];
  uint8_t lin_adv_slot = 0;
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M900: Get/Set Linear Advance K-factor <a href="https://reprap.org/wiki/G-code#M900:_Set_Linear_Advance_Scaling_Factors">M900: Set Linear Advance Scaling Factors</a>
 *
 *#### Usage
 *
 *    M900 [ T | K | L | S ]
 *
 *#### Parameters
 *
 * - `T` - Tool
 * - `K` - Set current advance K factor (Slot 0)
 * - `L` - Set secondary advance K factor (Slot 1)
 * - `S` - Activate slot 0 or 1
 *
 * With no parameters report Linear Advance K-factor(s)
 */
void GcodeSuite::M900() {

  #if EXTRUDERS < 2
    constexpr uint8_t tool_index = 0;
  #else
    const uint8_t tool_index = parser.intval('T', active_extruder);
    if (tool_index >= EXTRUDERS) {
      SERIAL_ECHOLNPGM("?T value out of range.");
      return;
    }
  #endif
  static_cast<void>(tool_index); // TODO support multiple extruders

  #if ENABLED(EXTRA_LIN_ADVANCE_K)

    bool ext_slot = TEST(lin_adv_slot, tool_index);

    if (parser.seenval('S')) {
      const bool slot = parser.value_bool();
      if (ext_slot != slot) {
        ext_slot = slot;
        SET_BIT_TO(lin_adv_slot, tool_index, slot);
        planner.synchronize();
        const float temp = planner.extruder_advance_K[tool_index];
        planner.extruder_advance_K[tool_index] = saved_extruder_advance_K[tool_index];
        saved_extruder_advance_K[tool_index] = temp;
      }
    }

    if (parser.seenval('K')) {
      const float newK = parser.value_float();
      if (WITHIN(newK, 0, 10)) {
        if (ext_slot)
          saved_extruder_advance_K[tool_index] = newK;
        else {
          planner.synchronize();
          planner.extruder_advance_K[tool_index] = newK;
        }
      }
      else
        SERIAL_ECHOLNPGM("?K value out of range (0-10).");
    }

    if (parser.seenval('L')) {
      const float newL = parser.value_float();
      if (WITHIN(newL, 0, 10)) {
        if (!ext_slot)
          saved_extruder_advance_K[tool_index] = newL;
        else {
          planner.synchronize();
          planner.extruder_advance_K[tool_index] = newL;
        }
      }
      else
        SERIAL_ECHOLNPGM("?L value out of range (0-10).");
    }

    if (!parser.seen_any()) {
      #if EXTRUDERS < 2
        SERIAL_ECHOLNPAIR("Advance S", ext_slot, " K", planner.extruder_advance_K[0],
                          "(Slot ", 1 - ext_slot, " K", saved_extruder_advance_K[0], ")");
      #else
        LOOP_L_N(i, EXTRUDERS) {
          const int slot = (int)TEST(lin_adv_slot, i);
          SERIAL_ECHOLNPAIR("Advance T", int(i), " S", slot, " K", planner.extruder_advance_K[i],
                            "(Slot ", 1 - slot, " K", saved_extruder_advance_K[i], ")");
          SERIAL_EOL();
        }
      #endif
    }

  #else

    if (parser.seenval('K')) {
      const float newK = parser.value_float();

      #if ENABLED(GCODE_COMPATIBILITY_MK3)
        if (gcode.gcode_compatibility_mode == GcodeSuite::GcodeCompatibilityMode::MK3 && newK >= 3) {
          // Higher K values on MK3 mean LA version 1.0 => we don't support those
          // Lower values on MK3 are very similar to MK4's, so we can use them and expect OK results.
          return;
        }
      #endif

      if (WITHIN(newK, 0, 10)) {
        const pressure_advance::Config default_config = pressure_advance::Config();
        M572_internal(newK, default_config.smooth_time);
      }
      else
        SERIAL_ECHOLNPGM("?K value out of range (0-10).");
    }
    else {
      SERIAL_ECHO_START();
      #if EXTRUDERS < 2
        SERIAL_ECHOLNPAIR("Advance K=", pressure_advance::get_axis_e_config().pressure_advance);
      #else
        SERIAL_ECHOPGM("Advance K");
        LOOP_L_N(i, EXTRUDERS) {
          SERIAL_CHAR(' '); SERIAL_ECHO(int(i));
          SERIAL_CHAR('='); SERIAL_ECHO(pressure_advance::get_axis_e_config().pressure_advance);
        }
        SERIAL_EOL();
      #endif
    }

  #endif
}

/** @}*/

//#endif // LIN_ADVANCE
