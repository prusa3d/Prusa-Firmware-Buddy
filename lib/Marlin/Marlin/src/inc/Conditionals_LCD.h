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

#include <option/has_mmu2.h>
#include <option/has_toolchanger.h>
#include <option/has_indx.h>

/**
 * Conditionals_LCD.h
 * Conditionals that need to be set before Configuration_adv.h or pins.h
 */

/**
 * Extruders have some combination of stepper motors and hotends
 * so we separate these concepts into the defines:
 *
 *  EXTRUDERS    - Number of Selectable Tools
 *  HOTENDS      - Number of hotends, whether connected or separate
 *  E_STEPPERS   - Number of actual E stepper motors
 *
 * These defines must be simple constants for use in REPEAT, etc.
 */
#define HAS_EXTRUDERS 1
#if EXTRUDERS > 1
  #define HAS_MULTI_EXTRUDER 1
#endif
#define E_AXIS_N(E) AxisEnum(E_AXIS + E_INDEX_N(E))

#define E_OPTARG(N) OPTARG(HAS_MULTI_EXTRUDER, N)
#define E_TERN_(N)  TERN_(HAS_MULTI_EXTRUDER, N)
#define E_TERN0(N)  TERN0(HAS_MULTI_EXTRUDER, N)

#if ENABLED(E_DUAL_STEPPER_DRIVERS) // E0/E1 steppers act in tandem as E0
  // #error dead code found by automatic analyses (see BFW-5461)
  #define E_STEPPERS      2

#elif HAS_MMU2()                // Průša Multi-Material Unit v2
  #define E_STEPPERS      1

#elif HAS_TOOLCHANGER() || HAS_INDX()
  #define E_STEPPERS      1
#endif

// Průša MMU1, MMU(S) 2.0 and EXTENDABLE_EMU_MMU2(S) force SINGLENOZZLE
#if HAS_MMU2()
  #define SINGLENOZZLE
#endif

#if ENABLED(SINGLENOZZLE)           // One hotend, one thermistor, no XY offset
  #undef HOTENDS
  #define HOTENDS       1
#endif

#ifndef HOTENDS
  #define HOTENDS EXTRUDERS
#endif
#ifndef E_STEPPERS
  #define E_STEPPERS EXTRUDERS
#endif

#if E_STEPPERS <= 7
  #undef INVERT_E7_DIR
  #if E_STEPPERS <= 6
    #undef INVERT_E6_DIR
    #if E_STEPPERS <= 5
      #undef INVERT_E5_DIR
      #if E_STEPPERS <= 4
        #undef INVERT_E4_DIR
        #if E_STEPPERS <= 3
          #undef INVERT_E3_DIR
          #if E_STEPPERS <= 2
            #undef INVERT_E2_DIR
            #if E_STEPPERS <= 1
              #undef INVERT_E1_DIR
              #if E_STEPPERS == 0
                // #error dead code found by automatic analyses (see BFW-5461)
                #undef INVERT_E0_DIR
              #endif
            #endif
          #endif
        #endif
      #endif
    #endif
  #endif
#endif

/**
 * Number of Linear Axes (e.g., XYZIJKUVW)
 * All the logical axes except for the tool (E) axis
 */
#ifdef NUM_AXES
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef NUM_AXES
  #define NUM_AXES_WARNING 1
#endif

#ifdef W_DRIVER_TYPE
  // #error dead code found by automatic analyses (see BFW-5461)
  #define NUM_AXES 9
#elif defined(V_DRIVER_TYPE)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define NUM_AXES 8
#elif defined(U_DRIVER_TYPE)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define NUM_AXES 7
#elif defined(K_DRIVER_TYPE)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define NUM_AXES 6
#elif defined(J_DRIVER_TYPE)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define NUM_AXES 5
#elif defined(I_DRIVER_TYPE)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define NUM_AXES 4
#elif defined(Z_DRIVER_TYPE)
  #define NUM_AXES 3
#elif defined(Y_DRIVER_TYPE)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define NUM_AXES 2
#else
  // #error dead code found by automatic analyses (see BFW-5461)
  #define NUM_AXES 1
