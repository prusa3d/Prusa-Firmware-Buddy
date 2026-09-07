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

#include <option/has_crash_detection.h>
#include <option/has_power_panic.h>
#include <option/has_print_sheet_detection.h>
#include <option/has_remote_accelerometer.h>
#include <option/has_toolchanger.h>
#include <option/has_gcode_compatibility.h>
#include <option/has_planner.h>
#include <option/has_mmu2.h>
#include <option/has_pause.h>

/**
 * SanityCheck.h
 *
 * Test configuration values for errors at compile-time.
 */

/**
 * Require gcc 4.7 or newer (first included with Arduino 1.6.8) for C++11 features.
 */
#if __cplusplus < 201103L
  #error "Marlin requires C++11 support (gcc >= 4.7, Arduino IDE >= 1.6.8). Please upgrade your toolchain."
#endif

/**
 * We try our best to include sanity checks for all changed configuration
 * directives because users have a tendency to use outdated config files with
 * the bleeding-edge source code, but sometimes this is not enough. This check
 * forces a minimum config file revision. Otherwise Marlin will not build.
 */
#define HEXIFY(H) _CAT(0x,H)
#if !defined(CONFIGURATION_H_VERSION) || HEXIFY(CONFIGURATION_H_VERSION) < HEXIFY(REQUIRED_CONFIGURATION_H_VERSION)
  #error "You are using an old Configuration.h file, update it before building Marlin."
#endif

#if !defined(CONFIGURATION_ADV_H_VERSION) || HEXIFY(CONFIGURATION_ADV_H_VERSION) < HEXIFY(REQUIRED_CONFIGURATION_ADV_H_VERSION)
  #error "You are using an old Configuration_adv.h file, update it before building Marlin."
#endif
#undef HEXIFY

/**
 * Warnings for old configurations
 */
#if !defined(X_BED_SIZE) || !defined(Y_BED_SIZE)
  #error "X_BED_SIZE and Y_BED_SIZE are now required! Please update your configuration."
#elif WATCH_TEMP_PERIOD > 500
  #error "WATCH_TEMP_PERIOD now uses seconds instead of milliseconds."
#elif DISABLED(THERMAL_PROTECTION_HOTENDS) && (defined(WATCH_TEMP_PERIOD) || defined(THERMAL_PROTECTION_PERIOD))
  #error "Thermal Runaway Protection for hotends is now enabled with THERMAL_PROTECTION_HOTENDS."
#elif DISABLED(THERMAL_PROTECTION_BED) && defined(THERMAL_PROTECTION_BED_PERIOD)
  #error "Thermal Runaway Protection for the bed is now enabled with THERMAL_PROTECTION_BED."
#elif (CORE_IS_XZ || CORE_IS_YZ) && ENABLED(Z_LATE_ENABLE)
  #error "Z_LATE_ENABLE can't be used with COREXZ, COREZX, COREYZ, or COREZY."
#elif defined(X_HOME_RETRACT_MM)
  #error "[XYZ]_HOME_RETRACT_MM settings have been renamed [XYZ]_HOME_BUMP_MM."
#elif defined(BTENABLED)
  #error "BTENABLED is now BLUETOOTH. Please update your configuration."
#elif defined(CUSTOM_MENDEL_NAME)
  #error "CUSTOM_MENDEL_NAME is now CUSTOM_MACHINE_NAME. Please update your configuration."
#elif defined(HAS_AUTOMATIC_VERSIONING)
  #error "HAS_AUTOMATIC_VERSIONING is now CUSTOM_VERSION_FILE. Please update your configuration."
#elif defined(USE_AUTOMATIC_VERSIONING)
  #error "USE_AUTOMATIC_VERSIONING is now CUSTOM_VERSION_FILE. Please update your configuration."
#elif defined(SDSLOW)
  #error "SDSLOW deprecated. Set SPI_SPEED to SPI_HALF_SPEED instead."
#elif defined(SDEXTRASLOW)
  #error "SDEXTRASLOW deprecated. Set SPI_SPEED to SPI_QUARTER_SPEED instead."
#elif defined(DISABLE_MAX_ENDSTOPS) || defined(DISABLE_MIN_ENDSTOPS)
  #error "DISABLE_MAX_ENDSTOPS and DISABLE_MIN_ENDSTOPS deprecated. Use individual USE_*_PLUG options instead."
#elif defined(EXTRUDER_OFFSET_X) || defined(EXTRUDER_OFFSET_Y)
  #error "EXTRUDER_OFFSET_[XY] is deprecated."
#elif defined(HOTEND_OFFSET_X) || defined(HOTEND_OFFSET_Y) || defined(HOTEND_OFFSET_Z)
  #error "HOTEND_OFFSET_[XYZ] is deprecated."
#elif defined(EXTRUDER_WATTS) || defined(BED_WATTS)
  #error "EXTRUDER_WATTS and BED_WATTS are deprecated. Remove them from your configuration."
#elif defined(SERVO_ENDSTOP_ANGLES)
  #error "SERVO_ENDSTOP_ANGLES is deprecated."
#elif defined(X_ENDSTOP_SERVO_NR) || defined(Y_ENDSTOP_SERVO_NR)
  #error "X_ENDSTOP_SERVO_NR and Y_ENDSTOP_SERVO_NR are deprecated and should be removed."
#elif defined(Z_ENDSTOP_SERVO_NR)
  #error "Z_ENDSTOP_SERVO_NR is now Z_PROBE_SERVO_NR. Please update your configuration."
#elif defined(DEFAULT_XYJERK)
  #error "DEFAULT_XYJERK is deprecated. Use DEFAULT_XJERK and DEFAULT_YJERK instead."
#elif defined(XY_TRAVEL_SPEED)
  #error "XY_TRAVEL_SPEED is deprecated. Use XY_PROBE_SPEED instead."
#elif defined(PROBE_SERVO_DEACTIVATION_DELAY)
  #error "PROBE_SERVO_DEACTIVATION_DELAY is deprecated."
#elif defined(SERVO_DEACTIVATION_DELAY)
  #error "SERVO_DEACTIVATION_DELAY is deprecated."
#elif defined(FILAMENT_CHANGE_X_POS) || defined(FILAMENT_CHANGE_Y_POS)
  #error "FILAMENT_CHANGE_[XY]_POS is now set with NOZZLE_PARK_POINT. Please update your configuration."
#elif defined(FILAMENT_CHANGE_Z_ADD)
  #error "FILAMENT_CHANGE_Z_ADD is now set with NOZZLE_PARK_POINT. Please update your configuration."
#elif defined(FILAMENT_CHANGE_XY_FEEDRATE)
  #error "FILAMENT_CHANGE_XY_FEEDRATE is now NOZZLE_PARK_XY_FEEDRATE. Please update your configuration."
#elif defined(FILAMENT_CHANGE_Z_FEEDRATE)
  #error "FILAMENT_CHANGE_Z_FEEDRATE is now NOZZLE_PARK_Z_FEEDRATE. Please update your configuration."
#elif defined(PAUSE_PARK_X_POS) || defined(PAUSE_PARK_Y_POS)
  #error "PAUSE_PARK_[XY]_POS is now set with NOZZLE_PARK_POINT. Please update your configuration."
#elif defined(PAUSE_PARK_Z_ADD)
  #error "PAUSE_PARK_Z_ADD is now set with NOZZLE_PARK_POINT. Please update your configuration."
#elif defined(PAUSE_PARK_XY_FEEDRATE)
  #error "PAUSE_PARK_XY_FEEDRATE is now NOZZLE_PARK_XY_FEEDRATE. Please update your configuration."
#elif defined(PAUSE_PARK_Z_FEEDRATE)
  #error "PAUSE_PARK_Z_FEEDRATE is now NOZZLE_PARK_Z_FEEDRATE. Please update your configuration."
#elif defined(FILAMENT_CHANGE_RETRACT_FEEDRATE) \
    || defined(FILAMENT_CHANGE_EXTRUDE_FEEDRATE) \
    || defined(ADVANCED_PAUSE_EXTRUDE_FEEDRATE) \
    || defined(PAUSE_PARK_RETRACT_FEEDRATE) \
    || defined(PARK_PAUSE_PRIME_FEEDRATE) \
    || defined(STANDARD_RETRACT_FEEDRATE) \
    || defined(STANDARD_DERETRACT_FEEDRATE) \
    || defined(FILAMENT_CHANGE_UNLOAD_FEEDRATE) \
    || defined(FILAMENT_CHANGE_SLOW_LOAD_FEEDRATE) \
    || defined(FILAMENT_CHANGE_FAST_LOAD_FEEDRATE) \
    || defined(FILAMENT_ASSISTED_FEEDRATE) \
    || defined(ADVANCED_PAUSE_PURGE_FEEDRATE)
  #error "Extruder feedrates have been consolidated into mapi/standard_feedrates Please update your configuration."
#elif defined(FILAMENT_CHANGE_RETRACT_LENGTH)
  #error "FILAMENT_CHANGE_RETRACT_LENGTH is now STANDARD_RETRACT_LENGTH. Please update your configuration."
#elif defined(FILAMENT_CHANGE_EXTRUDE_LENGTH)
  #error "FILAMENT_CHANGE_EXTRUDE_LENGTH is now ADVANCED_PAUSE_PURGE_LENGTH. Please update your configuration."
#elif defined(ADVANCED_PAUSE_EXTRUDE_LENGTH)
  #error "ADVANCED_PAUSE_EXTRUDE_LENGTH is now ADVANCED_PAUSE_PURGE_LENGTH. Please update your configuration."
#elif defined(PAUSE_PARK_RETRACT_LENGTH)
  #error "PAUSE_PARK_RETRACT_LENGTH is now STANDARD_RETRACT_LENGTH. Please update your configuration."
#elif defined(PLA_PREHEAT_HOTEND_TEMP)
  #error "PLA_PREHEAT_HOTEND_TEMP is now PREHEAT_1_TEMP_HOTEND. Please update your configuration."
#elif defined(PLA_PREHEAT_HPB_TEMP)
  #error "PLA_PREHEAT_HPB_TEMP is now PREHEAT_1_TEMP_BED. Please update your configuration."
