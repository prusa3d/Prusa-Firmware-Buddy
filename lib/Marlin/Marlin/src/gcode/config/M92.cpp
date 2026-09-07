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
#include "../../module/planner.h"
#include <utils/variant_utils.hpp>
#include <option/has_indx.h>

#include <option/has_phase_stepping.h>
#if HAS_PHASE_STEPPING()
  #include <feature/phase_stepping/axes.hpp>
#endif

void report_M92(const bool echo=true, const int8_t e=-1) {
  if (echo) SERIAL_ECHO_START(); else SERIAL_CHAR(' ');
  SERIAL_ECHOPAIR(" M92 X", LINEAR_UNIT(planner.settings.axis_steps_per_mm[X_AXIS]),
                  " Y", LINEAR_UNIT(planner.settings.axis_steps_per_mm[Y_AXIS]),
                  " Z", LINEAR_UNIT(planner.settings.axis_steps_per_mm[Z_AXIS]));
  #if DISABLED(DISTINCT_E_FACTORS)
    SERIAL_ECHOPAIR(" E", VOLUMETRIC_UNIT(planner.settings.axis_steps_per_mm[E_AXIS]));
  #endif
  SERIAL_EOL();

  #if ENABLED(DISTINCT_E_FACTORS)
    // #error dead code found by automatic analyses (see BFW-5461)
    for (uint8_t i = 0; i < E_STEPPERS; i++) {
      if (e >= 0 && i != e) continue;
      if (echo) SERIAL_ECHO_START(); else SERIAL_CHAR(' ');
      SERIAL_ECHOLNPAIR(" M92 T", (int)i,
                        " E", VOLUMETRIC_UNIT(planner.settings.axis_steps_per_mm[E_AXIS_N(i)]));
    }
  #endif
}

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M92: Get/Set axis steps-per-unit <a href="https://reprap.org/wiki/G-code#M92:_Set_axis_steps_per_unit">M92: Set axis_steps_per_unit</a>
 *
 * With multiple extruders use T to specify which one.
 *
 * If no argument is given print the current values.
 *
 * With MAGIC_NUMBERS_GCODE:
 * Use 'H' and/or 'L' to get ideal layer-height information.
 * 'H' specifies micro-steps to use. We guess if it's not supplied.
 * 'L' specifies a desired layer height. Nearest good heights are shown.
 *
 *#### Usage
 *
 *    M92 [ X | Y | Z | E | T | H | L ]
 *
 *#### Parameters
 *
 *  - `X` - Set current position on X axis
 *  - `Y` - Set steps-per-unit on Y axis
 *  - `Z` - Set steps-per-unit on Z axis
 *  - `E` - Set steps-per-unit on E axis
 *  - `T` - Set steps-per-unit on E axis of tool
 *  - `H` - Specifies micro-steps to use. We guess if it's not supplied.     (Not active by default)
 *  - `L` - Specifies a desired layer height. Nearest good heights are shown (Not active by default)
 *
 * Without parameters prints the current steps-per-unit
 *
 * Warning: Invalidates homing.
 */
void GcodeSuite::M92() {
  // No arguments? Show M92 report.
  if (!parser.seen("XYZE"
    #if ENABLED(MAGIC_NUMBERS_GCODE)
      // #error dead code found by automatic analyses (see BFW-5461)
      "HL"
    #endif
  )) {

    const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(get_target_physical_from_command());
    return report_M92(true, tool.has_value() ? tool->to_raw() : -1);
  }

  #if HAS_PHASE_STEPPING()
    phase_stepping::EnsureDisabled phstep_guard;
  #endif

  // We need to synchronize before we can change axis steps per unit
  planner.synchronize();
  if (planner.draining()) {
    return;
  }

  {
    auto s = planner.user_settings;

    LOOP_XYZE(i) {
      if (parser.seenval(axis_codes[i])) {
        const float value = parser.value_per_axis_units((AxisEnum)i);

        if (i == E_AXIS) {
#if E_STEPPERS == 1
          const AxisEnum e_axis = E_AXIS_N(0);
#else
          const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(get_target_physical_from_command());
          if(!tool.has_value()) {
            continue;
          }
          const AxisEnum e_axis = E_AXIS_N(tool->to_raw());
#endif
          if (value < 20) {
            float factor = planner.settings.axis_steps_per_mm[e_axis] / value; // increase e constants if M92 E14 is given for netfab.
            #if HAS_CLASSIC_E_JERK
              s.max_jerk.e *= factor;
            #endif
            s.max_feedrate_mm_s[e_axis] *= factor;
            planner.max_acceleration_msteps_per_s2[e_axis] = static_cast<uint32_t>(planner.max_acceleration_msteps_per_s2[e_axis] * factor);
          }
          s.axis_steps_per_mm[e_axis] = value;
          s.axis_msteps_per_mm[e_axis] = value * PLANNER_STEPS_MULTIPLIER;
        }
        else {
          s.axis_steps_per_mm[i] = value;
          s.axis_msteps_per_mm[i] = value * PLANNER_STEPS_MULTIPLIER;
        }
      }
    }

    planner.apply_settings(s);
  }
  planner.refresh_positioning();

  #if ENABLED(MAGIC_NUMBERS_GCODE)
    // #error dead code found by automatic analyses (see BFW-5461)
    #ifndef Z_MICROSTEPS
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z_MICROSTEPS 16
    #endif
    const float wanted = parser.floatval('L');
    if (parser.seen('H') || wanted) {
      const uint16_t argH = parser.ushortval('H'),
                     micro_steps = argH ?: Z_MICROSTEPS;
      const float z_full_step_mm = micro_steps * planner.mm_per_step[Z_AXIS];
      SERIAL_ECHO_START();
      SERIAL_ECHOPAIR("{ micro_steps:", micro_steps, ", z_full_step_mm:", z_full_step_mm);
      if (wanted) {
        const float best = uint16_t(wanted / z_full_step_mm) * z_full_step_mm;
        SERIAL_ECHOPAIR(", best:[", best);
        if (best != wanted) { SERIAL_CHAR(','); SERIAL_ECHO(best + z_full_step_mm); }
        SERIAL_CHAR(']');
      }
      SERIAL_ECHOLNPGM(" }");
    }
  #endif

  #if HAS_PHASE_STEPPING()
    // FIXME: Code above does not update config store, phstep takes data from config store
    phase_stepping::initialize_axis_motor_params();
  #endif

  // Invalidate all homing
  axes_home_level = AxesHomeLevel::no_axes_homed;
}

/** @}*/