#endif
#if NUM_AXES >= XY
  #define HAS_Y_AXIS 1
  #if NUM_AXES >= XYZ
    #define HAS_Z_AXIS 1
    #if defined(Z3_DRIVER_TYPE)
      // #error dead code found by automatic analyses (see BFW-5461)
      #define NUM_Z_STEPPERS 3
    #elif defined(Z2_DRIVER_TYPE)
      // #error dead code found by automatic analyses (see BFW-5461)
      #define NUM_Z_STEPPERS 2
    #else
      #define NUM_Z_STEPPERS 1
    #endif
    #if NUM_AXES >= 4
      // #error dead code found by automatic analyses (see BFW-5461)
      #define HAS_I_AXIS 1
      #if NUM_AXES >= 5
        // #error dead code found by automatic analyses (see BFW-5461)
        #define HAS_J_AXIS 1
        #if NUM_AXES >= 6
          // #error dead code found by automatic analyses (see BFW-5461)
          #define HAS_K_AXIS 1
          #if NUM_AXES >= 7
            // #error dead code found by automatic analyses (see BFW-5461)
            #define HAS_U_AXIS 1
            #if NUM_AXES >= 8
              // #error dead code found by automatic analyses (see BFW-5461)
              #define HAS_V_AXIS 1
              #if NUM_AXES >= 9
                // #error dead code found by automatic analyses (see BFW-5461)
                #define HAS_W_AXIS 1
              #endif
            #endif
          #endif
        #endif
      #endif
    #endif
  #endif
#endif

#if E_STEPPERS <= 0
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef E0_DRIVER_TYPE
#endif
#if E_STEPPERS <= 1
  #undef E1_DRIVER_TYPE
#endif
#if E_STEPPERS <= 2
  #undef E2_DRIVER_TYPE
#endif
#if E_STEPPERS <= 3
  #undef E3_DRIVER_TYPE
#endif
#if E_STEPPERS <= 4
  #undef E4_DRIVER_TYPE
#endif
#if E_STEPPERS <= 5
  #undef E5_DRIVER_TYPE
#endif
#if E_STEPPERS <= 6
  #undef E6_DRIVER_TYPE
#endif
#if E_STEPPERS <= 7
  #undef E7_DRIVER_TYPE
#endif

#if !HAS_Y_AXIS
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef ENDSTOPPULLUP_YMIN
  #undef ENDSTOPPULLUP_YMAX
  #undef Y_MIN_ENDSTOP_INVERTING
  #undef Y_MAX_ENDSTOP_INVERTING
  #undef Y_ENABLE_ON
  #undef DISABLE_Y
  #undef INVERT_Y_DIR
  #undef Y_HOME_DIR
  #undef Y_MIN_POS
  #undef Y_MAX_POS
  #undef MANUAL_Y_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_Y
  #undef MAX_SOFTWARE_ENDSTOP_Y
  #undef SAFE_BED_LEVELING_START_Y
#endif

#if !HAS_Z_AXIS
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef ENDSTOPPULLUP_ZMIN
  #undef ENDSTOPPULLUP_ZMAX
  #undef Z_MIN_ENDSTOP_INVERTING
  #undef Z_MAX_ENDSTOP_INVERTING
  #undef Z2_DRIVER_TYPE
  #undef Z3_DRIVER_TYPE
  #undef Z_ENABLE_ON
  #undef DISABLE_Z
  #undef INVERT_Z_DIR
  #undef Z_HOME_DIR
  #undef Z_MIN_POS
  #undef Z_MAX_POS
  #undef MANUAL_Z_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_Z
  #undef MAX_SOFTWARE_ENDSTOP_Z
  #undef SAFE_BED_LEVELING_START_Z
#endif

#if !HAS_I_AXIS
  #undef ENDSTOPPULLUP_IMIN
  #undef ENDSTOPPULLUP_IMAX
  #undef I_MIN_ENDSTOP_INVERTING
  #undef I_MAX_ENDSTOP_INVERTING
  #undef I_ENABLE_ON
  #undef DISABLE_I
  #undef INVERT_I_DIR
  #undef I_HOME_DIR
  #undef I_MIN_POS
  #undef I_MAX_POS
  #undef MANUAL_I_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_I
  #undef MAX_SOFTWARE_ENDSTOP_I
  #undef SAFE_BED_LEVELING_START_I
#endif

#if !HAS_J_AXIS
  #undef ENDSTOPPULLUP_JMIN
  #undef ENDSTOPPULLUP_JMAX
  #undef J_MIN_ENDSTOP_INVERTING
  #undef J_MAX_ENDSTOP_INVERTING
  #undef J_ENABLE_ON
  #undef DISABLE_J
  #undef INVERT_J_DIR
  #undef J_HOME_DIR
  #undef J_MIN_POS
  #undef J_MAX_POS
  #undef MANUAL_J_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_J
  #undef MAX_SOFTWARE_ENDSTOP_J
  #undef SAFE_BED_LEVELING_START_J
#endif

#if !HAS_K_AXIS
  #undef ENDSTOPPULLUP_KMIN
  #undef ENDSTOPPULLUP_KMAX
  #undef K_MIN_ENDSTOP_INVERTING
  #undef K_MAX_ENDSTOP_INVERTING
  #undef K_ENABLE_ON
  #undef DISABLE_K
  #undef INVERT_K_DIR
  #undef K_HOME_DIR
  #undef K_MIN_POS
  #undef K_MAX_POS
  #undef MANUAL_K_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_K
  #undef MAX_SOFTWARE_ENDSTOP_K
  #undef SAFE_BED_LEVELING_START_K