#elif defined(PLA_PREHEAT_FAN_SPEED)
  #error "PLA_PREHEAT_FAN_SPEED is now PREHEAT_1_FAN_SPEED. Please update your configuration."
#elif defined(ABS_PREHEAT_HOTEND_TEMP)
  #error "ABS_PREHEAT_HOTEND_TEMP is now PREHEAT_2_TEMP_HOTEND. Please update your configuration."
#elif defined(ABS_PREHEAT_HPB_TEMP)
  #error "ABS_PREHEAT_HPB_TEMP is now PREHEAT_2_TEMP_BED. Please update your configuration."
#elif defined(ABS_PREHEAT_FAN_SPEED)
  #error "ABS_PREHEAT_FAN_SPEED is now PREHEAT_2_FAN_SPEED. Please update your configuration."
#elif defined(ENDSTOPS_ONLY_FOR_HOMING)
  #error "ENDSTOPS_ONLY_FOR_HOMING is deprecated. Use (disable) ENDSTOPS_ALWAYS_ON_DEFAULT instead."
#elif defined(HOMING_FEEDRATE)
  #error "HOMING_FEEDRATE is deprecated. Set individual rates with HOMING_FEEDRATE_(XY|Z|E) instead."
#elif defined(MANUAL_HOME_POSITIONS)
  #error "MANUAL_HOME_POSITIONS is deprecated. Set MANUAL_[XYZ]_HOME_POS as-needed instead."
#elif defined(PID_ADD_EXTRUSION_RATE)
  #error "PID_ADD_EXTRUSION_RATE is now PID_EXTRUSION_SCALING and is DISABLED by default. Are you sure you want to use this option? Please update your configuration."
#elif defined(Z_RAISE_BEFORE_HOMING)
  #error "Z_RAISE_BEFORE_HOMING is now Z_HOMING_HEIGHT. Please update your configuration."
#elif defined(MIN_Z_HEIGHT_FOR_HOMING)
  #error "MIN_Z_HEIGHT_FOR_HOMING is now Z_HOMING_HEIGHT. Please update your configuration."
#elif defined(Z_RAISE_BEFORE_PROBING) || defined(Z_RAISE_AFTER_PROBING)
  #error "Z_RAISE_(BEFORE|AFTER)_PROBING are deprecated. Use Z_CLEARANCE_DEPLOY_PROBE and Z_AFTER_PROBING instead."
#elif defined(Z_RAISE_PROBE_DEPLOY_STOW) || defined(Z_RAISE_BETWEEN_PROBINGS)
  #error "Z_RAISE_PROBE_DEPLOY_STOW and Z_RAISE_BETWEEN_PROBINGS are now Z_CLEARANCE_DEPLOY_PROBE and Z_CLEARANCE_BETWEEN_PROBES. Please update your configuration."
#elif defined(Z_PROBE_DEPLOY_HEIGHT) || defined(Z_PROBE_TRAVEL_HEIGHT)
  #error "Z_PROBE_DEPLOY_HEIGHT and Z_PROBE_TRAVEL_HEIGHT are now Z_CLEARANCE_DEPLOY_PROBE and Z_CLEARANCE_BETWEEN_PROBES. Please update your configuration."
#elif defined(MANUAL_BED_LEVELING)
  #error "MANUAL_BED_LEVELING is now LCD_BED_LEVELING. Please update your configuration."
#elif defined(MESH_HOME_SEARCH_Z)
  #error "MESH_HOME_SEARCH_Z is now LCD_PROBE_Z_RANGE. Please update your configuration."
#elif defined(MANUAL_PROBE_Z_RANGE)
  #error "MANUAL_PROBE_Z_RANGE is now LCD_PROBE_Z_RANGE. Please update your configuration."
#elif !defined(MIN_STEPS_PER_SEGMENT)
  #error Please replace "const int dropsegments" with "#define MIN_STEPS_PER_SEGMENT" (and increase by 1) in Configuration_adv.h.
#elif MIN_STEPS_PER_SEGMENT <= 0
  #error "MIN_STEPS_PER_SEGMENT must be at least 1. Please update your Configuration_adv.h."
#elif defined(PREVENT_DANGEROUS_EXTRUDE)
  #error "PREVENT_DANGEROUS_EXTRUDE is now PREVENT_COLD_EXTRUSION. Please update your configuration."
#elif defined(MESH_NUM_X_POINTS) || defined(MESH_NUM_Y_POINTS)
  #error "MESH_NUM_[XY]_POINTS is now GRID_MAX_POINTS_[XY]. Please update your configuration."
#elif defined(UBL_MESH_NUM_X_POINTS) || defined(UBL_MESH_NUM_Y_POINTS)
  #error "UBL_MESH_NUM_[XY]_POINTS is now GRID_MAX_POINTS_[XY]. Please update your configuration."
#elif defined(min_software_endstops) || defined(max_software_endstops)
  #error "(min|max)_software_endstops are now (MIN|MAX)_SOFTWARE_ENDSTOPS. Please update your configuration."
#elif defined(MIN_RETRACT)
  #error "MIN_RETRACT is now MIN_AUTORETRACT and MAX_AUTORETRACT. Please update your Configuration_adv.h."
#elif defined(UBL_MESH_INSET)
  #error "UBL_MESH_INSET is now just MESH_INSET. Please update your configuration."
#elif defined(UBL_MESH_MIN_X) || defined(UBL_MESH_MIN_Y) || defined(UBL_MESH_MAX_X) || defined(UBL_MESH_MAX_Y)
  #error "UBL_MESH_(MIN|MAX)_[XY] is now just MESH_(MIN|MAX)_[XY]. Please update your configuration."
#elif defined(LEFT_PROBE_BED_POSITION)
  #error "LEFT_PROBE_BED_POSITION has been replaced by MIN_PROBE_EDGE_LEFT. Please update your configuration."
#elif defined(RIGHT_PROBE_BED_POSITION)
  #error "RIGHT_PROBE_BED_POSITION has been replaced by MIN_PROBE_EDGE_RIGHT. Please update your configuration."
#elif defined(FRONT_PROBE_BED_POSITION)
  #error "FRONT_PROBE_BED_POSITION has been replaced by MIN_PROBE_EDGE_FRONT. Please update your configuration."
#elif defined(BACK_PROBE_BED_POSITION)
  #error "BACK_PROBE_BED_POSITION has been replaced by MIN_PROBE_EDGE_BACK. Please update your configuration."
#elif defined(ENABLE_MESH_EDIT_GFX_OVERLAY)
  #error "ENABLE_MESH_EDIT_GFX_OVERLAY is now MESH_EDIT_GFX_OVERLAY. Please update your configuration."
#elif defined(BABYSTEP_ZPROBE_GFX_REVERSE)
  #error "BABYSTEP_ZPROBE_GFX_REVERSE is now set by OVERLAY_GFX_REVERSE. Please update your configurations."
#elif defined(UBL_GRANULAR_SEGMENTATION_FOR_CARTESIAN)
  #error "UBL_GRANULAR_SEGMENTATION_FOR_CARTESIAN is now SEGMENT_LEVELED_MOVES. Please update your configuration."
#elif HAS_PID_HEATING && (defined(K1) || !defined(PID_K1))
  #error "K1 is now PID_K1. Please update your configuration."
#elif defined(PROBE_DOUBLE_TOUCH)
  #error "PROBE_DOUBLE_TOUCH is now MULTIPLE_PROBING. Please update your configuration."
#elif defined(HAVE_TMCDRIVER)
  #error "HAVE_TMCDRIVER is now [AXIS]_DRIVER_TYPE TMC26X. Please update your Configuration.h."
#elif defined(STEALTHCHOP)
  #error "STEALTHCHOP is now STEALTHCHOP_(XY|Z|E). Please update your Configuration_adv.h."
#elif defined(AUTOMATIC_CURRENT_CONTROL)
  #error "AUTOMATIC_CURRENT_CONTROL is now MONITOR_DRIVER_STATUS. Please update your configuration."
#elif defined(FILAMENT_CHANGE_LOAD_LENGTH)
  #error "FILAMENT_CHANGE_LOAD_LENGTH is now FILAMENT_CHANGE_FAST_LOAD_LENGTH. Please update your configuration."
#elif ENABLED(LEVEL_BED_CORNERS) && !defined(LEVEL_CORNERS_INSET)
  #error "LEVEL_BED_CORNERS requires a LEVEL_CORNERS_INSET value. Please update your Configuration.h."
#elif defined(BEZIER_JERK_CONTROL)
  #error "BEZIER_JERK_CONTROL is now S_CURVE_ACCELERATION. Please update your configuration."
#elif DISABLED(CLASSIC_JERK) && defined(JUNCTION_DEVIATION_FACTOR)
  #error "JUNCTION_DEVIATION_FACTOR is now JUNCTION_DEVIATION_MM. Please update your configuration."
#elif defined(JUNCTION_ACCELERATION_FACTOR)
  #error "JUNCTION_ACCELERATION_FACTOR is obsolete. Delete it from Configuration_adv.h."
#elif defined(JUNCTION_ACCELERATION)
  #error "JUNCTION_ACCELERATION is obsolete. Delete it from Configuration_adv.h."
#elif defined(RETRACT_ZLIFT)
  #error "RETRACT_ZLIFT is now RETRACT_ZRAISE. Please update your Configuration_adv.h."
#elif defined(SINGLENOZZLE_TOOLCHANGE_ZRAISE)
  #error "SINGLENOZZLE_TOOLCHANGE_ZRAISE is now TOOLCHANGE_ZRAISE. Please update your configuration."
#elif defined(SINGLENOZZLE_SWAP_LENGTH)
  #error "SINGLENOZZLE_SWAP_LENGTH is now TOOLCHANGE_FIL_SWAP_LENGTH. Please update your configuration."
#elif defined(SINGLENOZZLE_SWAP_RETRACT_SPEED)
  #error "SINGLENOZZLE_SWAP_RETRACT_SPEED is now TOOLCHANGE_FIL_SWAP_RETRACT_SPEED. Please update your configuration."
#elif defined(SINGLENOZZLE_SWAP_PRIME_SPEED)
  #error "SINGLENOZZLE_SWAP_PRIME_SPEED is now TOOLCHANGE_FIL_SWAP_PRIME_SPEED. Please update your configuration."
