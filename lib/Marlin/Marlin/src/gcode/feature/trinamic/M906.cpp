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

#if HAS_TRINAMIC

#include "../../gcode.h"
#include "../../../feature/tmc_util.h"
#include "../../../module/stepper/indirection.h"
#include <utils/variant_utils.hpp>
#include <option/has_indx.h>

#include <option/has_motor_current_profiles.h>
#if HAS_MOTOR_CURRENT_PROFILES()
    #include <feature/motor_current_profile/motor_current_profile.hpp>
#endif

/** \addtogroup G-Codes
 * @{
 */

/**
 *# to ## M906: Get/Set motor current in milliamps <a href="https://reprap.org/wiki/G-code#M906:_Set_motor_currents">M906: Set motor currents</a>
 *
 *#### Usage
 *
 *    M906 [ X | Y | Z | E | I | T | P ]
 *
 *#### Parameters
 *
 * -` X` - Set mA current for X driver(s)
 * -` Y` - Set mA current for Y driver(s)
 * -` Z` - Set mA current for Z driver(s)
 * -` E` - Set mA current for E driver(s)
 *
 * -` I` - Axis sub-index
 *   - `0` - or Omit for X, Y, Z
 *   - `1` - for Z2
 *   - `2` - for Z3
 * -` T` - Extruder index (Zero-based. Omit for E0 only.)
 * -` P` - Motor current profile (requires HAS_MOTOR_CURRENT_PROFILES). When present, other axis parameters are ignored.
 *   - `0` - Firmware defaults
 *   - `1` - Increased E current
 *   - `2` - Decreased E current for flexible filaments
 *
 * With no parameters report driver currents.
 */
void GcodeSuite::M906() {
  #define TMC_SAY_CURRENT(Q) tmc_print_current(stepper##Q)
  #define TMC_SET_CURRENT(Q) stepper##Q.rms_current(value)

#if HAS_MOTOR_CURRENT_PROFILES()
  if (parser.seen('P')) {
    const auto val = parser.byteval('P');
    if (val >= static_cast<uint8_t>(buddy::StandardMotorCurrentProfile::_count)) {
      SERIAL_ECHOLNPGM("Invalid current profile");
      return;
    }
    buddy::set_active_motor_current_profile(static_cast<buddy::StandardMotorCurrentProfile>(val));
    return;
  }
#endif

  bool report = true;

  #if AXIS_IS_TMC(X) || AXIS_IS_TMC(Y) || AXIS_IS_TMC(Z) || AXIS_IS_TMC(Z2) || AXIS_IS_TMC(Z3)
    const uint8_t index = parser.byteval('I');
  #endif

  LOOP_XYZE(i) if (uint16_t value = parser.intval(axis_codes[i])) {
    report = false;
    switch (i) {
      case X_AXIS:
        #if AXIS_IS_TMC(X)
          if (index == 0) TMC_SET_CURRENT(X);
        #endif
        break;
      case Y_AXIS:
        #if AXIS_IS_TMC(Y)
          if (index == 0) TMC_SET_CURRENT(Y);
        #endif
        break;
      case Z_AXIS:
        #if AXIS_IS_TMC(Z)
          if (index == 0) TMC_SET_CURRENT(Z);
        #endif
        #if AXIS_IS_TMC(Z2)
          // #error dead code found by automatic analyses (see BFW-5461)
          if (index == 1) TMC_SET_CURRENT(Z2);
        #endif
        #if AXIS_IS_TMC(Z3)
          // #error dead code found by automatic analyses (see BFW-5461)
          if (index == 2) TMC_SET_CURRENT(Z3);
        #endif
        break;
      case E_AXIS: {
#if HAS_INDX()
        // INDX has passive tools (single E stepper), so apply even with no tool selected.
        static_assert(E_STEPPERS == 1, "INDX assumes a single E stepper");
        switch (E_INDEX_N(0)) {
#else
        const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(get_target_physical_from_command());
        if (!tool.has_value()) return;
        switch (E_INDEX_N(tool->to_raw())) {
#endif
          #if AXIS_IS_TMC(E0)
            case 0: TMC_SET_CURRENT(E0); break;
          #endif
          #if AXIS_IS_TMC(E1)
            // #error dead code found by automatic analyses (see BFW-5461)
            case 1: TMC_SET_CURRENT(E1); break;
          #endif
          #if AXIS_IS_TMC(E2)
            // #error dead code found by automatic analyses (see BFW-5461)
            case 2: TMC_SET_CURRENT(E2); break;
          #endif
          #if AXIS_IS_TMC(E3)
            // #error dead code found by automatic analyses (see BFW-5461)
            case 3: TMC_SET_CURRENT(E3); break;
          #endif
          #if AXIS_IS_TMC(E4)
            // #error dead code found by automatic analyses (see BFW-5461)
            case 4: TMC_SET_CURRENT(E4); break;
          #endif
          #if AXIS_IS_TMC(E5)
            // #error dead code found by automatic analyses (see BFW-5461)
            case 5: TMC_SET_CURRENT(E5); break;
          #endif
        }
      } break;
    }
  }

  if (report) {
    #if AXIS_IS_TMC(X)
      TMC_SAY_CURRENT(X);
    #endif
    #if AXIS_IS_TMC(Y)
      TMC_SAY_CURRENT(Y);
    #endif
    #if AXIS_IS_TMC(Z)
      TMC_SAY_CURRENT(Z);
    #endif
    #if AXIS_IS_TMC(Z2)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_SAY_CURRENT(Z2);
    #endif
    #if AXIS_IS_TMC(Z3)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_SAY_CURRENT(Z3);
    #endif
    #if AXIS_IS_TMC(E0)
      TMC_SAY_CURRENT(E0);
    #endif
    #if AXIS_IS_TMC(E1)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_SAY_CURRENT(E1);
    #endif
    #if AXIS_IS_TMC(E2)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_SAY_CURRENT(E2);
    #endif
    #if AXIS_IS_TMC(E3)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_SAY_CURRENT(E3);
    #endif
    #if AXIS_IS_TMC(E4)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_SAY_CURRENT(E4);
    #endif
    #if AXIS_IS_TMC(E5)
      // #error dead code found by automatic analyses (see BFW-5461)
      TMC_SAY_CURRENT(E5);
    #endif
  }
}

/** @}*/

#endif // HAS_TRINAMIC