#endif

#if !HAS_U_AXIS
  #undef ENDSTOPPULLUP_UMIN
  #undef ENDSTOPPULLUP_UMAX
  #undef U_MIN_ENDSTOP_INVERTING
  #undef U_MAX_ENDSTOP_INVERTING
  #undef U_ENABLE_ON
  #undef DISABLE_U
  #undef INVERT_U_DIR
  #undef U_HOME_DIR
  #undef U_MIN_POS
  #undef U_MAX_POS
  #undef MANUAL_U_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_U
  #undef MAX_SOFTWARE_ENDSTOP_U
  #undef SAFE_BED_LEVELING_START_U
#endif

#if !HAS_V_AXIS
  #undef ENDSTOPPULLUP_VMIN
  #undef ENDSTOPPULLUP_VMAX
  #undef V_MIN_ENDSTOP_INVERTING
  #undef V_MAX_ENDSTOP_INVERTING
  #undef V_ENABLE_ON
  #undef DISABLE_V
  #undef INVERT_V_DIR
  #undef V_HOME_DIR
  #undef V_MIN_POS
  #undef V_MAX_POS
  #undef MANUAL_V_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_V
  #undef MAX_SOFTWARE_ENDSTOP_V
  #undef SAFE_BED_LEVELING_START_V
#endif

#if !HAS_W_AXIS
  #undef ENDSTOPPULLUP_WMIN
  #undef ENDSTOPPULLUP_WMAX
  #undef W_MIN_ENDSTOP_INVERTING
  #undef W_MAX_ENDSTOP_INVERTING
  #undef W_ENABLE_ON
  #undef DISABLE_W
  #undef INVERT_W_DIR
  #undef W_HOME_DIR
  #undef W_MIN_POS
  #undef W_MAX_POS
  #undef MANUAL_W_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_W
  #undef MAX_SOFTWARE_ENDSTOP_W
  #undef SAFE_BED_LEVELING_START_W
#endif

/**
 * Number of Primary Linear Axes (e.g., XYZ)
 * X, XY, or XYZ axes. Excluding duplicate axes (Z2, Z3)
 */
#if NUM_AXES >= 3
  #define PRIMARY_LINEAR_AXES 3
#else
  // #error dead code found by automatic analyses (see BFW-5461)
  #define PRIMARY_LINEAR_AXES NUM_AXES
#endif

/**
 * Number of Secondary Axes (e.g., IJKUVW)
 * All linear/rotational axes between XYZ and E.
 */
#define SECONDARY_AXES SUB3(NUM_AXES)

/**
 * Number of Rotational Axes (e.g., IJK)
 * All axes for which AXIS*_ROTATES is defined.
 * For these axes, positions are specified in angular degrees.
 */
#if ENABLED(AXIS9_ROTATES)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define ROTATIONAL_AXES 6
#elif ENABLED(AXIS8_ROTATES)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define ROTATIONAL_AXES 5
#elif ENABLED(AXIS7_ROTATES)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define ROTATIONAL_AXES 4
#elif ENABLED(AXIS6_ROTATES)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define ROTATIONAL_AXES 3
#elif ENABLED(AXIS5_ROTATES)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define ROTATIONAL_AXES 2
#elif ENABLED(AXIS4_ROTATES)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define ROTATIONAL_AXES 1
#else
  #define ROTATIONAL_AXES 0
#endif

/**
 * Number of Secondary Linear Axes (e.g., UVW)
 * All secondary axes for which AXIS*_ROTATES is not defined.
 * Excluding primary axes and excluding duplicate axes (Z2, Z3)
 */
#define SECONDARY_LINEAR_AXES (NUM_AXES - PRIMARY_LINEAR_AXES - ROTATIONAL_AXES)

/**
 * Number of Logical Axes (e.g., XYZIJKUVWE)
 * All logical axes that can be commanded directly by G-code.
 * Delta maps stepper-specific values to ABC steppers.
 */
#if HAS_EXTRUDERS
  #define LOGICAL_AXES INCREMENT(NUM_AXES)
#else
  // #error dead code found by automatic analyses (see BFW-5461)
  #define LOGICAL_AXES NUM_AXES
#endif

/**
 * DISTINCT_E_FACTORS is set to give extruders (some) individual settings.
 *
 * DISTINCT_AXES is the number of distinct addressable axes (not steppers).
 *  Includes all linear axes plus all distinguished extruders.
 *  The default behavior is to treat all extruders as a single E axis
 *  with shared motion and temperature settings.
 *
 * DISTINCT_E is the number of distinguished extruders. By default this
 *  well be 1 which indicates all extruders share the same settings.
 *
 * E_INDEX_N(E) should be used to get the E index of any item that might be
 *  distinguished.
 */
