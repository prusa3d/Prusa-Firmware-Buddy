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

#include <option/has_local_bed.h>
#include <option/has_remote_bed.h>
#include <option/has_pause.h>
#include <option/has_indx.h>

/**
 * Conditionals_post.h
 * Defines that depend on configuration but are not editable.
 */

#if (ENABLED(CLASSIC_JERK))
  #define HAS_CLASSIC_JERK 1
#endif
#define HAS_CLASSIC_E_JERK HAS_CLASSIC_JERK

/**
 * Axis lengths and center
 */
#define X_MAX_LENGTH (X_MAX_POS - (X_MIN_POS))
#define Y_MAX_LENGTH (Y_MAX_POS - (Y_MIN_POS))
#define Z_MAX_LENGTH (Z_MAX_POS - (Z_MIN_POS))

// Defined only if the sanity-check is bypassed
#ifndef X_BED_SIZE
  // #error dead code found by automatic analyses (see BFW-5461)
  #define X_BED_SIZE X_MAX_LENGTH
#endif
#ifndef Y_BED_SIZE
  // #error dead code found by automatic analyses (see BFW-5461)
  #define Y_BED_SIZE Y_MAX_LENGTH
#endif

// Define center values for future use
#define _X_HALF_BED ((X_BED_SIZE) / 2)
#define _Y_HALF_BED ((Y_BED_SIZE) / 2)
#if ENABLED(BED_CENTER_AT_0_0)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define X_CENTER 0
  #define Y_CENTER 0
#else
  #define X_CENTER _X_HALF_BED
  #define Y_CENTER _Y_HALF_BED
#endif

// Get the linear boundaries of the bed
#define X_MIN_BED (X_CENTER - _X_HALF_BED)
#define X_MAX_BED (X_MIN_BED + X_BED_SIZE)
#define Y_MIN_BED (Y_CENTER - _Y_HALF_BED)
#define Y_MAX_BED (Y_MIN_BED + Y_BED_SIZE)

/**
 * CoreXY, CoreXZ, and CoreYZ - and their reverse
 */
#if EITHER(COREXY, COREYX)
  #define CORE_IS_XY 1
#endif
#define CORE_IS_XZ EITHER(COREXZ, COREZX)
#define CORE_IS_YZ EITHER(COREYZ, COREZY)
#if (CORE_IS_XY || CORE_IS_XZ || CORE_IS_YZ)
  #define IS_CORE 1
#endif
#if IS_CORE
  #if CORE_IS_XY
    #define CORE_AXIS_1 A_AXIS
    #define CORE_AXIS_2 B_AXIS
    #define NORMAL_AXIS Z_AXIS
  #elif CORE_IS_XZ
    // #error dead code found by automatic analyses (see BFW-5461)
    #define CORE_AXIS_1 A_AXIS
    #define NORMAL_AXIS Y_AXIS
    #define CORE_AXIS_2 C_AXIS
  #elif CORE_IS_YZ
    // #error dead code found by automatic analyses (see BFW-5461)
    #define NORMAL_AXIS X_AXIS
    #define CORE_AXIS_1 B_AXIS
    #define CORE_AXIS_2 C_AXIS
  #endif
  #if ANY(COREYX, COREZX, COREZY)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define CORESIGN(n) (-(n))
  #else
    #define CORESIGN(n) (n)
  #endif
#endif

/**
 * Set the home position based on settings or manual overrides
 */
#ifdef MANUAL_X_HOME_POS
  #define X_HOME_POS MANUAL_X_HOME_POS
#elif ENABLED(BED_CENTER_AT_0_0)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define X_HOME_POS (X_HOME_DIR < 0 ? X_MIN_POS : X_MAX_POS)
#else
  #define X_HOME_POS (X_HOME_DIR < 0 ? X_MIN_POS : X_MAX_POS)
#endif

#ifdef MANUAL_Y_HOME_POS
  // #error dead code found by automatic analyses (see BFW-5461)
  #define Y_HOME_POS MANUAL_Y_HOME_POS
#elif ENABLED(BED_CENTER_AT_0_0)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define Y_HOME_POS (Y_HOME_DIR < 0 ? Y_MIN_POS : Y_MAX_POS)
#else
  #define Y_HOME_POS (Y_HOME_DIR < 0 ? Y_MIN_POS : Y_MAX_POS)
#endif

#ifdef MANUAL_Z_HOME_POS
  #define Z_HOME_POS MANUAL_Z_HOME_POS
#else
  #define Z_HOME_POS (Z_HOME_DIR < 0 ? Z_MIN_POS : Z_MAX_POS)
#endif

#ifndef MESH_INSET
  #define MESH_INSET 0
#endif

/**
 * Safe Homing Options
 */