#elif defined(SINGLENOZZLE_SWAP_PARK)
  #error "SINGLENOZZLE_SWAP_PARK is now TOOLCHANGE_PARK. Please update your configuration."
#elif defined(SINGLENOZZLE_TOOLCHANGE_XY)
  #error "SINGLENOZZLE_TOOLCHANGE_XY is now TOOLCHANGE_PARK_XY. Please update your configuration."
#elif defined(SINGLENOZZLE_PARK_XY_FEEDRATE)
  #error "SINGLENOZZLE_PARK_XY_FEEDRATE is now TOOLCHANGE_PARK_XY_FEEDRATE. Please update your configuration."
#elif defined(G0_FEEDRATE) && G0_FEEDRATE == 0
  #error "G0_FEEDRATE is now used to set the G0 feedrate. Please update your configuration."
#elif defined(MBL_Z_STEP)
  #error "MBL_Z_STEP is now MESH_EDIT_Z_STEP. Please update your configuration."
#elif defined(TMC_Z_CALIBRATION)
  #error "TMC_Z_CALIBRATION has been deprecated in favor of Z_STEPPER_AUTO_ALIGN. Please update your configuration."
#elif defined(Z_MIN_PROBE_ENDSTOP)
  #error "Z_MIN_PROBE_ENDSTOP is no longer required. Please remove it from Configuration.h."
#elif defined(MENU_ITEM_CASE_LIGHT)
  #error "MENU_ITEM_CASE_LIGHT is now CASE_LIGHT_MENU. Please update your configuration."
#elif defined(ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED)
  #error "ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED is now SD_ABORT_ON_ENDSTOP_HIT. Please update your Configuration_adv.h."
#elif POWER_SUPPLY == 1
  #error "Replace POWER_SUPPLY 1 by enabling PSU_CONTROL and setting PSU_ACTIVE_HIGH to 'false'."
#elif POWER_SUPPLY == 2
  #error "Replace POWER_SUPPLY 2 by enabling PSU_CONTROL and setting PSU_ACTIVE_HIGH to 'true'."
#elif defined(POWER_SUPPLY)
  #error "POWER_SUPPLY is now obsolete. Please remove it from Configuration.h."
#elif defined(STRING_SPLASH_LINE1) || defined(STRING_SPLASH_LINE2)
  #error "STRING_SPLASH_LINE[12] are now obsolete. Please remove them from Configuration.h."
#elif defined(X_PROBE_OFFSET_FROM_EXTRUDER) || defined(Y_PROBE_OFFSET_FROM_EXTRUDER) || defined(Z_PROBE_OFFSET_FROM_EXTRUDER)
  #error "[XYZ]_PROBE_OFFSET_FROM_EXTRUDER is now NOZZLE_TO_PROBE_OFFSET. Please update your configuration."
#elif defined(MIN_PROBE_X) || defined(MIN_PROBE_Y) || defined(MAX_PROBE_X) || defined(MAX_PROBE_Y)
  #error "(MIN|MAX)_PROBE_[XY] are now calculated at runtime. Please remove them from Configuration.h."
#elif defined(Z_STEPPER_ALIGN_X) || defined(Z_STEPPER_ALIGN_X)
  #error "Z_STEPPER_ALIGN_X and Z_STEPPER_ALIGN_Y are now combined as Z_STEPPER_ALIGN_XY. Please update your Configuration_adv.h."
#elif defined(JUNCTION_DEVIATION)
  #error "JUNCTION_DEVIATION is no longer required. (See CLASSIC_JERK). Please remove it from Configuration.h."
#endif

/**
 * Marlin release, version and default string
 */
#ifndef SHORT_BUILD_VERSION
  #error "SHORT_BUILD_VERSION must be specified."
#elif !defined(DETAILED_BUILD_VERSION)
  #error "BUILD_VERSION must be specified."
#elif !defined(STRING_DISTRIBUTION_DATE)
  #error "STRING_DISTRIBUTION_DATE must be specified."
#elif !defined(PROTOCOL_VERSION)
  #error "PROTOCOL_VERSION must be specified."
#elif !defined(MACHINE_NAME)
  #error "MACHINE_NAME must be specified."
#elif !defined(SOURCE_CODE_URL)
  #error "SOURCE_CODE_URL must be specified."
#elif !defined(DEFAULT_MACHINE_UUID)
  #error "DEFAULT_MACHINE_UUID must be specified."
#elif !defined(WEBSITE_URL)
  #error "WEBSITE_URL must be specified."
#endif

/**
 * Serial
 */
#if !(defined(__AVR__) && defined(USBCON))
  #if ENABLED(SERIAL_XON_XOFF) && RX_BUFFER_SIZE < 1024
    #error "SERIAL_XON_XOFF requires RX_BUFFER_SIZE >= 1024 for reliable transfers without drops."
  #elif RX_BUFFER_SIZE && (RX_BUFFER_SIZE < 2 || !IS_POWER_OF_2(RX_BUFFER_SIZE))
    #error "RX_BUFFER_SIZE must be a power of 2 greater than 1."
  #elif TX_BUFFER_SIZE && (TX_BUFFER_SIZE < 2 || TX_BUFFER_SIZE > 256 || !IS_POWER_OF_2(TX_BUFFER_SIZE))
    #error "TX_BUFFER_SIZE must be 0 or a power of 2 between 1 and 256."
  #endif
#elif ANY(SERIAL_XON_XOFF, SERIAL_STATS_MAX_RX_QUEUED, SERIAL_STATS_DROPPED_RX)
  #error "SERIAL_XON_XOFF and SERIAL_STATS_* features not supported on USB-native AVR devices."
#endif

#if SERIAL_PORT > 7
  #error "Set SERIAL_PORT to the port on your board. Usually this is 0."
#endif

#if defined(SERIAL_PORT_2) && NUM_SERIAL < 2
  #error "SERIAL_PORT_2 is not supported for your MOTHERBOARD. Disable it to continue."
#endif

/**
 * Triple Stepper Drivers
 */
#if ENABLED(Z_TRIPLE_STEPPER_DRIVERS) && !(HAS_Z2_ENABLE && HAS_Z2_STEP && HAS_Z2_DIR && HAS_Z3_ENABLE && HAS_Z3_STEP && HAS_Z3_DIR)
  #error "Z_TRIPLE_STEPPER_DRIVERS requires Z3 pins (and two extra E plugs)."
#endif

/**
 * Validate that the bed size fits
 */
static_assert(X_MAX_LENGTH >= X_BED_SIZE, "Movement bounds (X_MIN_POS, X_MAX_POS) are too narrow to contain X_BED_SIZE.");
static_assert(Y_MAX_LENGTH >= Y_BED_SIZE, "Movement bounds (Y_MIN_POS, Y_MAX_POS) are too narrow to contain Y_BED_SIZE.");

/**
 * Granular software endstops (Marlin >= 1.1.7)
 */
#if ENABLED(MIN_SOFTWARE_ENDSTOPS) && DISABLED(MIN_SOFTWARE_ENDSTOP_Z)
  #if NONE(MIN_SOFTWARE_ENDSTOP_X, MIN_SOFTWARE_ENDSTOP_Y)
    #error "MIN_SOFTWARE_ENDSTOPS requires at least one of the MIN_SOFTWARE_ENDSTOP_[XYZ] options."
  #endif
#endif

#if ENABLED(MAX_SOFTWARE_ENDSTOPS) && DISABLED(MAX_SOFTWARE_ENDSTOP_Z)
  #if NONE(MAX_SOFTWARE_ENDSTOP_X, MAX_SOFTWARE_ENDSTOP_Y)
    #error "MAX_SOFTWARE_ENDSTOPS requires at least one of the MAX_SOFTWARE_ENDSTOP_[XYZ] options."
  #endif
#endif

#if !defined(TARGET_LPC1768) && ANY( \
    ENDSTOPPULLDOWNS, \
    ENDSTOPPULLDOWN_XMAX, ENDSTOPPULLDOWN_YMAX, \
    ENDSTOPPULLDOWN_ZMAX, ENDSTOPPULLDOWN_XMIN, \
    ENDSTOPPULLDOWN_YMIN, ENDSTOPPULLDOWN_ZMIN \
  )
  #error "PULLDOWN pin mode is not available on the selected board."
#endif

#if BOTH(ENDSTOPPULLUPS, ENDSTOPPULLDOWNS)
  #error "Enable only one of ENDSTOPPULLUPS or ENDSTOPPULLDOWNS."
#elif BOTH(ENDSTOPPULLUP_XMAX, ENDSTOPPULLDOWN_XMAX)
  #error "Enable only one of ENDSTOPPULLUP_X_MAX or ENDSTOPPULLDOWN_X_MAX."
#elif BOTH(ENDSTOPPULLUP_YMAX, ENDSTOPPULLDOWN_YMAX)
  #error "Enable only one of ENDSTOPPULLUP_Y_MAX or ENDSTOPPULLDOWN_Y_MAX."
#elif BOTH(ENDSTOPPULLUP_ZMAX, ENDSTOPPULLDOWN_ZMAX)
  #error "Enable only one of ENDSTOPPULLUP_Z_MAX or ENDSTOPPULLDOWN_Z_MAX."
#elif BOTH(ENDSTOPPULLUP_XMIN, ENDSTOPPULLDOWN_XMIN)
  #error "Enable only one of ENDSTOPPULLUP_X_MIN or ENDSTOPPULLDOWN_X_MIN."
#elif BOTH(ENDSTOPPULLUP_YMIN, ENDSTOPPULLDOWN_YMIN)
  #error "Enable only one of ENDSTOPPULLUP_Y_MIN or ENDSTOPPULLDOWN_Y_MIN."
#elif BOTH(ENDSTOPPULLUP_ZMIN, ENDSTOPPULLDOWN_ZMIN)
  #error "Enable only one of ENDSTOPPULLUP_Z_MIN or ENDSTOPPULLDOWN_Z_MIN."
#endif

#if ENABLED(SD_REPRINT_LAST_SELECTED_FILE)
  #error "SD_REPRINT_LAST_SELECTED_FILE currently requires a Marlin-native LCD menu."
#endif

/**
 * Custom Boot and Status screens
 */
