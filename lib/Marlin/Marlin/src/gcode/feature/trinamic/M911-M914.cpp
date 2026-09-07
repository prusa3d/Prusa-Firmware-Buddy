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
#include <option/has_crash_detection.h>

#if HAS_TRINAMIC

#include "../../gcode.h"
#include "../../../feature/tmc_util.h"
#include "../../../module/stepper/indirection.h"
#include "../../../module/planner.h"
#include <utils/variant_utils.hpp>
#include <option/has_indx.h>
#include "../../queue.h"

#include "config_store/store_instance.hpp"

#if HAS_CRASH_DETECTION()
  #include "../../feature/prusa/crash_recovery.hpp"
#endif

#if ENABLED(MONITOR_DRIVER_STATUS)

  #define M91x_USE(ST) (AXIS_DRIVER_TYPE(ST, TMC2130) || AXIS_DRIVER_TYPE(ST, TMC2160) || AXIS_DRIVER_TYPE(ST, TMC2208) || AXIS_DRIVER_TYPE(ST, TMC2209) || AXIS_DRIVER_TYPE(ST, TMC2660) || AXIS_DRIVER_TYPE(ST, TMC5130) || AXIS_DRIVER_TYPE(ST, TMC5160))
  #define M91x_USE_E(N) (E_STEPPERS > N && M91x_USE(E##N))

  #define M91x_SOME_X (M91x_USE(X))
  #define M91x_SOME_Y (M91x_USE(Y))
  #define M91x_SOME_Z (M91x_USE(Z) || M91x_USE(Z2) || M91x_USE(Z3))
  #define M91x_SOME_E (M91x_USE_E(0) || M91x_USE_E(1) || M91x_USE_E(2) || M91x_USE_E(3) || M91x_USE_E(4) || M91x_USE_E(5))

  #if !M91x_SOME_X && !M91x_SOME_Y && !M91x_SOME_Z && !M91x_SOME_E
    #error "MONITOR_DRIVER_STATUS requires at least one TMC2130, 2160, 2208, 2209, 2660, 5130, or 5160."
  #endif

  /** \addtogroup G-Codes
   * @{
   */

  /**
   *### M911: Report TMC stepper driver overtemperature pre-warn flag <a href=" "> </a>
  *
  * Only MK3.5/S, MK3.9/S and MK4/S
  *
  *#### Usage
  *
  *    M911
  *
  */
  void GcodeSuite::M911() {
    #if M91x_USE(X)
      tmc_report_otpw(stepperX);
    #endif
    #if M91x_USE(Y)
      tmc_report_otpw(stepperY);
    #endif
    #if M91x_USE(Z)
      tmc_report_otpw(stepperZ);
    #endif
    #if M91x_USE(Z2)
      // #error dead code found by automatic analyses (see BFW-5461)
      tmc_report_otpw(stepperZ2);
    #endif
    #if M91x_USE(Z3)
      // #error dead code found by automatic analyses (see BFW-5461)
      tmc_report_otpw(stepperZ3);
    #endif
    #if M91x_USE_E(0)
      tmc_report_otpw(stepperE0);
    #endif
    #if M91x_USE_E(1)
      // #error dead code found by automatic analyses (see BFW-5461)
      tmc_report_otpw(stepperE1);
    #endif
    #if M91x_USE_E(2)
      // #error dead code found by automatic analyses (see BFW-5461)
      tmc_report_otpw(stepperE2);
    #endif
    #if M91x_USE_E(3)
      // #error dead code found by automatic analyses (see BFW-5461)
      tmc_report_otpw(stepperE3);
    #endif
    #if M91x_USE_E(4)
      // #error dead code found by automatic analyses (see BFW-5461)
      tmc_report_otpw(stepperE4);
    #endif
    #if M91x_USE_E(5)
      // #error dead code found by automatic analyses (see BFW-5461)
      tmc_report_otpw(stepperE5);
    #endif
  }

  /**
   *### M912: Clear TMC stepper driver overtemperature pre-warn flag held by the library <a href=" "> </a>
  *
  * Only MK3.5/S, MK3.9/S and MK4/S
  *
  *#### Usage
  *
  *    M912 [ X | Y | Z | E |  ]
  *
  *#### Parameters
  *
  * - `X` - X driver
  * - `Y` - Y driver
  * - `Z` - Z driver
  * - `E` - all E drivers
  *   -`E<number>` - Only E<number>
  *
  * Without parameters clear all
  *
  * #### Examples:
  *       M912 X   ; clear X
  *       M912 X1  ; clear X1 only
  *       M912 X E ; clear X and all E
  *       M912 E1  ; clear E1 only
  */
  void GcodeSuite::M912() {
    #if M91x_SOME_X
      const bool hasX = parser.seen(axis_codes.x);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      constexpr bool hasX = false;
    #endif

    #if M91x_SOME_Y
      const bool hasY = parser.seen(axis_codes.y);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      constexpr bool hasY = false;
    #endif

    #if M91x_SOME_Z
      const bool hasZ = parser.seen(axis_codes.z);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      constexpr bool hasZ = false;
    #endif

    #if M91x_SOME_E
      const bool hasE = parser.seen(axis_codes.e);
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      constexpr bool hasE = false;
    #endif

    const bool hasNone = !hasX && !hasY && !hasZ && !hasE;

    #if M91x_SOME_X
      const int8_t xval = int8_t(parser.byteval(axis_codes.x, 0xFF));
      #if M91x_USE(X)
        if (hasNone || xval == 1 || (hasX && xval < 0)) tmc_clear_otpw(stepperX);
      #endif
    #endif

    #if M91x_SOME_Y
      const int8_t yval = int8_t(parser.byteval(axis_codes.y, 0xFF));
      #if M91x_USE(Y)
        if (hasNone || yval == 1 || (hasY && yval < 0)) tmc_clear_otpw(stepperY);
      #endif
    #endif

    #if M91x_SOME_Z
      const int8_t zval = int8_t(parser.byteval(axis_codes.z, 0xFF));
      #if M91x_USE(Z)
        if (hasNone || zval == 1 || (hasZ && zval < 0)) tmc_clear_otpw(stepperZ);
      #endif
      #if M91x_USE(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        if (hasNone || zval == 2 || (hasZ && zval < 0)) tmc_clear_otpw(stepperZ2);
      #endif
      #if M91x_USE(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        if (hasNone || zval == 3 || (hasZ && zval < 0)) tmc_clear_otpw(stepperZ3);
      #endif
    #endif

    #if M91x_SOME_E
      const int8_t eval = int8_t(parser.byteval(axis_codes.e, 0xFF));
      #if M91x_USE_E(0)
        if (hasNone || eval == 0 || (hasE && eval < 0)) tmc_clear_otpw(stepperE0);
      #endif
      #if M91x_USE_E(1)
        // #error dead code found by automatic analyses (see BFW-5461)
        if (hasNone || eval == 1 || (hasE && eval < 0)) tmc_clear_otpw(stepperE1);
      #endif
      #if M91x_USE_E(2)
        // #error dead code found by automatic analyses (see BFW-5461)
        if (hasNone || eval == 2 || (hasE && eval < 0)) tmc_clear_otpw(stepperE2);
      #endif
      #if M91x_USE_E(3)
        // #error dead code found by automatic analyses (see BFW-5461)
        if (hasNone || eval == 3 || (hasE && eval < 0)) tmc_clear_otpw(stepperE3);
      #endif
      #if M91x_USE_E(4)
        // #error dead code found by automatic analyses (see BFW-5461)
        if (hasNone || eval == 4 || (hasE && eval < 0)) tmc_clear_otpw(stepperE4);
      #endif
      #if M91x_USE_E(5)
        // #error dead code found by automatic analyses (see BFW-5461)
        if (hasNone || eval == 5 || (hasE && eval < 0)) tmc_clear_otpw(stepperE5);
      #endif
    #endif
  }