#if ENABLED(Z_SAFE_HOMING)
  #if ENABLED(AUTO_BED_LEVELING_UBL)
    // Home close to center so grid points have z heights very close to 0
    #define _SAFE_POINT(A) (((GRID_MAX_POINTS_##A) / 2) * (A##_BED_SIZE - 2 * (MESH_INSET)) / (GRID_MAX_POINTS_##A - 1) + MESH_INSET)
  #else
    #define _SAFE_POINT(A) A##_CENTER
  #endif
  #ifndef Z_SAFE_HOMING_X_POINT
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_SAFE_HOMING_X_POINT _SAFE_POINT(X)
  #endif
  #ifndef Z_SAFE_HOMING_Y_POINT
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_SAFE_HOMING_Y_POINT _SAFE_POINT(Y)
  #endif
#endif

/**
 * Host keep alive
 */
#ifndef DEFAULT_KEEPALIVE_INTERVAL
  // #error dead code found by automatic analyses (see BFW-5461)
  #define DEFAULT_KEEPALIVE_INTERVAL 2
#endif

/**
 * Set defaults for missing (newer) options
 */
#ifndef DISABLE_INACTIVE_X
  // #error dead code found by automatic analyses (see BFW-5461)
  #define DISABLE_INACTIVE_X DISABLE_X
#endif
#ifndef DISABLE_INACTIVE_Y
  // #error dead code found by automatic analyses (see BFW-5461)
  #define DISABLE_INACTIVE_Y DISABLE_Y
#endif
#ifndef DISABLE_INACTIVE_Z
  // #error dead code found by automatic analyses (see BFW-5461)
  #define DISABLE_INACTIVE_Z DISABLE_Z
#endif
#ifndef DISABLE_INACTIVE_E
  // #error dead code found by automatic analyses (see BFW-5461)
  #define DISABLE_INACTIVE_E DISABLE_E
#endif

/**
 * Power Supply Control
 */
#ifndef PSU_NAME
  #if ENABLED(PSU_CONTROL)
    #if PSU_ACTIVE_HIGH
      #define PSU_NAME "XBox"     // X-Box 360 (203W)
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      #define PSU_NAME "ATX"      // ATX style
    #endif
  #else
    #define PSU_NAME "Generic"    // No control
  #endif
#endif

#define HAS_POWER_SWITCH (ENABLED(PSU_CONTROL) && PIN_EXISTS(PS_ON))

/**
 * Temp Sensor defines
 */

#define ANY_TEMP_SENSOR_IS(n) (TEMP_SENSOR_0 == (n) || TEMP_SENSOR_BED == (n))

#if TEMP_SENSOR_0 > 0
  #define THERMISTOR_HEATER_0 TEMP_SENSOR_0
  #define HEATER_0_USES_THERMISTOR
#endif

#if defined(TEMP_1) || defined(TEMP_SENSOR_1) || defined(HEATER_1_MINTEMP) || defined(HEATER_1_MAXTEMP)
  #error Support for multiple local hotends removed
#endif

#if TEMP_SENSOR_BED > 0
  #define THERMISTORBED TEMP_SENSOR_BED
  #define HEATER_BED_USES_THERMISTOR
#else
  #undef BED_MINTEMP
  #undef BED_MAXTEMP
#endif


#if TEMP_SENSOR_HEATBREAK > 0
  #define THERMISTORHEATBREAK TEMP_SENSOR_HEATBREAK
  #define HEATBREAK_USES_THERMISTOR
#else
  #undef HEATBREAK_MINTEMP
  #undef HEATBREAK_MAXTEMP
#endif


#if TEMP_SENSOR_BOARD > 0
  #define THERMISTORBOARD TEMP_SENSOR_BOARD
  #define BOARD_USES_THERMISTOR
#else
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef BOARD_MINTEMP
  #undef BOARD_MINTEMP
#endif

/**
 * Driver Timings
 * NOTE: Driver timing order is longest-to-shortest duration.
 *       Preserve this ordering when adding new drivers.
 */

#define TRINAMICS (HAS_TRINAMIC || HAS_DRIVER(TMC2130_STANDALONE) || HAS_DRIVER(TMC2208_STANDALONE) || HAS_DRIVER(TMC2209_STANDALONE) || HAS_DRIVER(TMC26X_STANDALONE) || HAS_DRIVER(TMC2660_STANDALONE) || HAS_DRIVER(TMC5130_STANDALONE) || HAS_DRIVER(TMC5160_STANDALONE) || HAS_DRIVER(TMC2160_STANDALONE))

#ifndef MINIMUM_STEPPER_POST_DIR_DELAY
  #if HAS_DRIVER(TB6560)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_POST_DIR_DELAY 15000
  #elif HAS_DRIVER(TB6600)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_POST_DIR_DELAY 1500
  #elif HAS_DRIVER(DRV8825)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_POST_DIR_DELAY 650
  #elif HAS_DRIVER(LV8729)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_POST_DIR_DELAY 500
  #elif HAS_DRIVER(A5984)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_POST_DIR_DELAY 400
  #elif HAS_DRIVER(A4988)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_POST_DIR_DELAY 200
  #elif TRINAMICS
    #define MINIMUM_STEPPER_POST_DIR_DELAY 20
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_POST_DIR_DELAY 0   // Expect at least 10µS since one Stepper ISR must transpire
  #endif
#endif