#if EITHER(SHOW_CUSTOM_BOOTSCREEN, CUSTOM_STATUS_SCREEN_IMAGE)
  #error "Graphical LCD is required for SHOW_CUSTOM_BOOTSCREEN and CUSTOM_STATUS_SCREEN_IMAGE."
#endif

/**
 * SD File Sorting
 */
#if ENABLED(SDCARD_SORT_ALPHA)
  #if SDSORT_LIMIT > 256
    #error "SDSORT_LIMIT must be 256 or smaller."
  #elif SDSORT_LIMIT < 10
    #error "SDSORT_LIMIT should be greater than 9 to be useful."
  #elif DISABLED(SDSORT_USES_RAM)
    #if ENABLED(SDSORT_DYNAMIC_RAM)
      #error "SDSORT_DYNAMIC_RAM requires SDSORT_USES_RAM (which reads the directory into RAM)."
    #elif ENABLED(SDSORT_CACHE_NAMES)
      #error "SDSORT_CACHE_NAMES requires SDSORT_USES_RAM (which reads the directory into RAM)."
    #endif
  #endif

  #if ENABLED(SDSORT_CACHE_NAMES) && DISABLED(SDSORT_DYNAMIC_RAM)
    #if SDSORT_CACHE_VFATS < 2
      #error "SDSORT_CACHE_VFATS must be 2 or greater!"
    #elif SDSORT_CACHE_VFATS > MAX_VFAT_ENTRIES
      #undef SDSORT_CACHE_VFATS
      #define SDSORT_CACHE_VFATS MAX_VFAT_ENTRIES
      #warning "SDSORT_CACHE_VFATS was reduced to MAX_VFAT_ENTRIES!"
    #endif
  #endif
#endif

/**
 * Advanced Pause
 */
#if HAS_PAUSE()
  #if !HAS_RESUME_CONTINUE
    #error "HAS_PAUSE() currently requires an LCD controller or EMERGENCY_PARSER."
  #elif ENABLED(PREVENT_LENGTHY_EXTRUDE) && FILAMENT_CHANGE_UNLOAD_LENGTH > EXTRUDE_MAXLENGTH
    #error "FILAMENT_CHANGE_UNLOAD_LENGTH must be less than or equal to EXTRUDE_MAXLENGTH."
  #elif ENABLED(PREVENT_LENGTHY_EXTRUDE) && FILAMENT_CHANGE_SLOW_LOAD_LENGTH > EXTRUDE_MAXLENGTH
    #error "FILAMENT_CHANGE_SLOW_LOAD_LENGTH must be less than or equal to EXTRUDE_MAXLENGTH."
  #elif ENABLED(PREVENT_LENGTHY_EXTRUDE) && FILAMENT_CHANGE_FAST_LOAD_LENGTH > EXTRUDE_MAXLENGTH
    #error "FILAMENT_CHANGE_FAST_LOAD_LENGTH must be less than or equal to EXTRUDE_MAXLENGTH."
  #endif
#endif

constexpr float npp[] = XYZ_NOZZLE_PARK_POINT;
static_assert(COUNT(npp) == XYZ, "NOZZLE_PARK_POINT requires X, Y, and Z values.");

/**
 * Options only for EXTRUDERS > 1
 */
#if EXTRUDERS > 1
  #ifndef TOOLCHANGE_ZRAISE
    #error "TOOLCHANGE_ZRAISE required for EXTRUDERS > 1. Please update your Configuration_adv.h."
  #endif

#elif ENABLED(SINGLENOZZLE)
  #error "SINGLENOZZLE requires 2 or more EXTRUDERS."
#endif

/**
 * Kinematics
 */

/**
 * Allow only one kinematic type to be defined
 */
#if 1 < 0 \
  + ENABLED(COREXY) \
  + ENABLED(COREXZ) \
  + ENABLED(COREYZ) \
  + ENABLED(COREYX) \
  + ENABLED(COREZX) \
  + ENABLED(COREZY)
  #error "Please enable only one of COREXY, COREYX, COREXZ, COREZX, COREYZ, or COREZY."
#endif

/**
 * Probes
 */

/**
 * Allow only one probe option to be defined
 */
#if 1 < 0 \
  + ENABLED(FIX_MOUNTED_PROBE) \
  + ENABLED(TOUCH_MI_PROBE) \
  + ENABLED(SENSORLESS_PROBING)
  #error "Please enable only one probe option: SENSORLESS_PROBING, FIX_MOUNTED_PROBE, or TOUCH_MI_PROBE."
#endif

#if HAS_BED_PROBE
  /**
   * Touch-MI probe requirements
   */
  #if ENABLED(TOUCH_MI_PROBE)
    #if DISABLED(Z_SAFE_HOMING)
      #error "TOUCH_MI_PROBE requires Z_SAFE_HOMING."
    #elif !defined(TOUCH_MI_RETRACT_Z)
      #error "TOUCH_MI_PROBE requires TOUCH_MI_RETRACT_Z."
    #elif defined(Z_AFTER_PROBING)
      #error "TOUCH_MI_PROBE requires Z_AFTER_PROBING to be disabled."
    #elif Z_HOMING_HEIGHT < 10
      #error "TOUCH_MI_PROBE requires Z_HOMING_HEIGHT >= 10."
    #elif Z_MIN_PROBE_ENDSTOP_INVERTING
      #error "TOUCH_MI_PROBE requires Z_MIN_PROBE_ENDSTOP_INVERTING to be set to false."
    #elif !HAS_RESUME_CONTINUE
      #error "TOUCH_MI_PROBE currently requires an LCD controller or EMERGENCY_PARSER."
    #endif
  #endif

  /**
   * Require pin options and pins to be defined
   */
  #if ENABLED(SENSORLESS_PROBING)
    #if !AXIS_HAS_STALLGUARD(Z)
      #error "SENSORLESS_PROBING requires a TMC2130/2160/2209/5130/5160 driver on Z."
    #endif
  #elif ENABLED(Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN)
    #if DISABLED(USE_ZMIN_PLUG)
      #error "Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN requires USE_ZMIN_PLUG to be enabled."
    #elif !HAS_Z_MIN
      #error "Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN requires the Z_MIN_PIN to be defined."
    #elif Z_MIN_PROBE_ENDSTOP_INVERTING != Z_MIN_ENDSTOP_INVERTING
      #error "Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN requires Z_MIN_ENDSTOP_INVERTING to match Z_MIN_PROBE_ENDSTOP_INVERTING."
    #endif
  #elif !HAS_Z_MIN_PROBE_PIN
    #error "Z_MIN_PROBE_PIN must be defined if Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN is not enabled."
  #endif

  /**
   * Make sure Z raise values are set
   */
  #ifndef Z_CLEARANCE_DEPLOY_PROBE
    #error "You must define Z_CLEARANCE_DEPLOY_PROBE in your configuration."
  #elif !defined(Z_CLEARANCE_BETWEEN_PROBES)
    #error "You must define Z_CLEARANCE_BETWEEN_PROBES in your configuration."
  #elif Z_CLEARANCE_DEPLOY_PROBE < 0
    #error "Probes need Z_CLEARANCE_DEPLOY_PROBE >= 0."
  #elif Z_AFTER_PROBING < 0
    #error "Probes need Z_AFTER_PROBING >= 0."
  #endif

  #if MULTIPLE_PROBING || EXTRA_PROBING
    #if !MULTIPLE_PROBING
      #error "EXTRA_PROBING requires MULTIPLE_PROBING."
    #elif MULTIPLE_PROBING < 2
      #error "MULTIPLE_PROBING must be 2 or more."
    #elif MULTIPLE_PROBING <= EXTRA_PROBING
      #error "EXTRA_PROBING must be less than MULTIPLE_PROBING."
    #endif
  #endif

  #if Z_PROBE_LOW_POINT > 0
    #error "Z_PROBE_LOW_POINT must be less than or equal to 0."
  #endif

#else

  #if ENABLED(Z_MIN_PROBE_REPEATABILITY_TEST)
    #error "Z_MIN_PROBE_REPEATABILITY_TEST requires FIX_MOUNTED_PROBE."
  #endif

#endif

/**
 * Bed Leveling Requirements
 */

#if ENABLED(AUTO_BED_LEVELING_UBL)

  /**
   * Unified Bed Leveling
   */

  #if !WITHIN(GRID_MAX_POINTS_X, 3, 36) || !WITHIN(GRID_MAX_POINTS_Y, 3, 36)
    #error "GRID_MAX_POINTS_[XY] must be a whole number between 3 and 36."
  #elif !defined(RESTORE_LEVELING_AFTER_G28)
    #error "AUTO_BED_LEVELING_UBL used to enable RESTORE_LEVELING_AFTER_G28. To keep this behavior enable RESTORE_LEVELING_AFTER_G28. Otherwise define it as 'false'."
  #endif

#endif

#if HAS_MESH
  #if HAS_CLASSIC_JERK
    static_assert(DEFAULT_ZJERK > 0.1, "Low DEFAULT_ZJERK values are incompatible with mesh-based leveling.");
  #endif
#endif

#if ENABLED(MESH_EDIT_GFX_OVERLAY)
  #error "MESH_EDIT_GFX_OVERLAY requires AUTO_BED_LEVELING_UBL and a Graphical LCD."
#endif

/**
 * LCD_BED_LEVELING requirements
 */
#if ENABLED(LCD_BED_LEVELING)
  #error "LCD_BED_LEVELING requires a programmable LCD controller."
#endif

/**
 * Homing
 */
#if X_HOME_BUMP_MM < 0 || Y_HOME_BUMP_MM < 0 || Z_HOME_BUMP_MM < 0
  #error "[XYZ]_HOME_BUMP_MM must be greater than or equal to 0."
#endif

#if ENABLED(CODEPENDENT_XY_HOMING)
  #if ENABLED(QUICK_HOME)
    #error "QUICK_HOME is incompatible with CODEPENDENT_XY_HOMING."
  #endif
#endif

/**
 * Make sure Z_SAFE_HOMING point is reachable
 */
#if ENABLED(Z_SAFE_HOMING)
  static_assert(WITHIN(Z_SAFE_HOMING_X_POINT, X_MIN_POS, X_MAX_POS), "Z_SAFE_HOMING_X_POINT can't be reached by the nozzle.");
  static_assert(WITHIN(Z_SAFE_HOMING_Y_POINT, Y_MIN_POS, Y_MAX_POS), "Z_SAFE_HOMING_Y_POINT can't be reached by the nozzle.");