#if ENABLED(DISTINCT_E_FACTORS) && E_STEPPERS > 1
  // #error dead code found by automatic analyses (see BFW-5461)
  #define DISTINCT_AXES (NUM_AXES + E_STEPPERS)
  #define DISTINCT_E E_STEPPERS
  #define E_INDEX_N(E) (E)
  #define UNUSED_E(E) NOOP
#else
  #undef DISTINCT_E_FACTORS
  #define DISTINCT_AXES LOGICAL_AXES
  #define DISTINCT_E 1
  #define E_INDEX_N(E) 0
  #define UNUSED_E(E) UNUSED(E)
#endif
#define XYZE_N (NUM_AXES + E_STEPPERS)

#define HAS_HOTEND 1
#ifndef HOTEND_OVERSHOOT
  #define HOTEND_OVERSHOOT 15
#endif
#if HOTENDS > 1 || HAS_INDX()
  #define HAS_MULTI_HOTEND 1
  #define HAS_HOTEND_OFFSET 1
#endif

#ifndef PREHEAT_1_LABEL
  // #error dead code found by automatic analyses (see BFW-5461)
  #define PREHEAT_1_LABEL "PLA"
#endif

#ifndef PREHEAT_2_LABEL
  // #error dead code found by automatic analyses (see BFW-5461)
  #define PREHEAT_2_LABEL "ABS"
#endif

/**
 * Set flags for enabled probes
 */
#define HAS_BED_PROBE ANY(FIX_MOUNTED_PROBE, TOUCH_MI_PROBE, SENSORLESS_PROBING)
#define PROBE_SELECTED (HAS_BED_PROBE)

#if HAS_BED_PROBE
  #define HAS_CUSTOM_PROBE_PIN  DISABLED(Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN)
  #if (Z_HOME_DIR < 0 && !HAS_CUSTOM_PROBE_PIN)
    #define HOMING_Z_WITH_PROBE 1
  #endif
  #ifndef Z_PROBE_LOW_POINT
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_PROBE_LOW_POINT -5
  #endif
  #ifdef MULTIPLE_PROBING
    #if EXTRA_PROBING
      #define TOTAL_PROBING (MULTIPLE_PROBING + EXTRA_PROBING)
    #else
      #define TOTAL_PROBING MULTIPLE_PROBING
    #endif
  #endif
#else
  // Clear probe pin settings when no probe is selected
  #undef Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN
#endif

#ifdef GRID_MAX_POINTS_X
  #define GRID_MAX_POINTS ((GRID_MAX_POINTS_X) * (GRID_MAX_POINTS_Y))
#endif

#define HAS_SOFTWARE_ENDSTOPS        EITHER(MIN_SOFTWARE_ENDSTOPS, MAX_SOFTWARE_ENDSTOPS)
#define HAS_RESUME_CONTINUE          ANY(EXTENSIBLE_UI, EMERGENCY_PARSER)

#if ENABLED(MORGAN_SCARA)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define IS_SCARA 1
#endif
static_assert(!(ENABLED(DELTA) || ENABLED(IS_SCARA)), "Support dropped");

#ifndef INVERT_X_DIR
  // #error dead code found by automatic analyses (see BFW-5461)
  #define INVERT_X_DIR false
#endif
#ifndef INVERT_Y_DIR
  // #error dead code found by automatic analyses (see BFW-5461)
  #define INVERT_Y_DIR false
#endif
#ifndef INVERT_Z_DIR
  // #error dead code found by automatic analyses (see BFW-5461)
  #define INVERT_Z_DIR false
#endif
#ifndef INVERT_E_DIR
  #define INVERT_E_DIR false
#endif

#define HAS_SDCARD_CONNECTION EITHER(TARGET_LPC1768, ADAFRUIT_GRAND_CENTRAL_M4)

#define HAS_LINEAR_E_JERK (DISABLED(CLASSIC_JERK) && ENABLED(LIN_ADVANCE))

#if X_HOME_DIR || (HAS_Y_AXIS && Y_HOME_DIR) || (HAS_Z_AXIS && Z_HOME_DIR) || (HAS_I_AXIS && I_HOME_DIR) || (HAS_J_AXIS && J_HOME_DIR) || (HAS_K_AXIS && K_HOME_DIR)
  #define HAS_ENDSTOPS 1
  #define COORDINATE_OKAY(N,L,H) WITHIN(N,L,H)
#else
  // #error dead code found by automatic analyses (see BFW-5461)
  #define COORDINATE_OKAY(N,L,H) true
#endif