#ifndef MINIMUM_STEPPER_PRE_DIR_DELAY
  #define MINIMUM_STEPPER_PRE_DIR_DELAY MINIMUM_STEPPER_POST_DIR_DELAY
#endif

#ifndef MINIMUM_STEPPER_PULSE
  #if HAS_DRIVER(TB6560)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_PULSE 30
  #elif HAS_DRIVER(TB6600)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_PULSE 3
  #elif HAS_DRIVER(DRV8825)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_PULSE 2
  #elif HAS_DRIVER(A4988) || HAS_DRIVER(A5984)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_PULSE 1
  #elif HAS_DRIVER(LV8729)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_PULSE 0
  #elif TRINAMICS
    #define MINIMUM_STEPPER_PULSE 0
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MINIMUM_STEPPER_PULSE 2
  #endif
#endif

#ifndef MAXIMUM_STEPPER_RATE
  #if HAS_DRIVER(TB6560)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MAXIMUM_STEPPER_RATE 15000
  #elif HAS_DRIVER(TB6600)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MAXIMUM_STEPPER_RATE 150000
  #elif HAS_DRIVER(LV8729)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MAXIMUM_STEPPER_RATE 200000
  #elif HAS_DRIVER(DRV8825)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MAXIMUM_STEPPER_RATE 250000
  #elif TRINAMICS
    #define MAXIMUM_STEPPER_RATE 400000
  #elif HAS_DRIVER(A4988)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MAXIMUM_STEPPER_RATE 500000
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    #define MAXIMUM_STEPPER_RATE 250000
  #endif
#endif

#if ENABLED(Z_TRIPLE_ENDSTOPS)
  // #error dead code found by automatic analyses (see BFW-5461)
  #if Z_HOME_DIR > 0
    // #error dead code found by automatic analyses (see BFW-5461)
    #if Z2_USE_ENDSTOP == _XMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MAX_ENDSTOP_INVERTING X_MIN_ENDSTOP_INVERTING
      #define Z2_MAX_PIN X_MIN_PIN
    #elif Z2_USE_ENDSTOP == _XMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MAX_ENDSTOP_INVERTING X_MAX_ENDSTOP_INVERTING
      #define Z2_MAX_PIN X_MAX_PIN
    #elif Z2_USE_ENDSTOP == _YMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MAX_ENDSTOP_INVERTING Y_MIN_ENDSTOP_INVERTING
      #define Z2_MAX_PIN Y_MIN_PIN
    #elif Z2_USE_ENDSTOP == _YMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MAX_ENDSTOP_INVERTING Y_MAX_ENDSTOP_INVERTING
      #define Z2_MAX_PIN Y_MAX_PIN
    #elif Z2_USE_ENDSTOP == _ZMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MAX_ENDSTOP_INVERTING Z_MIN_ENDSTOP_INVERTING
      #define Z2_MAX_PIN Z_MIN_PIN
    #elif Z2_USE_ENDSTOP == _ZMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MAX_ENDSTOP_INVERTING Z_MAX_ENDSTOP_INVERTING
      #define Z2_MAX_PIN Z_MAX_PIN
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MAX_ENDSTOP_INVERTING false
    #endif
    #define Z2_MIN_ENDSTOP_INVERTING false
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    #if Z2_USE_ENDSTOP == _XMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MIN_ENDSTOP_INVERTING X_MIN_ENDSTOP_INVERTING
      #define Z2_MIN_PIN X_MIN_PIN
    #elif Z2_USE_ENDSTOP == _XMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MIN_ENDSTOP_INVERTING X_MAX_ENDSTOP_INVERTING
      #define Z2_MIN_PIN X_MAX_PIN
    #elif Z2_USE_ENDSTOP == _YMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MIN_ENDSTOP_INVERTING Y_MIN_ENDSTOP_INVERTING
      #define Z2_MIN_PIN Y_MIN_PIN
    #elif Z2_USE_ENDSTOP == _YMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MIN_ENDSTOP_INVERTING Y_MAX_ENDSTOP_INVERTING
      #define Z2_MIN_PIN Y_MAX_PIN
    #elif Z2_USE_ENDSTOP == _ZMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MIN_ENDSTOP_INVERTING Z_MIN_ENDSTOP_INVERTING
      #define Z2_MIN_PIN Z_MIN_PIN
    #elif Z2_USE_ENDSTOP == _ZMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MIN_ENDSTOP_INVERTING Z_MAX_ENDSTOP_INVERTING
      #define Z2_MIN_PIN Z_MAX_PIN
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z2_MIN_ENDSTOP_INVERTING false
    #endif
    #define Z2_MAX_ENDSTOP_INVERTING false
  #endif
  #if Z_HOME_DIR > 0
    // #error dead code found by automatic analyses (see BFW-5461)
    #if Z3_USE_ENDSTOP == _XMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MAX_ENDSTOP_INVERTING X_MIN_ENDSTOP_INVERTING
      #define Z3_MAX_PIN X_MIN_PIN
    #elif Z3_USE_ENDSTOP == _XMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MAX_ENDSTOP_INVERTING X_MAX_ENDSTOP_INVERTING
      #define Z3_MAX_PIN X_MAX_PIN
    #elif Z3_USE_ENDSTOP == _YMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MAX_ENDSTOP_INVERTING Y_MIN_ENDSTOP_INVERTING
      #define Z3_MAX_PIN Y_MIN_PIN
    #elif Z3_USE_ENDSTOP == _YMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MAX_ENDSTOP_INVERTING Y_MAX_ENDSTOP_INVERTING
      #define Z3_MAX_PIN Y_MAX_PIN
    #elif Z3_USE_ENDSTOP == _ZMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MAX_ENDSTOP_INVERTING Z_MIN_ENDSTOP_INVERTING
      #define Z3_MAX_PIN Z_MIN_PIN
    #elif Z3_USE_ENDSTOP == _ZMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MAX_ENDSTOP_INVERTING Z_MAX_ENDSTOP_INVERTING
      #define Z3_MAX_PIN Z_MAX_PIN
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MAX_ENDSTOP_INVERTING false
    #endif
    #define Z3_MIN_ENDSTOP_INVERTING false
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    #if Z3_USE_ENDSTOP == _XMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MIN_ENDSTOP_INVERTING X_MIN_ENDSTOP_INVERTING
      #define Z3_MIN_PIN X_MIN_PIN
    #elif Z3_USE_ENDSTOP == _XMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MIN_ENDSTOP_INVERTING X_MAX_ENDSTOP_INVERTING
      #define Z3_MIN_PIN X_MAX_PIN
    #elif Z3_USE_ENDSTOP == _YMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MIN_ENDSTOP_INVERTING Y_MIN_ENDSTOP_INVERTING
      #define Z3_MIN_PIN Y_MIN_PIN
    #elif Z3_USE_ENDSTOP == _YMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MIN_ENDSTOP_INVERTING Y_MAX_ENDSTOP_INVERTING
      #define Z3_MIN_PIN Y_MAX_PIN
    #elif Z3_USE_ENDSTOP == _ZMIN_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MIN_ENDSTOP_INVERTING Z_MIN_ENDSTOP_INVERTING
      #define Z3_MIN_PIN Z_MIN_PIN
    #elif Z3_USE_ENDSTOP == _ZMAX_
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MIN_ENDSTOP_INVERTING Z_MAX_ENDSTOP_INVERTING
      #define Z3_MIN_PIN Z_MAX_PIN
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      #define Z3_MIN_ENDSTOP_INVERTING false
    #endif
    #define Z3_MAX_ENDSTOP_INVERTING false
  #endif