#endif // Z_SAFE_HOMING

/**
 * Make sure DISABLE_[XYZ] compatible with selected homing options
 */
#if ANY(DISABLE_X, DISABLE_Y, DISABLE_Z)
  #if EITHER(HOME_AFTER_DEACTIVATE, Z_SAFE_HOMING)
    #error "DISABLE_[XYZ] is not compatible with HOME_AFTER_DEACTIVATE or Z_SAFE_HOMING."
  #endif
#endif

/**
 * Case Light requirements
 */
#if ENABLED(CASE_LIGHT_ENABLE)
  #if !PIN_EXISTS(CASE_LIGHT)
    #error "CASE_LIGHT_ENABLE requires CASE_LIGHT_PIN to be defined."
  #elif CASE_LIGHT_PIN == FAN_PIN
    #error "CASE_LIGHT_PIN conflicts with FAN_PIN. Resolve before continuing."
  #endif
#endif

/**
 * Test Heater, Temp Sensor, and Extruder Pins; Sensor Type must also be set.
 */
#if ((defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)) && !PINS_EXIST(E0_STEP, E0_DIR))
  #error "E0_STEP_PIN or E0_DIR_PIN not defined for this board."
#elif ( !(defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)) && (!PINS_EXIST(E0_STEP, E0_DIR) || !HAS_E0_ENABLE))
  #error "E0_STEP_PIN, E0_DIR_PIN, or E0_ENABLE_PIN not defined for this board."
#endif

  #if E_STEPPERS
    #if !(PINS_EXIST(E0_STEP, E0_DIR) && HAS_E0_ENABLE)
      #error "E0_STEP_PIN, E0_DIR_PIN, or E0_ENABLE_PIN not defined for this board."
    #endif
    #if E_STEPPERS > 1
      #if !(PINS_EXIST(E1_STEP, E1_DIR) && HAS_E1_ENABLE)
        #error "E1_STEP_PIN, E1_DIR_PIN, or E1_ENABLE_PIN not defined for this board."
      #endif
      #if E_STEPPERS > 2
        #if !(PINS_EXIST(E2_STEP, E2_DIR) && HAS_E2_ENABLE)
          #error "E2_STEP_PIN, E2_DIR_PIN, or E2_ENABLE_PIN not defined for this board."
        #endif
        #if E_STEPPERS > 3
          #if !(PINS_EXIST(E3_STEP, E3_DIR) && HAS_E3_ENABLE)
            #error "E3_STEP_PIN, E3_DIR_PIN, or E3_ENABLE_PIN not defined for this board."
          #endif
          #if E_STEPPERS > 4
            #if !(PINS_EXIST(E4_STEP, E4_DIR) && HAS_E4_ENABLE)
              #error "E4_STEP_PIN, E4_DIR_PIN, or E4_ENABLE_PIN not defined for this board."
            #endif
            #if E_STEPPERS > 5
              #if !(PINS_EXIST(E5_STEP, E5_DIR) && HAS_E5_ENABLE)
                #error "E5_STEP_PIN, E5_DIR_PIN, or E5_ENABLE_PIN not defined for this board."
              #endif
            #endif // E_STEPPERS > 5
          #endif // E_STEPPERS > 4
        #endif // E_STEPPERS > 3
      #endif // E_STEPPERS > 2
    #endif // E_STEPPERS > 1
  #endif // E_STEPPERS

/**
 * Endstop Tests
 */

#define _PLUG_UNUSED_TEST(AXIS,PLUG) (DISABLED(USE_##PLUG##MIN_PLUG, USE_##PLUG##MAX_PLUG) && !(ENABLED(AXIS##_DUAL_ENDSTOPS) && WITHIN(AXIS##2_USE_ENDSTOP, _##PLUG##MAX_, _##PLUG##MIN_)))
#define _AXIS_PLUG_UNUSED_TEST(AXIS) (_PLUG_UNUSED_TEST(AXIS,X) && _PLUG_UNUSED_TEST(AXIS,Y) && _PLUG_UNUSED_TEST(AXIS,Z))

// At least 3 endstop plugs must be used
#if _AXIS_PLUG_UNUSED_TEST(X)
  #error "You must enable USE_XMIN_PLUG or USE_XMAX_PLUG."
#endif
#if _AXIS_PLUG_UNUSED_TEST(Y)
  #error "You must enable USE_YMIN_PLUG or USE_YMAX_PLUG."
#endif
#if _AXIS_PLUG_UNUSED_TEST(Z)
  #error "You must enable USE_ZMIN_PLUG or USE_ZMAX_PLUG."
#endif

// Delta and Cartesian use 3 homing endstops
#if X_HOME_DIR < 0 && DISABLED(USE_XMIN_PLUG)
  #error "Enable USE_XMIN_PLUG when homing X to MIN."
#elif X_HOME_DIR > 0 && DISABLED(USE_XMAX_PLUG)
  #error "Enable USE_XMAX_PLUG when homing X to MAX."
#elif Y_HOME_DIR < 0 && DISABLED(USE_YMIN_PLUG)
  #error "Enable USE_YMIN_PLUG when homing Y to MIN."
#elif Y_HOME_DIR > 0 && DISABLED(USE_YMAX_PLUG)
  #error "Enable USE_YMAX_PLUG when homing Y to MAX."
#endif

#if Z_HOME_DIR < 0 && DISABLED(USE_ZMIN_PLUG)
  #error "Enable USE_ZMIN_PLUG when homing Z to MIN."
#elif Z_HOME_DIR > 0 && DISABLED(USE_ZMAX_PLUG)
  #error "Enable USE_ZMAX_PLUG when homing Z to MAX."
#endif

#if ENABLED(Z_TRIPLE_ENDSTOPS)
  #if !Z2_USE_ENDSTOP
    #error "You must set Z2_USE_ENDSTOP with Z_TRIPLE_ENDSTOPS."
  #elif Z2_USE_ENDSTOP == _XMIN_ && DISABLED(USE_XMIN_PLUG)
    #error "USE_XMIN_PLUG is required when Z2_USE_ENDSTOP is _XMIN_."
  #elif Z2_USE_ENDSTOP == _XMAX_ && DISABLED(USE_XMAX_PLUG)
    #error "USE_XMAX_PLUG is required when Z2_USE_ENDSTOP is _XMAX_."
  #elif Z2_USE_ENDSTOP == _YMIN_ && DISABLED(USE_YMIN_PLUG)
    #error "USE_YMIN_PLUG is required when Z2_USE_ENDSTOP is _YMIN_."
  #elif Z2_USE_ENDSTOP == _YMAX_ && DISABLED(USE_YMAX_PLUG)
    #error "USE_YMAX_PLUG is required when Z2_USE_ENDSTOP is _YMAX_."
  #elif Z2_USE_ENDSTOP == _ZMIN_ && DISABLED(USE_ZMIN_PLUG)
    #error "USE_ZMIN_PLUG is required when Z2_USE_ENDSTOP is _ZMIN_."
  #elif Z2_USE_ENDSTOP == _ZMAX_ && DISABLED(USE_ZMAX_PLUG)
    #error "USE_ZMAX_PLUG is required when Z2_USE_ENDSTOP is _ZMAX_."
  #elif !HAS_Z2_MIN && !HAS_Z2_MAX
    #error "Z2_USE_ENDSTOP has been assigned to a nonexistent endstop!"
  #endif
  #if !Z3_USE_ENDSTOP
    #error "You must set Z3_USE_ENDSTOP with Z_TRIPLE_ENDSTOPS."
  #elif Z3_USE_ENDSTOP == _XMIN_ && DISABLED(USE_XMIN_PLUG)
    #error "USE_XMIN_PLUG is required when Z3_USE_ENDSTOP is _XMIN_."
  #elif Z3_USE_ENDSTOP == _XMAX_ && DISABLED(USE_XMAX_PLUG)
    #error "USE_XMAX_PLUG is required when Z3_USE_ENDSTOP is _XMAX_."
  #elif Z3_USE_ENDSTOP == _YMIN_ && DISABLED(USE_YMIN_PLUG)
    #error "USE_YMIN_PLUG is required when Z3_USE_ENDSTOP is _YMIN_."
  #elif Z3_USE_ENDSTOP == _YMAX_ && DISABLED(USE_YMAX_PLUG)
    #error "USE_YMAX_PLUG is required when Z3_USE_ENDSTOP is _YMAX_."
  #elif Z3_USE_ENDSTOP == _ZMIN_ && DISABLED(USE_ZMIN_PLUG)
    #error "USE_ZMIN_PLUG is required when Z3_USE_ENDSTOP is _ZMIN_."
  #elif Z3_USE_ENDSTOP == _ZMAX_ && DISABLED(USE_ZMAX_PLUG)
    #error "USE_ZMAX_PLUG is required when Z3_USE_ENDSTOP is _ZMAX_."
  #elif !HAS_Z3_MIN && !HAS_Z3_MAX
    #error "Z3_USE_ENDSTOP has been assigned to a nonexistent endstop!"
  #endif
#endif

/**
 * emergency-command parser
 */
#if ENABLED(EMERGENCY_PARSER) && defined(__AVR__) && defined(USBCON)
  #error "EMERGENCY_PARSER does not work on boards with AT90USB processors (USBCON)."
#endif

/**
 * G38 Probe Target
 */
#if ENABLED(G38_PROBE_TARGET)
  #if !HAS_BED_PROBE
    #error "G38_PROBE_TARGET requires a bed probe."
  #endif
#endif

/**
 * Make sure only one display is enabled
 */
#if 1 < 0 \
  + (ENABLED(EXTENSIBLE_UI) && DISABLED(IS_EXTUI))
  #error "Please select no more than one LCD controller option."
#endif

/**
 * Check existing CS pins against enabled TMC SPI drivers.
 */
#define INVALID_TMC_SPI(ST) (AXIS_HAS_SPI && !PIN_EXISTS(ST##_CS))
#if INVALID_TMC_SPI(X)
  #error "An SPI driven TMC driver on X requires X_CS_PIN."