#endif // MONITOR_DRIVER_STATUS

/**
 *### M913: Get/Set Set Hybrid Threshold Speed <a href=" "> </a>
 *
 * Only MK3.5/S, MK3.9/S and MK4/S
 *
 *#### Usage
 *
 *    M913 [ X | Y | Z | E | I ]
 *
 *#### Parameters
 *
 * - `X` - Set Hybrid Threshold for X to the given value
 * - `Y` - Set Hybrid Threshold for Y to the given value
 * - `Z` - Set Hybrid Threshold for Z to the given value
 * - `E` - Set Hybrid Threshold for E to the given value
 * - `I` - Index for multiple steppers
 *   - `1` - for Z2
 *   - `2` - for Z3
 *
 * With no parameters report stealthCop max speeds
 */
#if ENABLED(HYBRID_THRESHOLD)
  void GcodeSuite::M913() {
    #define TMC_SAY_PWMTHRS(A,Q) tmc_print_pwmthrs(stepper##Q)
    #define TMC_SET_PWMTHRS(A,Q) stepper##Q.set_pwm_thrs(value)
    #define TMC_SAY_PWMTHRS_E(E) tmc_print_pwmthrs(stepperE##E)
    #define TMC_SET_PWMTHRS_E(E) stepperE##E.set_pwm_thrs(value)

    bool report = true;
    #if AXIS_IS_TMC(X) || AXIS_IS_TMC(Y) || AXIS_IS_TMC(Z) || AXIS_IS_TMC(Z2) || AXIS_IS_TMC(Z3)
      const uint8_t index = parser.byteval('I');
    #endif
    LOOP_XYZE(i) if (int32_t value = parser.longval(axis_codes[i])) {
      report = false;
      switch (i) {
        case X_AXIS:
          #if AXIS_HAS_STEALTHCHOP(X)
            if (index < 2) TMC_SET_PWMTHRS(X,X);
          #endif
          break;
        case Y_AXIS:
          #if AXIS_HAS_STEALTHCHOP(Y)
            if (index < 2) TMC_SET_PWMTHRS(Y,Y);
          #endif
          break;
        case Z_AXIS:
          #if AXIS_HAS_STEALTHCHOP(Z)
            if (index < 2) TMC_SET_PWMTHRS(Z,Z);
          #endif
          #if AXIS_HAS_STEALTHCHOP(Z2)
            // #error dead code found by automatic analyses (see BFW-5461)
            if (index == 0 || index == 2) TMC_SET_PWMTHRS(Z,Z2);
          #endif
          #if AXIS_HAS_STEALTHCHOP(Z3)
            // #error dead code found by automatic analyses (see BFW-5461)
            if (index == 0 || index == 3) TMC_SET_PWMTHRS(Z,Z3);
          #endif
          break;
        case E_AXIS: {
          #if E_STEPPERS
          #if HAS_INDX()
            // INDX has passive tools (single E stepper), so apply even with no tool selected.
            static_assert(E_STEPPERS == 1, "INDX assumes a single E stepper");
            switch (E_INDEX_N(0)) {
          #else
            const std::optional<PhysicalToolIndex> tool = stdext::get_optional<PhysicalToolIndex>(get_target_physical_from_command());
            if (!tool.has_value()) return;
            switch (E_INDEX_N(tool->to_raw())) {
          #endif
              #if AXIS_HAS_STEALTHCHOP(E0)
                case 0: TMC_SET_PWMTHRS_E(0); break;
              #endif
              #if E_STEPPERS > 1 && AXIS_HAS_STEALTHCHOP(E1)
                // #error dead code found by automatic analyses (see BFW-5461)
                case 1: TMC_SET_PWMTHRS_E(1); break;
              #endif
              #if E_STEPPERS > 2 && AXIS_HAS_STEALTHCHOP(E2)
                // #error dead code found by automatic analyses (see BFW-5461)
                case 2: TMC_SET_PWMTHRS_E(2); break;
              #endif
              #if E_STEPPERS > 3 && AXIS_HAS_STEALTHCHOP(E3)
                // #error dead code found by automatic analyses (see BFW-5461)
                case 3: TMC_SET_PWMTHRS_E(3); break;
              #endif
              #if E_STEPPERS > 4 && AXIS_HAS_STEALTHCHOP(E4)
                // #error dead code found by automatic analyses (see BFW-5461)
                case 4: TMC_SET_PWMTHRS_E(4); break;
              #endif
              #if E_STEPPERS > 5 && AXIS_HAS_STEALTHCHOP(E5)
                // #error dead code found by automatic analyses (see BFW-5461)
                case 5: TMC_SET_PWMTHRS_E(5); break;
              #endif
            }
          #endif // E_STEPPERS
        } break;
      }
    }

    if (report) {
      #if AXIS_HAS_STEALTHCHOP(X)
        TMC_SAY_PWMTHRS(X,X);
      #endif
      #if AXIS_HAS_STEALTHCHOP(Y)
        TMC_SAY_PWMTHRS(Y,Y);
      #endif
      #if AXIS_HAS_STEALTHCHOP(Z)
        TMC_SAY_PWMTHRS(Z,Z);
      #endif
      #if AXIS_HAS_STEALTHCHOP(Z2)
        // #error dead code found by automatic analyses (see BFW-5461)
        TMC_SAY_PWMTHRS(Z,Z2);
      #endif
      #if AXIS_HAS_STEALTHCHOP(Z3)
        // #error dead code found by automatic analyses (see BFW-5461)
        TMC_SAY_PWMTHRS(Z,Z3);
      #endif
      #if E_STEPPERS && AXIS_HAS_STEALTHCHOP(E0)
        TMC_SAY_PWMTHRS_E(0);
      #endif
      #if E_STEPPERS > 1 && AXIS_HAS_STEALTHCHOP(E1)
        // #error dead code found by automatic analyses (see BFW-5461)
        TMC_SAY_PWMTHRS_E(1);
      #endif
      #if E_STEPPERS > 2 && AXIS_HAS_STEALTHCHOP(E2)
        // #error dead code found by automatic analyses (see BFW-5461)
        TMC_SAY_PWMTHRS_E(2);
      #endif
      #if E_STEPPERS > 3 && AXIS_HAS_STEALTHCHOP(E3)
        // #error dead code found by automatic analyses (see BFW-5461)
        TMC_SAY_PWMTHRS_E(3);
      #endif
      #if E_STEPPERS > 4 && AXIS_HAS_STEALTHCHOP(E4)
        // #error dead code found by automatic analyses (see BFW-5461)
        TMC_SAY_PWMTHRS_E(4);
      #endif
      #if E_STEPPERS > 5 && AXIS_HAS_STEALTHCHOP(E5)
        // #error dead code found by automatic analyses (see BFW-5461)
        TMC_SAY_PWMTHRS_E(5);
      #endif
    }
  }
#endif // HYBRID_THRESHOLD

/**
 *### M914: Get/Set StallGuard sensitivity <a href=" "> </a>
 *
 *#### Usage
 *
 *    M914 [ X | Y | Z | I ]
 *
 *#### Parameters
 *
 * - `X` - Sensitivity of the X stepper driver
 * - `Y` - Sensitivity of the Y stepper driver
 * - `Z` - Sensitivity of the Z stepper driver
 * - `I` - Index for multiple steppers
 *   - `1` - for Z2
 *   - `2` - for Z3
 *
 * With no parameters report StallGuard homing sensitivities
 */
#if USE_SENSORLESS
  void GcodeSuite::M914() {

    bool report = true;
    const uint8_t index = parser.byteval('I');
    LOOP_XYZ(i) if (parser.seen(axis_codes[i])) {
      int16_t value = parser.value_int();
      report = false;
      switch (i) {
        #if X_SENSORLESS
          case X_AXIS:
            if (!parser.has_value()) {
              value = X_STALL_SENSITIVITY;
            }
            #if AXIS_HAS_STALLGUARD(X)
              #if HAS_CRASH_DETECTION()
                if (index < 2) crash_s.home_sensitivity[0] = value;
              #else
                // #error dead code found by automatic analyses (see BFW-5461)
                if (index < 2) stepperX.stall_sensitivity(value);
              #endif
            #endif
            break;
        #endif
        #if Y_SENSORLESS
          case Y_AXIS:
            if (!parser.has_value()) {
              value = Y_STALL_SENSITIVITY;
            }
            #if AXIS_HAS_STALLGUARD(Y)
              #if HAS_CRASH_DETECTION()
                if (index < 2) crash_s.home_sensitivity[1] = value;
              #else
                // #error dead code found by automatic analyses (see BFW-5461)
                if (index < 2) stepperY.stall_sensitivity(value);
              #endif
            #endif
            break;
        #endif
        #if Z_SENSORLESS
          case Z_AXIS:
            if (!parser.has_value()) {
              value = Z_STALL_SENSITIVITY;
            }
            #if AXIS_HAS_STALLGUARD(Z)
              #if HAS_CRASH_DETECTION()
                if (index < 2) crash_s.home_sensitivity[2] = value;
              #else
                // #error dead code found by automatic analyses (see BFW-5461)
                if (index < 2) stepperZ.stall_sensitivity(value);
              #endif
            #endif
            #if AXIS_HAS_STALLGUARD(Z2)
              // #error dead code found by automatic analyses (see BFW-5461)
              #if HAS_CRASH_DETECTION()
                #error "Not implemented."
              #else
                // #error dead code found by automatic analyses (see BFW-5461)
                if (index == 0 || index == 2) stepperZ2.stall_sensitivity(value);
              #endif
            #endif
            #if AXIS_HAS_STALLGUARD(Z3)
              // #error dead code found by automatic analyses (see BFW-5461)
              #if HAS_CRASH_DETECTION()
                #error "Not implemented."
              #else
                // #error dead code found by automatic analyses (see BFW-5461)
                if (index == 0 || index == 3) stepperZ3.stall_sensitivity(value);
              #endif
            #endif
            break;
        #endif
      }
    }

    if (report) {
      #if HAS_CRASH_DETECTION()
        SERIAL_ECHOPGM("X homing sensitivity: ");
        SERIAL_PRINTLN(crash_s.home_sensitivity[0], DEC);
        SERIAL_ECHOPGM("Y homing sensitivity: ");
        SERIAL_PRINTLN(crash_s.home_sensitivity[1], DEC);
        SERIAL_ECHOPGM("Z homing sensitivity: ");
        SERIAL_PRINTLN(crash_s.home_sensitivity[2], DEC);
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        #if X_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          #if AXIS_HAS_STALLGUARD(X)
            // #error dead code found by automatic analyses (see BFW-5461)
            tmc_print_sgt(stepperX);
          #endif
        #endif
        #if Y_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          #if AXIS_HAS_STALLGUARD(Y)
            // #error dead code found by automatic analyses (see BFW-5461)
            tmc_print_sgt(stepperY);
          #endif
        #endif
        #if Z_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          #if AXIS_HAS_STALLGUARD(Z)
            // #error dead code found by automatic analyses (see BFW-5461)
            tmc_print_sgt(stepperZ);
          #endif
          #if AXIS_HAS_STALLGUARD(Z2)
            // #error dead code found by automatic analyses (see BFW-5461)
            tmc_print_sgt(stepperZ2);
          #endif
          #if AXIS_HAS_STALLGUARD(Z3)
            // #error dead code found by automatic analyses (see BFW-5461)
            tmc_print_sgt(stepperZ3);
          #endif
        #endif
      #endif
    }
  }
#endif // USE_SENSORLESS

/** @}*/

#endif // HAS_TRINAMIC