#endif

// Is an endstop plug used for the Z2 endstop or the bed probe?
#define IS_Z2_OR_PROBE(A,M) ( \
     (ENABLED(Z_TRIPLE_ENDSTOPS) && Z2_USE_ENDSTOP == _##A##M##_) \
  || (HAS_CUSTOM_PROBE_PIN && Z_MIN_PROBE_PIN == A##_##M##_PIN ) )

// Is an endstop plug used for the Z3 endstop or the bed probe?
#define IS_Z3_OR_PROBE(A,M) ( \
     (ENABLED(Z_TRIPLE_ENDSTOPS) && Z3_USE_ENDSTOP == _##A##M##_) \
  || (HAS_CUSTOM_PROBE_PIN && Z_MIN_PROBE_PIN == A##_##M##_PIN ) )

/**
 * Set ENDSTOPPULLUPS for active endstop switches
 */
#if ENABLED(ENDSTOPPULLUPS)
  #if ENABLED(USE_XMAX_PLUG)
    #define ENDSTOPPULLUP_XMAX
  #endif
  #if ENABLED(USE_YMAX_PLUG)
    #define ENDSTOPPULLUP_YMAX
  #endif
  #if ENABLED(USE_ZMAX_PLUG)
    #define ENDSTOPPULLUP_ZMAX
  #endif
  #if ENABLED(USE_XMIN_PLUG)
    #define ENDSTOPPULLUP_XMIN
  #endif
  #if ENABLED(USE_YMIN_PLUG)
    #define ENDSTOPPULLUP_YMIN
  #endif
  #if ENABLED(USE_ZMIN_PLUG)
    #define ENDSTOPPULLUP_ZMIN
  #endif
#endif

/**
 * Set ENDSTOPPULLDOWNS for active endstop switches
 */
#if ENABLED(ENDSTOPPULLDOWNS)
  // #error dead code found by automatic analyses (see BFW-5461)
  #if ENABLED(USE_XMAX_PLUG)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define ENDSTOPPULLDOWN_XMAX
  #endif
  #if ENABLED(USE_YMAX_PLUG)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define ENDSTOPPULLDOWN_YMAX
  #endif
  #if ENABLED(USE_ZMAX_PLUG)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define ENDSTOPPULLDOWN_ZMAX
  #endif
  #if ENABLED(USE_XMIN_PLUG)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define ENDSTOPPULLDOWN_XMIN
  #endif
  #if ENABLED(USE_YMIN_PLUG)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define ENDSTOPPULLDOWN_YMIN
  #endif
  #if ENABLED(USE_ZMIN_PLUG)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define ENDSTOPPULLDOWN_ZMIN
  #endif
#endif

/**
 * Shorthand for pin tests, used wherever needed
 */

// Steppers
#define HAS_X_ENABLE      (PIN_EXISTS(X_ENABLE))
#define HAS_X_DIR         (PIN_EXISTS(X_DIR))
#define HAS_X_STEP        (PIN_EXISTS(X_STEP))

#define HAS_Y_ENABLE      (PIN_EXISTS(Y_ENABLE))
#define HAS_Y_DIR         (PIN_EXISTS(Y_DIR))
#define HAS_Y_STEP        (PIN_EXISTS(Y_STEP))

#define HAS_Z_ENABLE      (PIN_EXISTS(Z_ENABLE))
#define HAS_Z_DIR         (PIN_EXISTS(Z_DIR))
#define HAS_Z_STEP        (PIN_EXISTS(Z_STEP))

#define HAS_Z2_ENABLE     (PIN_EXISTS(Z2_ENABLE))
#define HAS_Z2_DIR        (PIN_EXISTS(Z2_DIR))
#define HAS_Z2_STEP       (PIN_EXISTS(Z2_STEP))

#define HAS_Z3_ENABLE     (PIN_EXISTS(Z3_ENABLE))
#define HAS_Z3_DIR        (PIN_EXISTS(Z3_DIR))
#define HAS_Z3_STEP       (PIN_EXISTS(Z3_STEP))

// Extruder steppers
#define HAS_E0_ENABLE     (PIN_EXISTS(E0_ENABLE))
#define HAS_E0_DIR        (PIN_EXISTS(E0_DIR))
#define HAS_E0_STEP       (PIN_EXISTS(E0_STEP))

#define HAS_E1_ENABLE     (PIN_EXISTS(E1_ENABLE))
#define HAS_E1_DIR        (PIN_EXISTS(E1_DIR))
#define HAS_E1_STEP       (PIN_EXISTS(E1_STEP))

#define HAS_E2_ENABLE     (PIN_EXISTS(E2_ENABLE))
#define HAS_E2_DIR        (PIN_EXISTS(E2_DIR))
#define HAS_E2_STEP       (PIN_EXISTS(E2_STEP))

#define HAS_E3_ENABLE     (PIN_EXISTS(E3_ENABLE))
#define HAS_E3_DIR        (PIN_EXISTS(E3_DIR))
#define HAS_E3_STEP       (PIN_EXISTS(E3_STEP))

#define HAS_E4_ENABLE     (PIN_EXISTS(E4_ENABLE))
#define HAS_E4_DIR        (PIN_EXISTS(E4_DIR))
#define HAS_E4_STEP       (PIN_EXISTS(E4_STEP))

#define HAS_E5_ENABLE     (PIN_EXISTS(E5_ENABLE))
#define HAS_E5_DIR        (PIN_EXISTS(E5_DIR))
#define HAS_E5_STEP       (PIN_EXISTS(E5_STEP))

// Trinamic Stepper Drivers
#if HAS_TRINAMIC
  #define HAS_TMCX1X0       (HAS_DRIVER(TMC2130) || HAS_DRIVER(TMC2160) || HAS_DRIVER(TMC5130) || HAS_DRIVER(TMC5160))
  #define TMC_HAS_SPI       (HAS_TMCX1X0 || HAS_DRIVER(TMC2660))
  #define HAS_STALLGUARD    (HAS_TMCX1X0 || HAS_DRIVER(TMC2209) || HAS_DRIVER(TMC2660))
  #define HAS_STEALTHCHOP   (HAS_TMCX1X0 || HAS_TMC220x)

  #define STEALTHCHOP_ENABLED ANY(STEALTHCHOP_XY, STEALTHCHOP_Z, STEALTHCHOP_E)
  #define USE_SENSORLESS EITHER(SENSORLESS_HOMING, SENSORLESS_PROBING)
  #if (AXIS_HAS_STALLGUARD(X)  && defined(X_STALL_SENSITIVITY))
    #define X_SENSORLESS 1
  #endif
  #if (AXIS_HAS_STALLGUARD(Y)  && defined(Y_STALL_SENSITIVITY))
    #define Y_SENSORLESS 1
  #endif
  #define Z_SENSORLESS  (AXIS_HAS_STALLGUARD(Z)  && defined(Z_STALL_SENSITIVITY))
  #if (AXIS_HAS_STALLGUARD(Z2) && defined(Z2_STALL_SENSITIVITY))
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z2_SENSORLESS 1
  #endif
  #if (AXIS_HAS_STALLGUARD(Z3) && defined(Z3_STALL_SENSITIVITY))
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z3_SENSORLESS 1
  #endif
#endif

#define HAS_E_STEPPER_ENABLE (HAS_E_DRIVER(TMC2660) \
  || ( E0_ENABLE_PIN != X_ENABLE_PIN && E1_ENABLE_PIN != X_ENABLE_PIN   \
    && E0_ENABLE_PIN != Y_ENABLE_PIN && E1_ENABLE_PIN != Y_ENABLE_PIN ) \
)

// Endstops and bed probe
#define _HAS_STOP(A,M) (PIN_EXISTS(A##_##M) && !IS_Z2_OR_PROBE(A,M))
#define HAS_X_MIN _HAS_STOP(X,MIN)
#define HAS_X_MAX _HAS_STOP(X,MAX)
#define HAS_Y_MIN _HAS_STOP(Y,MIN)
#define HAS_Y_MAX _HAS_STOP(Y,MAX)
#define HAS_Z_MIN _HAS_STOP(Z,MIN)
#define HAS_Z_MAX _HAS_STOP(Z,MAX)
#define HAS_Z2_MIN (PIN_EXISTS(Z2_MIN))
#define HAS_Z2_MAX (PIN_EXISTS(Z2_MAX))
#define HAS_Z3_MIN (PIN_EXISTS(Z3_MIN))
#define HAS_Z3_MAX (PIN_EXISTS(Z3_MAX))
#define HAS_Z_MIN_PROBE_PIN (HAS_CUSTOM_PROBE_PIN && PIN_EXISTS(Z_MIN_PROBE))

// ADC Temp Sensors (Thermistor or Thermocouple with amplifier ADC interface)
#define HAS_ADC_TEST(P) (PIN_EXISTS(TEMP_##P) && TEMP_SENSOR_##P != 0)
#define HAS_TEMP_ADC_0 HAS_ADC_TEST(0)
#define HAS_TEMP_ADC_HEATBREAK HAS_ADC_TEST(HEATBREAK)
#define HAS_TEMP_ADC_BOARD HAS_ADC_TEST(BOARD)

// Not tied to HAS_ADC_XX, this needs to be true even if we have remote hotends
#define HAS_TEMP_HOTEND 1
#define HAS_TEMP_HEATBREAK HAS_TEMP_ADC_HEATBREAK
#define HAS_TEMP_BOARD HAS_TEMP_ADC_BOARD

#ifndef HAS_TEMP_HEATBREAK_CONTROL
  #define HAS_TEMP_HEATBREAK_CONTROL 0
#endif

// Shorthand for common combinations
#define HAS_HEATED_BED (HAS_LOCAL_BED() || HAS_REMOTE_BED())
#define HAS_TEMP_SENSOR (HAS_TEMP_HOTEND || HAS_HEATED_BED)

#if !HAS_TEMP_SENSOR
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef AUTO_REPORT_TEMPERATURES
#endif

// PID heating
#if !HAS_HEATED_BED
  #undef PIDTEMPBED
#endif
#define HAS_PID_HEATING EITHER(PIDTEMP, PIDTEMPBED)
#define HAS_PID_FOR_BOTH BOTH(PIDTEMP, PIDTEMPBED)

// Thermal protection
#define HAS_THERMALLY_PROTECTED_BED (HAS_HEATED_BED && ENABLED(THERMAL_PROTECTION_BED))
#if (ENABLED(THERMAL_PROTECTION_HOTENDS) && WATCH_TEMP_PERIOD > 0)
  #define WATCH_HOTENDS 1
#endif
#define WATCH_BED (HAS_THERMALLY_PROTECTED_BED && WATCH_BED_TEMP_PERIOD > 0)
#define WATCH_HEATBREAK (HAS_TEMP_HEATBREAK_CONTROL && ENABLED(THERMAL_PROTECTION_HEATBREAK) && WATCH_HEATBREAK_TEMP_PERIOD > 0)
#define WATCH_BOARD (HAS_TEMP_BOARD_CONTROL && ENABLED(THERMAL_PROTECTION_BOARD) && WATCH_BOARD_TEMP_PERIOD > 0)

// Other fans
#define HAS_FAN0 (PIN_EXISTS(FAN))
#define HAS_FAN1 (PIN_EXISTS(FAN1))
#define HAS_FAN2 (PIN_EXISTS(FAN2))

// User Interface
#define HAS_HOME        (PIN_EXISTS(HOME))
#define HAS_KILL        (PIN_EXISTS(KILL))
#define HAS_SUICIDE     (PIN_EXISTS(SUICIDE))
#define HAS_CASE_LIGHT  (PIN_EXISTS(CASE_LIGHT) && ENABLED(CASE_LIGHT_ENABLE))

// Digital control
#define HAS_STEPPER_RESET     (PIN_EXISTS(STEPPER_RESET))

#if !HAS_TEMP_SENSOR
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef AUTO_REPORT_TEMPERATURES
#endif

#define HAS_AUTO_REPORTING ENABLED(AUTO_REPORT_TEMPERATURES)

/**
 * This setting is also used by M109 when trying to calculate
 * a ballpark safe margin to prevent wait-forever situation.
 */
#ifndef EXTRUDE_MINTEMP
  // #error dead code found by automatic analyses (see BFW-5461)
  #define EXTRUDE_MINTEMP 170
#endif

#ifndef MIN_POWER
  #define MIN_POWER 0
#endif

/**
 * Heated chamber requires settings
 */
#if HAS_TEMP_HEATBREAK_CONTROL
  #ifndef MIN_HEATBREAK_POWER
    #define MIN_HEATBREAK_POWER 0
  #endif
  #ifndef MAX_HEATBREAK_POWER
    #define MAX_HEATBREAK_POWER 255
  #endif
#endif

/**
 * PWM fan
 */
#ifndef FAN_INVERTING
  #define FAN_INVERTING false
#endif

#if HAS_FAN2
  #error "FAN2 is not supported"
#elif HAS_FAN1
  #error "FAN1 is not supported"
#elif HAS_FAN0
  #define FAN_COUNT 1
#else
  #error "where is print fan?"
#endif

#define WRITE_FAN(n, v) WRITE(FAN##n##_PIN, (v) ^ FAN_INVERTING)

/**
 * MIN/MAX case light PWM scaling
 */
#if HAS_CASE_LIGHT
  // #error dead code found by automatic analyses (see BFW-5461)
  #ifndef CASE_LIGHT_MAX_PWM
    // #error dead code found by automatic analyses (see BFW-5461)
    #define CASE_LIGHT_MAX_PWM 255
  #elif !WITHIN(CASE_LIGHT_MAX_PWM, 1, 255)
    #error "CASE_LIGHT_MAX_PWM must be a value from 1 to 255."
  #endif
#endif

/**
 * Bed Probe dependencies
 */
#if HAS_BED_PROBE
  #if ENABLED(ENDSTOPPULLUPS) && HAS_Z_MIN_PROBE_PIN
    // #error dead code found by automatic analyses (see BFW-5461)
    #define ENDSTOPPULLUP_ZMIN_PROBE
  #endif
  #ifndef Z_PROBE_OFFSET_RANGE_MIN
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_PROBE_OFFSET_RANGE_MIN -20
  #endif
  #ifndef Z_PROBE_OFFSET_RANGE_MAX
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_PROBE_OFFSET_RANGE_MAX 20
  #endif
  #ifndef XY_PROBE_SPEED
    // #error dead code found by automatic analyses (see BFW-5461)
    #ifdef HOMING_FEEDRATE_XY
      // #error dead code found by automatic analyses (see BFW-5461)
      #define XY_PROBE_SPEED HOMING_FEEDRATE_XY
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      #define XY_PROBE_SPEED 4000
    #endif
  #endif
  #ifndef XY_PROBE_SPEED_INITIAL
    #define XY_PROBE_SPEED_INITIAL XY_PROBE_SPEED
  #endif
#else
  #undef NOZZLE_TO_PROBE_OFFSET
#endif

/**
 * Set granular options based on the specific type of leveling
 */
#define UBL_SEGMENTED   (ENABLED(AUTO_BED_LEVELING_UBL) && ANY(SEGMENT_LEVELED_MOVES))
#if ENABLED(AUTO_BED_LEVELING_UBL)
  #define HAS_LEVELING 1
#endif
#define HAS_AUTOLEVEL   (ENABLED(AUTO_BED_LEVELING_UBL))
#define HAS_MESH        ENABLED(AUTO_BED_LEVELING_UBL)
#define HAS_PROBING_PROCEDURE (ENABLED(AUTO_BED_LEVELING_UBL) || ENABLED(Z_MIN_PROBE_REPEATABILITY_TEST))

#if ENABLED(AUTO_BED_LEVELING_UBL)
  #undef LCD_BED_LEVELING
#endif

/**
 * Heater, Fan, and Probe interactions
 */
#define QUIET_PROBING (HAS_BED_PROBE && DELAY_BEFORE_PROBING > 0)

#if HAS_PAUSE() && !defined(FILAMENT_CHANGE_SLOW_LOAD_LENGTH)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define FILAMENT_CHANGE_SLOW_LOAD_LENGTH 0
#endif

#if EXTRUDERS > 1 && !defined(TOOLCHANGE_FIL_EXTRA_PRIME)
  #define TOOLCHANGE_FIL_EXTRA_PRIME 0
#endif

/**
 * Bed Probing rectangular bounds
 * These can be further constrained in code for Delta
 */
#ifndef MIN_PROBE_EDGE
  // #error dead code found by automatic analyses (see BFW-5461)
  #define MIN_PROBE_EDGE 0
#endif
#ifndef MIN_PROBE_EDGE_LEFT
  #define MIN_PROBE_EDGE_LEFT MIN_PROBE_EDGE
#endif
#ifndef MIN_PROBE_EDGE_RIGHT
  #define MIN_PROBE_EDGE_RIGHT MIN_PROBE_EDGE
#endif
#ifndef MIN_PROBE_EDGE_FRONT
  #define MIN_PROBE_EDGE_FRONT MIN_PROBE_EDGE
#endif
#ifndef MIN_PROBE_EDGE_BACK
  #define MIN_PROBE_EDGE_BACK MIN_PROBE_EDGE
#endif

#ifndef NOZZLE_TO_PROBE_OFFSET
  #define NOZZLE_TO_PROBE_OFFSET { 0, 0, 0 }
#endif

#if ENABLED(SEGMENT_LEVELED_MOVES) && !defined(LEVELED_SEGMENT_LENGTH)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define LEVELED_SEGMENT_LENGTH 5
#endif

/**
 * Default mesh area is an area with an inset margin on the print area.
 */
#if HAS_LEVELING
  // Boundaries for Cartesian probing based on set limits
  #if ENABLED(AUTO_BED_LEVELING_UBL)
    #define _MESH_MIN_X (_MAX(X_MIN_BED + MESH_INSET, X_MIN_POS))  // UBL is careful not to probe off the bed.  It does not
    #define _MESH_MIN_Y (_MAX(Y_MIN_BED + MESH_INSET, Y_MIN_POS))  // need NOZZLE_TO_PROBE_OFFSET in the mesh dimensions
    #define _MESH_MAX_X (_MIN(X_MAX_BED - (MESH_INSET), X_MAX_POS))
    #define _MESH_MAX_Y (_MIN(Y_MAX_BED - (MESH_INSET), Y_MAX_POS))
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    #define _MESH_MIN_X (_MAX(X_MIN_BED + MESH_INSET, X_MIN_POS + nozzle_to_probe_offset.x))
    #define _MESH_MIN_Y (_MAX(Y_MIN_BED + MESH_INSET, Y_MIN_POS + nozzle_to_probe_offset.y))
    #define _MESH_MAX_X (_MIN(X_MAX_BED - (MESH_INSET), X_MAX_POS + nozzle_to_probe_offset.x))
    #define _MESH_MAX_Y (_MIN(Y_MAX_BED - (MESH_INSET), Y_MAX_POS + nozzle_to_probe_offset.y))
  #endif

  // These may be overridden in Configuration.h if a smaller area is desired
  #ifndef MESH_MIN_X
    #define MESH_MIN_X _MESH_MIN_X
  #endif
  #ifndef MESH_MIN_Y
    #define MESH_MIN_Y _MESH_MIN_Y
  #endif
  #ifndef MESH_MAX_X
    #define MESH_MAX_X _MESH_MAX_X
  #endif
  #ifndef MESH_MAX_Y
    #define MESH_MAX_Y _MESH_MAX_Y
  #endif

#endif // AUTO_BED_LEVELING_UBL

/**
 * Z_HOMING_HEIGHT / Z_CLEARANCE_BETWEEN_PROBES
 */
#ifndef Z_HOMING_HEIGHT
  // #error dead code found by automatic analyses (see BFW-5461)
  #ifndef Z_CLEARANCE_BETWEEN_PROBES
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_HOMING_HEIGHT 0
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_HOMING_HEIGHT Z_CLEARANCE_BETWEEN_PROBES
  #endif
#endif

#if PROBE_SELECTED
  #ifndef Z_CLEARANCE_BETWEEN_PROBES
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_CLEARANCE_BETWEEN_PROBES Z_HOMING_HEIGHT
  #endif
  #ifndef Z_CLEARANCE_MULTI_PROBE
    // #error dead code found by automatic analyses (see BFW-5461)
    #define Z_CLEARANCE_MULTI_PROBE Z_CLEARANCE_BETWEEN_PROBES
  #endif
#endif

// Updated G92 behavior shifts the workspace
#define HAS_POSITION_SHIFT DISABLED(NO_WORKSPACE_OFFSETS)
// The home offset also shifts the coordinate space
#define HAS_HOME_OFFSET (DISABLED(NO_WORKSPACE_OFFSETS))
// Cumulative offset to workspace to save some calculation
#define HAS_WORKSPACE_OFFSET (HAS_POSITION_SHIFT && HAS_HOME_OFFSET)
// M206 sets the home offset for Cartesian machines
#define HAS_M206_COMMAND (HAS_HOME_OFFSET)

// Add commands that need sub-codes to this list
#define USE_GCODE_SUBCODES ANY(G38_PROBE_TARGET, PRINT_CHECKING_Q_CMDS)

// Number of VFAT entries used. Each entry has 13 UTF-16 characters
#if ENABLED(SCROLL_LONG_FILENAMES)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define MAX_VFAT_ENTRIES (5)
#else
  #define MAX_VFAT_ENTRIES (2)
#endif

// Defined here to catch the above defines
#if ENABLED(SDCARD_SORT_ALPHA)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define HAS_FOLDER_SORTING (FOLDER_SORTING || ENABLED(SDSORT_GCODE))
#endif

// If platform requires early initialization of watchdog to properly boot
#define EARLY_WATCHDOG (ENABLED(USE_WATCHDOG) && defined(ARDUINO_ARCH_SAM))

#if ENABLED(Z_TRIPLE_STEPPER_DRIVERS)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define Z_STEPPER_COUNT 3
#else
  #define Z_STEPPER_COUNT 1
#endif

#if !NUM_SERIAL
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef BAUD_RATE_GCODE
#endif

#if ENABLED(Z_STEPPER_ALIGN_KNOWN_STEPPER_POSITIONS)
  // #error dead code found by automatic analyses (see BFW-5461)
  #undef Z_STEPPER_ALIGN_AMP
#endif
#ifndef Z_STEPPER_ALIGN_AMP
  #define Z_STEPPER_ALIGN_AMP 1.0
#endif