#elif INVALID_TMC_SPI(Y)
  #error "An SPI driven TMC driver on Y requires Y_CS_PIN."
#elif INVALID_TMC_SPI(Z)
  #error "An SPI driven TMC driver on Z requires Z_CS_PIN."
#elif INVALID_TMC_SPI(Z2)
  #error "An SPI driven TMC driver on Z2 requires Z2_CS_PIN."
#elif INVALID_TMC_SPI(Z3)
  #error "An SPI driven TMC driver on Z3 requires Z3_CS_PIN."
#elif INVALID_TMC_SPI(E0)
  #error "An SPI driven TMC driver on E0 requires E0_CS_PIN."
#elif INVALID_TMC_SPI(E1)
  #error "An SPI driven TMC driver on E1 requires E1_CS_PIN."
#elif INVALID_TMC_SPI(E2)
  #error "An SPI driven TMC driver on E2 requires E2_CS_PIN."
#elif INVALID_TMC_SPI(E3)
  #error "An SPI driven TMC driver on E3 requires E3_CS_PIN."
#elif INVALID_TMC_SPI(E4)
  #error "An SPI driven TMC driver on E4 requires E4_CS_PIN."
#elif INVALID_TMC_SPI(E5)
  #error "An SPI driven TMC driver on E5 requires E5_CS_PIN."
#endif
#undef INVALID_TMC_SPI

/**
 * Check existing RX/TX pins against enable TMC UART drivers.
 */
#define INVALID_TMC_UART(ST) (AXIS_HAS_UART(ST) && !(defined(ST##_HARDWARE_SERIAL) || (PINS_EXIST(ST##_SERIAL_RX, ST##_SERIAL_TX))))
#if INVALID_TMC_UART(X)
  #error "TMC2208 or TMC2209 on X requires X_HARDWARE_SERIAL or X_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(Y)
  #error "TMC2208 or TMC2209 on Y requires Y_HARDWARE_SERIAL or Y_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(Z)
  #error "TMC2208 or TMC2209 on Z requires Z_HARDWARE_SERIAL or Z_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(Z2)
  #error "TMC2208 or TMC2209 on Z2 requires Z2_HARDWARE_SERIAL or Z2_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(Z3)
  #error "TMC2208 or TMC2209 on Z3 requires Z3_HARDWARE_SERIAL or Z3_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(E0)
  #error "TMC2208 or TMC2209 on E0 requires E0_HARDWARE_SERIAL or E0_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(E1)
  #error "TMC2208 or TMC2209 on E1 requires E1_HARDWARE_SERIAL or E1_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(E2)
  #error "TMC2208 or TMC2209 on E2 requires E2_HARDWARE_SERIAL or E2_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(E3)
  #error "TMC2208 or TMC2209 on E3 requires E3_HARDWARE_SERIAL or E3_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(E4)
  #error "TMC2208 or TMC2209 on E4 requires E4_HARDWARE_SERIAL or E4_SERIAL_(RX|TX)_PIN."
#elif INVALID_TMC_UART(E5)
  #error "TMC2208 or TMC2209 on E5 requires E5_HARDWARE_SERIAL or E5_SERIAL_(RX|TX)_PIN."
#endif
#undef INVALID_TMC_UART

/**
 * TMC2209 slave address values
 */
#define INVALID_TMC_ADDRESS(ST) static_assert(0 <= ST##_SLAVE_ADDRESS && ST##_SLAVE_ADDRESS <= 3, "TMC2209 slave address must be 0, 1, 2 or 3")
#if AXIS_DRIVER_TYPE_X(TMC2209)
  INVALID_TMC_ADDRESS(X);
#elif AXIS_DRIVER_TYPE_Y(TMC2209)
  INVALID_TMC_ADDRESS(Y);
#elif AXIS_DRIVER_TYPE_Z(TMC2209)
  INVALID_TMC_ADDRESS(Z);
#elif AXIS_DRIVER_TYPE_Z2(TMC2209)
  INVALID_TMC_ADDRESS(Z2);
#elif AXIS_DRIVER_TYPE_Z3(TMC2209)
  INVALID_TMC_ADDRESS(Z3);
#elif AXIS_DRIVER_TYPE_E0(TMC2209)
  INVALID_TMC_ADDRESS(E0);
#elif AXIS_DRIVER_TYPE_E1(TMC2209)
  INVALID_TMC_ADDRESS(E1);
#elif AXIS_DRIVER_TYPE_E2(TMC2209)
  INVALID_TMC_ADDRESS(E2);
#elif AXIS_DRIVER_TYPE_E3(TMC2209)
  INVALID_TMC_ADDRESS(E3);
#elif AXIS_DRIVER_TYPE_E4(TMC2209)
  INVALID_TMC_ADDRESS(E4);
#elif AXIS_DRIVER_TYPE_E5(TMC2209)
  INVALID_TMC_ADDRESS(E5);
#endif
#undef INVALID_TMC_ADDRESS

/**
 * TMC2208/2209 software UART and ENDSTOP_INTERRUPTS both use pin change interrupts (PCI)
 */
#if HAS_TMC220x && !defined(TARGET_LPC1768) && ENABLED(ENDSTOP_INTERRUPTS_FEATURE) && !( \
       defined(X_HARDWARE_SERIAL ) || defined(Y_HARDWARE_SERIAL ) \
    || defined(Z_HARDWARE_SERIAL ) || defined(Z2_HARDWARE_SERIAL) \
    || defined(Z3_HARDWARE_SERIAL) || defined(E0_HARDWARE_SERIAL) \
    || defined(E1_HARDWARE_SERIAL) || defined(E2_HARDWARE_SERIAL) \
    || defined(E3_HARDWARE_SERIAL) || defined(E4_HARDWARE_SERIAL) \
    || defined(E5_HARDWARE_SERIAL) )
  #error "Select hardware UART for TMC2208 to use both TMC2208 and ENDSTOP_INTERRUPTS_FEATURE."
#endif

/**
 * TMC2208/2209 software UART is only supported on AVR, LPC, STM32F1 and STM32F4
 */
#if HAS_TMC220x && !defined(__AVR__) && !defined(TARGET_LPC1768) && !defined(TARGET_STM32F1) && !defined(TARGET_STM32F4) && !( \
       defined(X_HARDWARE_SERIAL ) || defined(Y_HARDWARE_SERIAL ) \
    || defined(Z_HARDWARE_SERIAL ) || defined(Z2_HARDWARE_SERIAL) \
    || defined(Z3_HARDWARE_SERIAL) || defined(E0_HARDWARE_SERIAL) \
    || defined(E1_HARDWARE_SERIAL) || defined(E2_HARDWARE_SERIAL) \
    || defined(E3_HARDWARE_SERIAL) || defined(E4_HARDWARE_SERIAL) \
    || defined(E5_HARDWARE_SERIAL) )
  #error "TMC2208 Software Serial is supported only on AVR, LPC1768, STM32F1 and STM32F4 platforms."
#endif

#if ENABLED(SENSORLESS_HOMING)
  // Stall detection DIAG = HIGH : TMC2209
  // Stall detection DIAG = LOW  : TMC2130/TMC2160/TMC2660/TMC5130/TMC5160
  #define X_ENDSTOP_INVERTING !AXIS_DRIVER_TYPE(X,TMC2209)
  #define Y_ENDSTOP_INVERTING !AXIS_DRIVER_TYPE(Y,TMC2209)
  #define Z_ENDSTOP_INVERTING !AXIS_DRIVER_TYPE(Z,TMC2209)

  #if X_SENSORLESS && X_HOME_DIR < 0 && DISABLED(ENDSTOPPULLUPS, ENDSTOPPULLUP_XMIN)
    #error "SENSORLESS_HOMING requires ENDSTOPPULLUP_XMIN (or ENDSTOPPULLUPS) when homing to X_MIN."
  #elif X_SENSORLESS && X_HOME_DIR > 0 && DISABLED(ENDSTOPPULLUPS, ENDSTOPPULLUP_XMAX)
    #error "SENSORLESS_HOMING requires ENDSTOPPULLUP_XMAX (or ENDSTOPPULLUPS) when homing to X_MAX."
  #elif Y_SENSORLESS && Y_HOME_DIR < 0 && DISABLED(ENDSTOPPULLUPS, ENDSTOPPULLUP_YMIN)
    #error "SENSORLESS_HOMING requires ENDSTOPPULLUP_YMIN (or ENDSTOPPULLUPS) when homing to Y_MIN."
  #elif Y_SENSORLESS && Y_HOME_DIR > 0 && DISABLED(ENDSTOPPULLUPS, ENDSTOPPULLUP_YMAX)
    #error "SENSORLESS_HOMING requires ENDSTOPPULLUP_YMAX (or ENDSTOPPULLUPS) when homing to Y_MAX."
  #elif Z_SENSORLESS && Z_HOME_DIR < 0 && DISABLED(ENDSTOPPULLUPS, ENDSTOPPULLUP_ZMIN)
    #error "SENSORLESS_HOMING requires ENDSTOPPULLUP_ZMIN (or ENDSTOPPULLUPS) when homing to Z_MIN."
  #elif Z_SENSORLESS && Z_HOME_DIR > 0 && DISABLED(ENDSTOPPULLUPS, ENDSTOPPULLUP_ZMAX)
    #error "SENSORLESS_HOMING requires ENDSTOPPULLUP_ZMAX (or ENDSTOPPULLUPS) when homing to Z_MAX."
  #elif X_SENSORLESS && X_HOME_DIR < 0 && X_MIN_ENDSTOP_INVERTING != X_ENDSTOP_INVERTING
    #if X_ENDSTOP_INVERTING
      #error "SENSORLESS_HOMING requires X_MIN_ENDSTOP_INVERTING = true when homing to X_MIN."
    #else
      #error "SENSORLESS_HOMING requires X_MIN_ENDSTOP_INVERTING = false when homing TMC2209 to X_MIN."
    #endif
  #elif X_SENSORLESS && X_HOME_DIR > 0 && X_MAX_ENDSTOP_INVERTING != X_ENDSTOP_INVERTING
    #if X_ENDSTOP_INVERTING
      #error "SENSORLESS_HOMING requires X_MAX_ENDSTOP_INVERTING = true when homing to X_MAX."
    #else
      #error "SENSORLESS_HOMING requires X_MAX_ENDSTOP_INVERTING = false when homing TMC2209 to X_MAX."
    #endif
  #elif Y_SENSORLESS && Y_HOME_DIR < 0 && Y_MIN_ENDSTOP_INVERTING != Y_ENDSTOP_INVERTING
    #if Y_ENDSTOP_INVERTING
      #error "SENSORLESS_HOMING requires Y_MIN_ENDSTOP_INVERTING = true when homing to Y_MIN."
    #else
      #error "SENSORLESS_HOMING requires Y_MIN_ENDSTOP_INVERTING = false when homing TMC2209 to Y_MIN."
    #endif
  #elif Y_SENSORLESS && Y_HOME_DIR > 0 && Y_MAX_ENDSTOP_INVERTING != Y_ENDSTOP_INVERTING
    #if Y_ENDSTOP_INVERTING
      #error "SENSORLESS_HOMING requires Y_MAX_ENDSTOP_INVERTING = true when homing to Y_MAX."
    #else
      #error "SENSORLESS_HOMING requires Y_MAX_ENDSTOP_INVERTING = false when homing TMC2209 to Y_MAX."
    #endif
  #elif Z_SENSORLESS && Z_HOME_DIR < 0 && Z_MIN_ENDSTOP_INVERTING != Z_ENDSTOP_INVERTING
    #if Z_ENDSTOP_INVERTING
      #error "SENSORLESS_HOMING requires Z_MIN_ENDSTOP_INVERTING = true when homing to Z_MIN."
    #else
      #error "SENSORLESS_HOMING requires Z_MIN_ENDSTOP_INVERTING = false when homing TMC2209 to Z_MIN."
    #endif
  #elif Z_SENSORLESS && Z_HOME_DIR > 0 && Z_MAX_ENDSTOP_INVERTING != Z_ENDSTOP_INVERTING
    #if Z_ENDSTOP_INVERTING
      #error "SENSORLESS_HOMING requires Z_MAX_ENDSTOP_INVERTING = true when homing to Z_MAX."
    #else
      #error "SENSORLESS_HOMING requires Z_MAX_ENDSTOP_INVERTING = false when homing TMC2209 to Z_MAX."
    #endif
  #elif !(X_SENSORLESS || Y_SENSORLESS || Z_SENSORLESS)
    #error "SENSORLESS_HOMING requires a TMC stepper driver with StallGuard on X, Y, or Z axes."
  #endif

  #undef X_ENDSTOP_INVERTING
  #undef Y_ENDSTOP_INVERTING
  #undef Z_ENDSTOP_INVERTING
#endif

// Sensorless probing requirements
#if ENABLED(SENSORLESS_PROBING)
  #if !Z_SENSORLESS
    #error "SENSORLESS_PROBING requires a TMC stepper driver with StallGuard on Z."
  #endif
#endif

#if EITHER(SENSORLESS_HOMING, SENSORLESS_PROBING)
  #if !defined(X_STALL_SENSITIVITY) && !defined(Y_STALL_SENSITIVITY) && !defined(Z_STALL_SENSITIVITY)
    #error "Sensorless features (homing, probing) enabled but no sensitivity was set for specified drivers."
  #endif
#endif

// Sensorless homing is required for both combined steppers in an H-bot
#if CORE_IS_XY && X_SENSORLESS != Y_SENSORLESS
  #error "CoreXY requires both X and Y to use sensorless homing if either does."
#elif CORE_IS_XZ && X_SENSORLESS != Z_SENSORLESS
  #error "CoreXZ requires both X and Z to use sensorless homing if either does."
#elif CORE_IS_YZ && Y_SENSORLESS != Z_SENSORLESS
  #error "CoreYZ requires both Y and Z to use sensorless homing if either does."
#endif

// Other TMC feature requirements
#if ENABLED(HYBRID_THRESHOLD) && !STEALTHCHOP_ENABLED
  #error "Enable STEALTHCHOP_(XY|Z|E) to use HYBRID_THRESHOLD."
#elif ENABLED(SENSORLESS_HOMING) && !HAS_STALLGUARD
  #error "SENSORLESS_HOMING requires TMC2130, TMC2160, TMC2209, TMC2660, or TMC5160 stepper drivers."
#elif ENABLED(SENSORLESS_PROBING) && !HAS_STALLGUARD
  #error "SENSORLESS_PROBING requires TMC2130, TMC2160, TMC2209, TMC2660, or TMC5160 stepper drivers."
#elif STEALTHCHOP_ENABLED && !HAS_STEALTHCHOP
  #error "STEALTHCHOP requires TMC2130, TMC2160, TMC2208, TMC2209, or TMC5160 stepper drivers."
#endif

#define IN_CHAIN(A) (A##_CHAIN_POS > 0)
// TMC SPI Chaining
#if IN_CHAIN(X) || IN_CHAIN(Y) || IN_CHAIN(Z) || IN_CHAIN(Z2) || IN_CHAIN(Z3) || IN_CHAIN(E0) || IN_CHAIN(E1) || IN_CHAIN(E2) || IN_CHAIN(E3) || IN_CHAIN(E4) || IN_CHAIN(E5)
  #if  (IN_CHAIN(X)  && !PIN_EXISTS(X_CS) ) || (IN_CHAIN(Y)  && !PIN_EXISTS(Y_CS) ) \
    || (IN_CHAIN(Z)  && !PIN_EXISTS(Z_CS) ) || (IN_CHAIN(Z2) && !PIN_EXISTS(Z2_CS)) \
    || (IN_CHAIN(Z3) && !PIN_EXISTS(Z3_CS)) || (IN_CHAIN(E0) && !PIN_EXISTS(E0_CS)) \
    || (IN_CHAIN(E1) && !PIN_EXISTS(E1_CS)) || (IN_CHAIN(E2) && !PIN_EXISTS(E2_CS)) \
    || (IN_CHAIN(E3) && !PIN_EXISTS(E3_CS)) || (IN_CHAIN(E4) && !PIN_EXISTS(E4_CS)) \
    || (IN_CHAIN(E5) && !PIN_EXISTS(E5_CS))
    #error "All chained TMC drivers need a CS pin."
  #else
    #if IN_CHAIN(X)
      #define CS_COMPARE X_CS_PIN
    #elif IN_CHAIN(Y)
      #define CS_COMPARE Y_CS_PIN
    #elif IN_CHAIN(Z)
      #define CS_COMPARE Z_CS_PIN
    #elif IN_CHAIN(Z2)
      #define CS_COMPARE Z2_CS_PIN
    #elif IN_CHAIN(Z3)
      #define CS_COMPARE Z3_CS_PIN
    #elif IN_CHAIN(E0)
      #define CS_COMPARE E0_CS_PIN
    #elif IN_CHAIN(E1)
      #define CS_COMPARE E1_CS_PIN
    #elif IN_CHAIN(E2)
      #define CS_COMPARE E2_CS_PIN
    #elif IN_CHAIN(E3)
      #define CS_COMPARE E3_CS_PIN
    #elif IN_CHAIN(E4)
      #define CS_COMPARE E4_CS_PIN
    #elif IN_CHAIN(E5)
      #define CS_COMPARE E5_CS_PIN
    #endif
    #if  (IN_CHAIN(X)  && X_CS_PIN  != CS_COMPARE) || (IN_CHAIN(Y)  && Y_CS_PIN  != CS_COMPARE) \
      || (IN_CHAIN(Z)  && Z_CS_PIN  != CS_COMPARE) || (IN_CHAIN(Z2) && Z2_CS_PIN != CS_COMPARE) \
      || (IN_CHAIN(Z3) && Z3_CS_PIN != CS_COMPARE) || (IN_CHAIN(E0) && E0_CS_PIN != CS_COMPARE) \
      || (IN_CHAIN(E1) && E1_CS_PIN != CS_COMPARE) || (IN_CHAIN(E2) && E2_CS_PIN != CS_COMPARE) \
      || (IN_CHAIN(E3) && E3_CS_PIN != CS_COMPARE) || (IN_CHAIN(E4) && E4_CS_PIN != CS_COMPARE) \
      || (IN_CHAIN(E5) && E5_CS_PIN != CS_COMPARE)
      #error "All chained TMC drivers must use the same CS pin."
    #endif
  #endif
  #undef CS_COMPARE
#endif
#undef IN_CHAIN

/**
 * Check per-axis config defaults for errors.
 *
 * DEFAULT_MAX_FEEDRATE / DEFAULT_MAX_ACCELERATION (and the *_EDIT_VALUES below) are brace-init
 * lists {X, Y, Z, E, ...}. Each is materialized as a constexpr array so we can compile-time check
 * that it has the right number of elements and that every value is positive.
 */
template <size_t N>
constexpr bool sanity_all_positive(const float (&arr)[N]) {
  for (const float v : arr) {
    if (v <= 0) {
      return false;
    }
  }
  return true;
}

constexpr float sanity_max_feedrate[]     = DEFAULT_MAX_FEEDRATE,
                sanity_max_acceleration[] = DEFAULT_MAX_ACCELERATION;

// Axis steps/mm are individual per-axis macros (no bundled array). Z/E0 are always defined.
// X/Y naming differs by printer: most define _X/_Y, but belt printers (CoreOne/CoreOneL) omit those
// and define the per-belt 2GT/15GT variants instead (X/Y is resolved at runtime). Check whichever
// pair the printer actually uses, and error out if a printer defines neither.
static_assert(DEFAULT_AXIS_STEPS_PER_UNIT_Z > 0 && DEFAULT_AXIS_STEPS_PER_UNIT_E0 > 0,
              "DEFAULT_AXIS_STEPS_PER_UNIT_Z/_E0 must be positive.");
#if defined(DEFAULT_AXIS_STEPS_PER_UNIT_X)
static_assert(DEFAULT_AXIS_STEPS_PER_UNIT_X > 0 && DEFAULT_AXIS_STEPS_PER_UNIT_Y > 0,
              "DEFAULT_AXIS_STEPS_PER_UNIT_X/_Y must be positive.");
#elif defined(AXIS_STEPS_PER_UNIT_2GT_XY)
static_assert(AXIS_STEPS_PER_UNIT_2GT_XY > 0 && AXIS_STEPS_PER_UNIT_15GT_XY > 0,
              "AXIS_STEPS_PER_UNIT_2GT_XY/_15GT_XY must be positive.");
#else
  #error "No X/Y steps/mm default defined for this printer."
#endif

static_assert(COUNT(sanity_max_feedrate) >= XYZE,   "DEFAULT_MAX_FEEDRATE requires X, Y, Z and E elements.");
static_assert(COUNT(sanity_max_feedrate) <= XYZE_N, "DEFAULT_MAX_FEEDRATE has too many elements. (Did you forget to enable DISTINCT_E_FACTORS?)");
static_assert(sanity_all_positive(sanity_max_feedrate), "DEFAULT_MAX_FEEDRATE values must be positive.");

static_assert(COUNT(sanity_max_acceleration) >= XYZE,   "DEFAULT_MAX_ACCELERATION requires X, Y, Z and E elements.");
static_assert(COUNT(sanity_max_acceleration) <= XYZE_N, "DEFAULT_MAX_ACCELERATION has too many elements. (Did you forget to enable DISTINCT_E_FACTORS?)");
static_assert(sanity_all_positive(sanity_max_acceleration), "DEFAULT_MAX_ACCELERATION values must be positive.");

// CoreXY drives X and Y with the same two motors, so their motion limits must match.
// These arrays are {X, Y, Z, E}: index 0 = X, 1 = Y (the axis enum is not defined yet here).
#if CORE_IS_XY
static_assert(sanity_max_feedrate[0] == sanity_max_feedrate[1],
              "CoreXY requires equal X/Y max feedrate (DEFAULT_MAX_FEEDRATE).");
static_assert(sanity_max_acceleration[0] == sanity_max_acceleration[1],
              "CoreXY requires equal X/Y max acceleration (DEFAULT_MAX_ACCELERATION).");
  #ifdef DEFAULT_XJERK // classic jerk only; CoreOne/CoreOneL use junction deviation, so inactive there
static_assert(DEFAULT_XJERK == DEFAULT_YJERK,
              "CoreXY requires equal X/Y jerk (DEFAULT_XJERK/DEFAULT_YJERK).");
  #endif
#endif

#if ENABLED(LIMITED_MAX_ACCEL_EDITING)
  #ifdef MAX_ACCEL_EDIT_VALUES
    constexpr float sanity_max_accel_edit[] = MAX_ACCEL_EDIT_VALUES;
    static_assert(COUNT(sanity_max_accel_edit) >= XYZE, "MAX_ACCEL_EDIT_VALUES requires X, Y, Z and E elements.");
    static_assert(COUNT(sanity_max_accel_edit) <= XYZE, "MAX_ACCEL_EDIT_VALUES has too many elements. X, Y, Z and E elements only.");
    static_assert(sanity_all_positive(sanity_max_accel_edit), "MAX_ACCEL_EDIT_VALUES values must be positive.");
  #endif
#endif

#if ENABLED(LIMITED_MAX_FR_EDITING)
  #ifdef MAX_FEEDRATE_EDIT_VALUES
    constexpr float sanity_max_feedrate_edit[] = MAX_FEEDRATE_EDIT_VALUES;
    static_assert(COUNT(sanity_max_feedrate_edit) >= XYZE, "MAX_FEEDRATE_EDIT_VALUES requires X, Y, Z and E elements.");
    static_assert(COUNT(sanity_max_feedrate_edit) <= XYZE, "MAX_FEEDRATE_EDIT_VALUES has too many elements. X, Y, Z and E elements only.");
    static_assert(sanity_all_positive(sanity_max_feedrate_edit), "MAX_FEEDRATE_EDIT_VALUES values must be positive.");
  #endif
#endif

#if ENABLED(LIMITED_JERK_EDITING)
  #ifdef MAX_JERK_EDIT_VALUES
    constexpr float sanity_max_jerk_edit[] = MAX_JERK_EDIT_VALUES;
    static_assert(COUNT(sanity_max_jerk_edit) >= XYZE, "MAX_JERK_EDIT_VALUES requires X, Y, Z and E elements.");
    static_assert(COUNT(sanity_max_jerk_edit) <= XYZE, "MAX_JERK_EDIT_VALUES has too many elements. X, Y, Z and E elements only.");
    static_assert(sanity_all_positive(sanity_max_jerk_edit), "MAX_JERK_EDIT_VALUES values must be positive.");
  #endif
#endif

#if HAS_PLANNER()
  #if !BLOCK_BUFFER_SIZE || !IS_POWER_OF_2(BLOCK_BUFFER_SIZE)
    #error "BLOCK_BUFFER_SIZE must be a power of 2."
  #endif
#else
  #ifdef BLOCK_BUFFER_SIZE
    #error "BLOCK_BUFFER_SIZE should not be defined without a planner"
  #endif
#endif

#if HAS_POWER_PANIC() && !HAS_CRASH_DETECTION()
  #error "HAS_POWER_PANIC requires HAS_CRASH_DETECTION."
#endif

#if ENABLED(AXIS_MEASURE) && !HAS_CRASH_DETECTION()
  #error "AXIS_MEASURE requires HAS_CRASH_DETECTION."
#endif

#if HAS_PRINT_SHEET_DETECTION() && DISABLED(Z_SAFE_HOMING)
  #error "HAS_PRINT_SHEET_DETECTION requires Z_SAFE_HOMING."
#endif

#if ENABLED(Z_STEPPER_AUTO_ALIGN)

  #if !ENABLED(Z_TRIPLE_STEPPER_DRIVERS)
    #error "Z_STEPPER_AUTO_ALIGN requires Z_TRIPLE_STEPPER_DRIVERS."
  #elif !HAS_BED_PROBE
    #error "Z_STEPPER_AUTO_ALIGN requires a Z-bed probe."
  #endif

  #if ENABLED(Z_STEPPER_ALIGN_KNOWN_STEPPER_POSITIONS)
    #if DISABLED(Z_TRIPLE_STEPPER_DRIVERS)
      #error "Z_STEPPER_ALIGN_KNOWN_STEPPER_POSITIONS requires Z_TRIPLE_STEPPER_DRIVERS."
    #endif
    constexpr float sanity_arr_screw_xy[][2] = Z_STEPPER_ALIGN_STEPPER_XY;
    static_assert(
      COUNT(sanity_arr_screw_xy) == Z_STEPPER_COUNT,
      "Z_STEPPER_ALIGN_STEPPER_XY requires three {X,Y} entries (one per Z stepper)."
    );
  #endif

#endif

#if ENABLED(BACKLASH_COMPENSATION)
  #if IS_CORE
    #error "BACKLASH_COMPENSATION is incompatible with CORE kinematics."
  #endif
  #ifndef BACKLASH_DISTANCE_MM
    #error "BACKLASH_COMPENSATION requires BACKLASH_DISTANCE_MM"
  #endif
  #ifndef BACKLASH_CORRECTION
    #error "BACKLASH_COMPENSATION requires BACKLASH_CORRECTION"
  #endif
#endif

#if ENABLED(GRADIENT_MIX) && MIXING_VIRTUAL_TOOLS < 2
  #error "GRADIENT_MIX requires 2 or more MIXING_VIRTUAL_TOOLS."
#endif

/**
 * Prusa MMU2 requirements
 */
#if HAS_MMU2()
  #if EXTRUDERS != 6
    #error "HAS_MMU2() requires EXTRUDERS = 6."
  #endif
#endif

/**
 * Require soft endstops for certain setups
 */
#if !BOTH(MIN_SOFTWARE_ENDSTOPS, MAX_SOFTWARE_ENDSTOPS)
  #if HAS_HOTEND_OFFSET
    #error "MIN_ and MAX_SOFTWARE_ENDSTOPS are both required with offset hotends."
  #endif
#endif

/**
 * Ensure this option is set intentionally
 */
#if ENABLED(PSU_CONTROL) && !defined(PSU_ACTIVE_HIGH)
  #error "PSU_CONTROL requires PSU_ACTIVE_HIGH to be defined as 'true' or 'false'."
#endif

#if ENABLED(PRINT_PROGRESS_SHOW_DECIMALS)
  #error "PRINT_PROGRESS_SHOW_DECIMALS currently requires a Graphical LCD."
#elif ENABLED(SHOW_REMAINING_TIME)
  #error "SHOW_REMAINING_TIME currently requires a Graphical LCD."
#endif

#if HAS_GCODE_COMPATIBILITY() && ENABLED(GCODE_MOTION_MODES)
    #error "HAS_GCODE_COMPATIBILITY() and GCODE_MOTION_MODES can't be enabled at the same time"
#endif

#if (NUM_SERVOS > 0) || ENABLED(EDITABLE_SERVO_ANGLES) || ENABLED(DEACTIVATE_SERVOS_AFTER_MOVE) || defined(SERVO_DELAY) || HAS_Z_SERVO_PROBE || defined(HAS_Z_SERVO_PROBE) || defined(Z_PROBE_SERVO_NR)
    #error "servos are not supported"
#endif

#if ENABLED(BLTOUCH) || ENABLED(BLTOUCH_DELAY) || ENABLED(BLTOUCH_FORCE_5V_MODE) || ENABLED(BLTOUCH_FORCE_OPEN_DRAIN_MODE) || defined(BLTOUCH_V3)
    #error "BLTOUCH is not supported"
#endif

#if ENABLED(SPINDLE_FEATURE) || ENABLED(LASER_FEATURE)
    #error "laser is not supported"
#endif

#if ENABLED(EXT_SOLENOID) || ENABLED(MANUAL_SOLENOID_CONTROL) || ENABLED(SOLENOID_PROBE) || HAS_SOLENOID_1
    #error "solenoids are not supported"
#endif

#if ENABLED(Z_PROBE_SLED)
    #error "Z_PROBE_SLED is not supported"
#endif

#if ENABLED(Z_PROBE_ALLEN_KEY)
    #error "Z_PROBE_ALLEN_KEY is not supported"
#endif

#if ENABLED(DIGIPOT_I2C)
    #error "DIGIPOT is not supported"
#endif

#if ENABLED(COOLANT_MIST) || ENABLED(COOLANT_FLOOD) || ENABLED(COOLANT_CONTROL)
    #error "COOLANT is not supported"
#endif
