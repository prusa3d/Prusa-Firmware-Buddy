/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
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

#include "hw_configuration.hpp"
#include <Marlin/src/core/macros.h>
#include <option/has_loadcell.h>
#include <option/has_mmu2.h>
#include <option/has_precise_homing_corexy.h>
#include <option/has_precise_homing.h>
#include <option/has_print_sheet_detection.h>

// clang-format off

/**
 * Configuration.h
 *
 * Basic settings such as:
 *
 * - Type of electronics
 * - Type of temperature sensor
 * - Printer geometry
 * - Endstop configuration
 * - LCD controller
 * - Extra features
 *
 * Advanced settings can be found in Configuration_adv.h
 *
 */
#define CONFIGURATION_H_VERSION 020000
#define USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES

#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    #include "config_store/store_c_api.h"
#endif

//===========================================================================
//============================= Getting Started =============================
//===========================================================================

/**
 * Here are some standard links for getting your machine calibrated:
 *
 * http://reprap.org/wiki/Calibration
 * http://youtu.be/wAL9d7FgInk
 * http://calculator.josefprusa.cz
 * http://reprap.org/wiki/Triffid_Hunter%27s_Calibration_Guide
 * http://www.thingiverse.com/thing:5573
 * https://sites.google.com/site/repraplogphase/calibration-of-your-reprap
 * http://www.thingiverse.com/thing:298812
 */

// @section info

// User-specified version info of this build to display in [Pronterface, etc] terminal window during
// startup. Implementation of an idea by Prof Braino to inform user that any changes made to this
// build by the user have been successfully uploaded into firmware.
#define STRING_CONFIG_H_AUTHOR "(none, default config)" // Who made the changes.
#define SHOW_BOOTSCREEN

/**
 * *** VENDORS PLEASE READ ***
 *
 * Marlin allows you to add a custom boot image for Graphical LCDs.
 * With this option Marlin will first show your custom screen followed
 * by the standard Marlin logo with version number and web URL.
 *
 * We encourage you to take advantage of this new feature and we also
 * respectfully request that you retain the unmodified Marlin boot screen.
 */

// Enable to show the bitmap in Marlin/_Bootscreen.h on startup.
//#define SHOW_CUSTOM_BOOTSCREEN

// Enable to show the bitmap in Marlin/_Statusscreen.h on the status screen.
//#define CUSTOM_STATUS_SCREEN_IMAGE

// @section machine

/**
 * Select the serial port on the board to use for communication with the host.
 * This allows the connection of wireless adapters (for instance) to non-default port pins.
 * Note: The first serial port (-1 or 0) will always be used by the Arduino bootloader.
 *
 * :[-1, 0, 1, 2, 3, 4, 5, 6, 7]
 */
#define SERIAL_PORT -1

/**
 * Select a secondary serial port on the board to use for communication with the host.
 * This allows the connection of wireless adapters (for instance) to non-default port pins.
 * Serial port -1 is the USB emulated serial port, if available.
 *
 * :[-1, 0, 1, 2, 3, 4, 5, 6, 7]
 */
//#define SERIAL_PORT_2 -1

/**
 * This setting determines the communication speed of the printer.
 *
 * 250000 works in most cases, but you might try a lower speed if
 * you commonly experience drop-outs during host printing.
 * You may try up to 1000000 to speed up SD file transfer.
 *
 * :[2400, 9600, 19200, 38400, 57600, 115200, 250000, 500000, 1000000]
 */
#define BAUDRATE 115200

// Enable the Bluetooth serial interface on AT90USB devices
//#define BLUETOOTH

// Optional custom name for your RepStrap or other custom machine
// Displayed in the LCD "Ready" message
//#define CUSTOM_MACHINE_NAME "3D Printer"

// Define this to set a unique identifier for this printer, (Used by some programs to differentiate between machines)
// You can use an online service to generate a random UUID. (eg http://www.uuidgenerator.net/version4)
//#define MACHINE_UUID "00000000-0000-0000-0000-000000000000"

// @section extruder

// This defines the number of extruders
// :[1, 2, 3, 4, 5, 6]
#if HAS_MMU2()
#define EXTRUDERS 6 // 5 + NoTool
#else
#define EXTRUDERS 1
#endif

// Generally expected filament diameter (1.75, 2.85, 3.0, ...). Used for Volumetric, Filament Width Sensor, etc.
#define DEFAULT_NOMINAL_FILAMENT_DIA 1.75

// For Cyclops or any "multi-extruder" that shares a single nozzle.
//#define SINGLENOZZLE

// @section temperature

//===========================================================================
//============================= Thermal Settings ============================
//===========================================================================

#define TEMP_SENSOR_0 2005
#define TEMP_SENSOR_BED 2004
#define TEMP_SENSOR_HEATBREAK 2008
#define TEMP_SENSOR_BOARD 2000

#define TEMP_RESIDENCY_TIME 5 // (seconds) Time to wait for hotend to "settle" in M109
#define TEMP_WINDOW 1 // (°C) Temperature proximity for the "temperature reached" timer
#define TEMP_HYSTERESIS 3 // (°C) Temperature proximity considered "close enough" to the target

#define TEMP_BED_RESIDENCY_TIME 5 // (seconds) Time to wait for bed to "settle" in M190
#define TEMP_BED_WINDOW 1 // (°C) Temperature proximity for the "temperature reached" timer
#define TEMP_BED_HYSTERESIS 3 // (°C) Temperature proximity considered "close enough" to the target

// Below this temperature the heater will be switched off
// because it probably indicates a broken thermistor wire.
#define HEATER_0_MINTEMP 5
#define BED_MINTEMP 5
#define HEATBREAK_MINTEMP 5
#define BOARD_MINTEMP 5

// Above this temperature the heater will be switched off.
// This can protect components from overheating, but NOT from shorts and failures.
// (Use MINTEMP for thermistor short/failure protection.)
#define HEATER_0_MAXTEMP 305
#define HEATER_MAXTEMP_SAFETY_MARGIN 15
#define BED_MAXTEMP 125
#define BED_MAXTEMP_SAFETY_MARGIN 5
#define HEATBREAK_MAXTEMP 100
#define BOARD_MAXTEMP 120

//===========================================================================
//============================= PID Settings ================================
//===========================================================================
// PID Tuning Guide here: http://reprap.org/wiki/PID_Tuning

// Comment the following line to disable PID and enable bang-bang.
#define PIDTEMP
#define BANG_MAX 255 // Limits current to nozzle while in bang-bang mode; 255=full current
#define PID_MAX BANG_MAX // Limits current to nozzle while PID is active (see PID_FUNCTIONAL_RANGE below); 255=full current
#define PID_K1 0.95 // Smoothing factor within any PID loop
#if ENABLED(PIDTEMP)
    //#define PID_EDIT_MENU         // Add PID editing to the "Advanced Settings" menu. (~700 bytes of PROGMEM)
    //#define PID_DEBUG             // Sends debug data to the serial port.
    //#define PID_OPENLOOP 1        // Puts PID in open loop. M104/M140 sets the output power from 0 to PID_MAX
    // Set/get with gcode: M301 E[extruder number, 0-2]
    /**
     * If the temperature difference between the target temperature and the actual temperature
     * is more than PID_FUNCTIONAL_RANGE then the PID will be shut off and the heater will be set to min/max.
     */
    #define PID_FUNCTIONAL_RANGE 500

    // RING
    #define DEFAULT_Kp 14.00
    #define DEFAULT_Ki 1.00
    #define DEFAULT_Kd 100.00

    //#define STEADY_STATE_HOTEND // Enable support for STEADY_STATE_HOTEND (feed-forward thermal management)
    #define STEADY_STATE_HOTEND_LINEAR_COOLING_TERM 0.322
    #define STEADY_STATE_HOTEND_QUADRATIC_COOLING_TERM 0.0002
    #define STEADY_STATE_HOTEND_FAN_COOLING_TERM 9.24
#endif // PIDTEMP

//===========================================================================
//============================= PID > Bed Temperature Control ===============
//===========================================================================

/**
 * PID Bed Heating
 *
 * If this option is enabled set PID constants below.
 * If this option is disabled, bang-bang will be used.
 *
 * The PID frequency will be the same as the extruder PWM.
 * If PID_dT is the default, and correct for the hardware/configuration, that means 7.689Hz,
 * which is fine for driving a square wave into a resistive load and does not significantly
 * impact FET heating. This also works fine on a Fotek SSR-10DA Solid State Relay into a 250W
 * heater. If your configuration is significantly different than this and you don't understand
 * the issues involved, don't use bed PID until someone else verifies that your hardware works.
 */
#define PIDTEMPBED
#if ENABLED(PIDTEMPBED)

    //#define PID_BED_DEBUG // Sends debug data to the serial port.

    //120V 250W silicone heater into 4mm borosilicate (MendelMax 1.5+)
    //from FOPDT model - kp=.39 Tp=405 Tdead=66, Tc set to 79.2, aggressive factor of .15 (vs .1, 1, 10)
    //#define DEFAULT_bedKp 10.00
    //#define DEFAULT_bedKi .023
    //#define DEFAULT_bedKd 305.4

    //120V 250W silicone heater into 4mm borosilicate (MendelMax 1.5+)
    //from pidautotune
    //#define DEFAULT_bedKp 97.1
    //#define DEFAULT_bedKi 1.41
    //#define DEFAULT_bedKd 1675.16

    //24V Prusa MK3 bed
    #define DEFAULT_bedKp 126.13f
    #define DEFAULT_bedKi 4.30f
    #define DEFAULT_bedKd 924.76f

// FIND YOUR OWN: "M303 E-1 C8 S90" to run autotune on the bed at 90 degreesC for 8 cycles.
#endif // PIDTEMPBED

// @section extruder

/**
 * Prevent extrusion if the temperature is below EXTRUDE_MINTEMP.
 * Add M302 to set the minimum extrusion temperature and/or turn
 * cold extrusion prevention on and off.
 *
 * *** IT IS HIGHLY RECOMMENDED TO LEAVE THIS OPTION ENABLED! ***
 */
#define PREVENT_COLD_EXTRUSION
#define EXTRUDE_MINTEMP 170

/**
 * Prevent a single extrusion longer than EXTRUDE_MAXLENGTH.
 * Note: For Bowden Extruders make this large enough to allow load/unload.
 */
#define PREVENT_LENGTHY_EXTRUDE
#define EXTRUDE_MAXLENGTH 200

//===========================================================================
//===============+=== PID > Heatbreak autocooling Control ===================
//===========================================================================

//PID autocooling
#define PIDTEMPHEATBREAK

#if ENABLED(PIDTEMPHEATBREAK)
    //#define PID_HEATBREAK_DEBUG // enable debug output for heatbreak fan PID regulator
    #define MAX_HEATBREAK_POWER 255 // limits duty cycle to heatbreak fan; 255=full current
    #define MIN_START_HEATBREAK_POWER 255 // Minimum PWM needed to start fan spinning reliably
    #define MIN_STOP_HEATBREAK_POWER 55 // Minimum PWM needed to keep fan spinning reliably
    #define HEATBREAK_FAN_KICK_CYCLES -1 // Output at least MIN_START_HEATBREAK_POWER once per cycles, -1 to deliver starting pulse just once
    #define HEATBREAK_FAN_ALWAYS_ON_NOZZLE_TEMPERATURE 45 // Never switch off heatbreak fan when nozzle temperature is over
    #define DEFAULT_HEATBREAK_TEMPERATURE 36

    #define HEATBREAK_PID_K1 0.995
    #define DEFAULT_heatbreakKp 25.50
    #define DEFAULT_heatbreakKi 5.00
    #define DEFAULT_heatbreakKd 300.00
#endif

//===========================================================================
//======================== Thermal Runaway Protection =======================
//===========================================================================

/**
 * Thermal Protection provides additional protection to your printer from damage
 * and fire. Marlin always includes safe min and max temperature ranges which
 * protect against a broken or disconnected thermistor wire.
 *
 * The issue: If a thermistor falls out, it will report the much lower
 * temperature of the air in the room, and the the firmware will keep
 * the heater on.
 *
 * If you get "Thermal Runaway" or "Heating failed" errors the
 * details can be tuned in Configuration_adv.h
 */

#define THERMAL_PROTECTION_HOTENDS // Enable thermal protection for all extruders
#define THERMAL_PROTECTION_BED // Enable thermal protection for the heated bed

//===========================================================================
//============================= Mechanical Settings =========================
//===========================================================================

// @section machine

// Uncomment one of these options to enable CoreXY, CoreXZ, or CoreYZ kinematics
// either in the usual order or reversed
//#define COREXY
//#define COREXZ
//#define COREYZ
//#define COREYX
//#define COREZX
//#define COREZY

//===========================================================================
//============================== Endstop Settings ===========================
//===========================================================================

// @section homing

//! Move in opposite direction as first homing move
//! Required for sensorless homing under most circumstances
#define MOVE_BACK_BEFORE_HOMING
#if ENABLED(MOVE_BACK_BEFORE_HOMING)
    #define MOVE_BACK_BEFORE_HOMING_DISTANCE       10.0f // increased distance for subsequent attempts
#endif

// Specify here all the endstop connectors that are connected to any endstop or probe.
// Almost all printers will be using one per axis. Probes will use one or more of the
// extra connectors. Leave undefined any used for non-endstop and non-probe purposes.
#define USE_XMIN_PLUG
#define USE_YMIN_PLUG
#define USE_ZMIN_PLUG
#define USE_XMAX_PLUG
#define USE_YMAX_PLUG
#define USE_ZMAX_PLUG

// Enable pullup for all endstops to prevent a floating state
#define ENDSTOPPULLUPS
#if DISABLED(ENDSTOPPULLUPS)
// Disable ENDSTOPPULLUPS to set pullups individually
//#define ENDSTOPPULLUP_XMAX
//#define ENDSTOPPULLUP_YMAX
//#define ENDSTOPPULLUP_ZMAX
//#define ENDSTOPPULLUP_XMIN
//#define ENDSTOPPULLUP_YMIN
//#define ENDSTOPPULLUP_ZMIN
//#define ENDSTOPPULLUP_ZMIN_PROBE
#endif

// Enable pulldown for all endstops to prevent a floating state
//#define ENDSTOPPULLDOWNS
#if DISABLED(ENDSTOPPULLDOWNS)
// Disable ENDSTOPPULLDOWNS to set pulldowns individually
//#define ENDSTOPPULLDOWN_XMAX
//#define ENDSTOPPULLDOWN_YMAX
//#define ENDSTOPPULLDOWN_ZMAX
//#define ENDSTOPPULLDOWN_XMIN
//#define ENDSTOPPULLDOWN_YMIN
//#define ENDSTOPPULLDOWN_ZMIN
//#define ENDSTOPPULLDOWN_ZMIN_PROBE
#endif

// Mechanical endstop with COM to ground and NC to Signal uses "false" here (most common setup).
    #define X_MIN_ENDSTOP_INVERTING true // set to true to invert the logic of the endstop.
    #define Y_MIN_ENDSTOP_INVERTING true // set to true to invert the logic of the endstop.
    #define Z_MIN_ENDSTOP_INVERTING true // set to true to invert the logic of the endstop.
    #define X_MAX_ENDSTOP_INVERTING true // set to true to invert the logic of the endstop.
    #define Y_MAX_ENDSTOP_INVERTING true // set to true to invert the logic of the endstop.
    #define Z_MAX_ENDSTOP_INVERTING true // set to true to invert the logic of the endstop.
    #define Z_MIN_PROBE_ENDSTOP_INVERTING true // set to true to invert the logic of the probe.

/**
 * Stepper Drivers
 *
 * These settings allow Marlin to tune stepper driver timing and enable advanced options for
 * stepper drivers that support them. You may also override timing options in Configuration_adv.h.
 *
 * A4988 is assumed for unspecified drivers.
 *
 * Options: A4988, A5984, DRV8825, LV8729, TB6560, TB6600, TMC2100,
 *          TMC2130, TMC2130_STANDALONE, TMC2208, TMC2208_STANDALONE,
 *          TMC26X,  TMC26X_STANDALONE,  TMC2660, TMC2660_STANDALONE,
 *          TMC2160, TMC2160_STANDALONE, TMC5130, TMC5130_STANDALONE,
 *          TMC5160, TMC5160_STANDALONE
 * :['A4988', 'A5984', 'DRV8825', 'LV8729', 'TB6560', 'TB6600', 'TMC2100', 'TMC2130', 'TMC2130_STANDALONE', 'TMC2160', 'TMC2160_STANDALONE', 'TMC2208', 'TMC2208_STANDALONE', 'TMC26X', 'TMC26X_STANDALONE', 'TMC2660', 'TMC2660_STANDALONE', 'TMC5130', 'TMC5130_STANDALONE', 'TMC5160', 'TMC5160_STANDALONE']
 */
#define X_DRIVER_TYPE TMC2130
#define Y_DRIVER_TYPE TMC2130
#define Z_DRIVER_TYPE TMC2130
#define E0_DRIVER_TYPE TMC2130

// Enable this feature if all enabled endstop pins are interrupt-capable.
// This will remove the need to poll the interrupt pins, saving many CPU cycles.
#define ENDSTOP_INTERRUPTS_FEATURE

//=============================================================================
//============================== Movement Settings ============================
//=============================================================================
// @section motion

/**
 * Default Settings
 *
 * These settings can be reset by M502
 *
 * Note that if EEPROM is enabled, saved values will override these.
 */

/**
 * With this option each E stepper can have its own factors for the
 * following movement settings. If fewer factors are given than the
 * total number of extruders, the last value applies to the rest.
 */
//#define DISTINCT_E_FACTORS

/**
 * Default Axis Steps Per Unit (steps/mm)
 * Override with M92
 *                                      X, Y, Z, E0 [, E1[, E2[, E3[, E4[, E5]]]]]
 */
#define DEFAULT_AXIS_STEPS_PER_UNIT_X 100
#define DEFAULT_AXIS_STEPS_PER_UNIT_Y 100
#define DEFAULT_AXIS_STEPS_PER_UNIT_Z 400
#define DEFAULT_AXIS_STEPS_PER_UNIT_E0 380

/**
 * Default Max Feed Rate (mm/s)
 * Override with M203
 *                                      X, Y, Z, E0 [, E1[, E2[, E3[, E4[, E5]]]]]
 */
#define DEFAULT_MAX_FEEDRATE \
    { 400, 400, 40, 50 }

/// HW limits of feed rate
#define HWLIMIT_NORMAL_MAX_FEEDRATE \
    { 300, 300, 40, 100 }
#define HWLIMIT_STEALTH_MAX_FEEDRATE \
    { 160, 160, 40, 100 }

/**
 * Default Max Acceleration (change/s) change = mm/s
 * (Maximum start speed for accelerated moves)
 * Override with M201
 *                                      X, Y, Z, E0 [, E1[, E2[, E3[, E4[, E5]]]]]
 */
#define DEFAULT_MAX_ACCELERATION \
    { 1250, 1250, 200, 1500 }

/// HW limits of max acceleration
#define HWLIMIT_NORMAL_MAX_ACCELERATION \
    { 7000, 7000, 750, 6000 }
#define HWLIMIT_STEALTH_MAX_ACCELERATION \
    { 2500, 2500, 200, 2500 }

/**
 * Default Acceleration (change/s) change = mm/s
 * Override with M204
 *
 *   M204 P    Acceleration
 *   M204 R    Retract Acceleration
 *   M204 T    Travel Acceleration
 */
#define DEFAULT_ACCELERATION 1250 // X, Y, Z and E acceleration for printing moves
#define DEFAULT_RETRACT_ACCELERATION 800 // E acceleration for retracts
#define DEFAULT_TRAVEL_ACCELERATION 1250 // X, Y, Z acceleration for travel (non printing) moves

//
// Use Junction Deviation instead of traditional Jerk Limiting
//
//#define JUNCTION_DEVIATION
// #define CLASSIC_JERK
#if DISABLED(CLASSIC_JERK)
    #define JUNCTION_DEVIATION_MM 0.01f // (mm) Distance from real junction edge
    #define JD_SMALL_SEGMENT_HANDLING   // Handle small segments (< 1 mm) with large junction angles (> 135°) based on a local curvature estimate, instead of just the junction angle.
#endif

/**
 * Default Jerk (mm/s)
 * Override with M205 X Y Z E
 *
 * "Jerk" specifies the minimum speed change that requires acceleration.
 * When changing speed and direction, if the difference is less than the
 * value set here, it may happen instantaneously.
 */
#if ENABLED(CLASSIC_JERK)
    #define DEFAULT_XJERK 8.0f
    #define DEFAULT_YJERK 8.0f
    #define DEFAULT_ZJERK 2.0f
#endif

#define DEFAULT_EJERK 5 // May be used by Linear Advance

/// HW limits of Jerk
#define HWLIMIT_NORMAL_JERK {10, 10, 2, 10}
#define HWLIMIT_STEALTH_JERK {8, 8, 2, 10}

/**
 * S-Curve Acceleration
 *
 * This option eliminates vibration during printing by fitting a Bézier
 * curve to move acceleration, producing much smoother direction changes.
 *
 * See https://github.com/synthetos/TinyG/wiki/Jerk-Controlled-Motion-Explained
 */
//#define S_CURVE_ACCELERATION

//===========================================================================
//============================= Z Probe Options =============================
//===========================================================================
// @section probes

//
// See http://marlinfw.org/docs/configuration/probes.html
//

/**
 * Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN
 *
 * Enable this option for a probe connected to the Z Min endstop pin.
 */
#define Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN

/**
 * Z_MIN_PROBE_PIN
 *
 * Define this pin if the probe is not connected to Z_MIN_PIN.
 * If not defined the default pin for the selected MOTHERBOARD
 * will be used. Most of the time the default is what you want.
 *
 *  - The simplest option is to use a free endstop connector.
 *  - Use 5V for powered (usually inductive) sensors.
 *
 *  - RAMPS 1.3/1.4 boards may use the 5V, GND, and Aux4->D32 pin:
 *    - For simple switches connect...
 *      - normally-closed switches to GND and D32.
 *      - normally-open switches to 5V and D32.
 *
 */
//#define Z_MIN_PROBE_PIN 32 // Pin 32 is the RAMPS default

/**
 * Probe Type
 *
 * Allen Key Probes, Servo Probes, Z-Sled Probes, FIX_MOUNTED_PROBE, etc.
 * Activate one of these to use Auto Bed Leveling below.
 */

/**
 * A Fix-Mounted Probe either doesn't deploy or needs manual deployment.
 *   (e.g., an inductive probe or a nozzle-based probe-switch.)
 */
#define FIX_MOUNTED_PROBE
#if HAS_LOADCELL()
  #define NOZZLE_LOAD_CELL
#endif

/**
 *   Z Probe to nozzle (X,Y) offset, relative to (0, 0).
 *   X and Y offsets must be integers.
 *
 *   In the following example the X and Y offsets are both positive:
 *   #define X_PROBE_OFFSET_FROM_EXTRUDER 10
 *   #define Y_PROBE_OFFSET_FROM_EXTRUDER 10
 *
 *      +-- BACK ---+
 *      |           |
 *    L |    (+) P  | R <-- probe (20,20)
 *    E |           | I
 *    F | (-) N (+) | G <-- nozzle (10,10)
 *    T |           | H
 *      |    (-)    | T
 *      |           |
 *      O-- FRONT --+
 *    (0,0)
 */
//PINDA
//#define X_PROBE_OFFSET_FROM_EXTRUDER 23  // X offset: -left  +right  [of the nozzle]
//#define Y_PROBE_OFFSET_FROM_EXTRUDER 5   // Y offset: -front +behind [the nozzle]
//#define Z_PROBE_OFFSET_FROM_EXTRUDER -0.40 //Z offset: -below +above  [the nozzle]

//Load Cell
#define NOZZLE_TO_PROBE_OFFSET \
    { 0, 0, 0 }

// Certain types of probes need to stay away from edges
#define MIN_PROBE_EDGE 0

// X and Y axis travel speed (mm/m) to get to the first probe location
#define XY_PROBE_SPEED_INITIAL 8000

// X and Y axis travel speed (mm/m) between probes
#define XY_PROBE_SPEED  18000

// Feedrate (mm/m) for the first approach when double-probing (MULTIPLE_PROBING == 2)
#define Z_PROBE_SPEED_FAST 6 * 100

// Feedrate (mm/m) for the "accurate" probe of each point
#define Z_PROBE_SPEED_SLOW 70

// [ms] delay before first Z probe for taring
#define Z_FIRST_PROBE_DELAY 300

#if ENABLED(NOZZLE_LOAD_CELL)
  // Enable G29 P9 for nozzle cleanup
  #define PROBE_CLEANUP_SUPPORT
  #define PROBE_CLEANUP_CLEARANCE 2.0
  #define PROBE_CLEANUP_TRAVEL_ACCELERATION 800
  #define Z_PROBE_SPEED_BACK_MOVE 20
#endif

// The number of probes to perform at each point.
//   Set to 2 for a fast/slow probe, using the second probe result.
//   Set to 3 or more for slow probes, averaging the results.
// For loadcell, specifies the maximum number of tries per probing point.
#define MULTIPLE_PROBING 40

//#define EXTRA_PROBING 1

/**
 * Z probes require clearance when deploying, stowing, and moving between
 * probe points to avoid hitting the bed and other hardware.
 * Servo-mounted probes require extra space for the arm to rotate.
 * Inductive probes need space to keep from triggering early.
 *
 * Use these settings to specify the distance (mm) to raise the probe (or
 * lower the bed). The values set here apply over and above any (negative)
 * probe Z Offset set with Z_PROBE_OFFSET_FROM_EXTRUDER, M851, or the LCD.
 * Only integer values >= 1 are valid here.
 *
 * Example: `M851 Z-5` with a CLEARANCE of 4  =>  9mm from bed to nozzle.
 *     But: `M851 Z+1` with a CLEARANCE of 2  =>  2mm from bed to nozzle.
 */
#define Z_CLEARANCE_BEFORE_PROBING 5 // Z Clearance before first MBL probe
#define Z_CLEARANCE_DEPLOY_PROBE 0 // Z Clearance for Deploy/Stow
#define Z_CLEARANCE_BETWEEN_PROBES 0.23f // Z Clearance between probe points 1
#define Z_CLEARANCE_MULTI_PROBE 0.2f // Z Clearance between multiple probes
#define Z_AFTER_PROBING 2 // Z position after probing is done 2

#define Z_PROBE_LOW_POINT -5 // Farthest distance below the trigger-point to go before stopping

// For M851 give a range for adjusting the Z probe offset
#define Z_PROBE_OFFSET_RANGE_MIN -20
#define Z_PROBE_OFFSET_RANGE_MAX 20

// Enable the M48 repeatability test to test probe accuracy
//#define Z_MIN_PROBE_REPEATABILITY_TEST

/**
 * Enable one or more of the following if probing seems unreliable.
 * Heaters and/or fans can be disabled during probing to minimize electrical
 * noise. A delay can also be added to allow noise and vibration to settle.
 * These options are most useful for the BLTouch probe, but may also improve
 * readings with inductive probes and piezo sensors.
 */
//#define PROBING_STEPPERS_OFF      // Turn steppers off (unless needed to hold position) when probing
//#define DELAY_BEFORE_PROBING 200  // (ms) To prevent vibrations from triggering piezo sensors

// For Inverting Stepper Enable Pins (Active Low) use 0, Non Inverting (Active High) use 1
// :{ 0:'Low', 1:'High' }
#define X_ENABLE_ON 0
#define Y_ENABLE_ON 0
#define Z_ENABLE_ON 0
#define E_ENABLE_ON 0 // For all extruders

// Disables axis stepper immediately when it's not being used.
// WARNING: When motors turn off there is a chance of losing position accuracy!
#define DISABLE_X false
#define DISABLE_Y false
#define DISABLE_Z false

// X and Y axes ENABLE/DISABLE functions are linked through pins
#if BOARD_IS_XBUDDY()
  #define XY_LINKED_ENABLE true
#endif

// Warn on display about possibly reduced accuracy
//#define DISABLE_REDUCED_ACCURACY_WARNING

// @section extruder

#define DISABLE_E false // For all extruders

// default values
#define DEFAULT_INVERT_X_DIR false
#define DEFAULT_INVERT_Y_DIR true
#define DEFAULT_INVERT_Z_DIR false
#define DEFAULT_INVERT_E0_DIR false

#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    //this part if header is accesible only from C++ because of bool
    #define INVERT_X_DIR  has_inverted_x()
    #define INVERT_Y_DIR  has_inverted_y()
    #define INVERT_Z_DIR  has_inverted_z()
    #define INVERT_E0_DIR has_inverted_e()
#else // !USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    // @section machine
    // Invert the stepper direction. Change (or reverse the motor connector) if an axis goes the wrong way.
    #define INVERT_X_DIR DEFAULT_INVERT_X_DIR
    #define INVERT_Y_DIR DEFAULT_INVERT_Y_DIR
    #define INVERT_Z_DIR DEFAULT_INVERT_Z_DIR

    // @section extruder
    #define INVERT_E0_DIR DEFAULT_INVERT_E0_DIR
#endif // USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES

//remaining extruders are not stored in eeprom, thus cannot be changed
#define INVERT_E1_DIR false
#define INVERT_E2_DIR false
#define INVERT_E3_DIR false
#define INVERT_E4_DIR false
#define INVERT_E5_DIR false

// @section homing

//#define UNKNOWN_Z_NO_RAISE // Don't raise Z (lower the bed) if Z is "unknown." For beds that fall when Z is powered off.
/**
 * (mm) Minimal Z height before homing (G28) for Z clearance above the bed, clamps, ...
 * Be sure you have this distance over your Z_MAX_POS in case.
 */
#define Z_HOMING_HEIGHT 4

// Direction of endstops when homing; 1=MAX, -1=MIN
// :[-1,1]
#define X_HOME_DIR -1
#define Y_HOME_DIR -1
#define Z_HOME_DIR -1

// @section machine

// The size of the print bed
#define X_BED_SIZE 250
#define Y_BED_SIZE 210
#define Z_SIZE 220

// Travel limits (mm) after homing, corresponding to endstop positions.
#define X_MIN_POS -1
#define Y_MIN_POS -4
#define Z_MIN_POS 0
#define X_MAX_POS (X_BED_SIZE + 1)
#define X_MIN_PRINT_POS X_MIN_POS
#define X_MAX_PRINT_POS X_MAX_POS
#define Y_MAX_POS (Y_BED_SIZE + 1)
#ifdef USE_PRUSA_EEPROM_AS_SOURCE_OF_DEFAULT_VALUES
    #define DEFAULT_Z_MAX_POS (Z_SIZE + 1)
    #define Z_MIN_LEN_LIMIT 1
    #define Z_MAX_LEN_LIMIT 10000
    #define Z_MAX_POS (get_z_max_pos_mm())
#else
    #define Z_MAX_POS (Z_SIZE + 1)
#endif

/// How much space there is between the bed and the ceiling for Z = 0 on CoreXY printers
/// If defined, the printer will check max_printed_z and if a move would result in the model getting above this clearance,
/// it will prompt the user
/// Requires HAS_CEILING_CLEARANCE()
/// !!! IMPORTANT: Consult with the slicer team when changing this number, needs to be synced with the slicer profiles
// #define Z_CEILING_CLEARANCE 100

/// Distance between start of the axis to the position where ordinary movement is allowed
#define X_HOME_GAP 0.5f
#define Y_HOME_GAP 0.5f
#define Z_HOME_GAP 0

/// Space after allowed end of axis where axis should end
#define X_END_GAP 5
#define Y_END_GAP 5
#define Z_END_GAP 10

/**
 * Number of precise homing tries
 *
 * Three times more tries are used when recovering from crash
 * or power panic.
 */
#define PRECISE_HOMING_TRIES 12

/**
 * Software Endstops
 *
 * - Prevent moves outside the set machine bounds.
 * - Individual axes can be disabled, if desired.
 * - X and Y only apply to Cartesian robots.
 * - Use 'M211' to set software endstops on/off or report current state
 */

// Min software endstops constrain movement within minimum coordinate bounds
#define MIN_SOFTWARE_ENDSTOPS
#if ENABLED(MIN_SOFTWARE_ENDSTOPS)
    #define MIN_SOFTWARE_ENDSTOP_X
    #define MIN_SOFTWARE_ENDSTOP_Y
    #define MIN_SOFTWARE_ENDSTOP_Z
#endif

// Max software endstops constrain movement within maximum coordinate bounds
#define MAX_SOFTWARE_ENDSTOPS
#if ENABLED(MAX_SOFTWARE_ENDSTOPS)
    #define MAX_SOFTWARE_ENDSTOP_X
    #define MAX_SOFTWARE_ENDSTOP_Y
    #define MAX_SOFTWARE_ENDSTOP_Z
#endif

#if EITHER(MIN_SOFTWARE_ENDSTOPS, MAX_SOFTWARE_ENDSTOPS)
//#define SOFT_ENDSTOPS_MENU_ITEM  // Enable/Disable software endstops from the LCD
#endif

//===========================================================================
//=============================== Bed Leveling ==============================
//===========================================================================
// @section calibrate

/**
 * Choose one of the options below to enable G29 Bed Leveling. The parameters
 * and behavior of G29 will change depending on your selection.
 *
 *  If using a Probe for Z Homing, enable Z_SAFE_HOMING also!
 *
 * - AUTO_BED_LEVELING_UBL (Unified Bed Leveling)
 *   A comprehensive bed leveling system combining the features and benefits
 *   of other systems. UBL also includes integrated Mesh Generation, Mesh
 *   Validation and Mesh Editing systems.
 */
#define AUTO_BED_LEVELING_UBL

/**
 * Normally G28 leaves leveling disabled on completion. Enable
 * this option to have G28 restore the prior leveling state.
 */
#define RESTORE_LEVELING_AFTER_G28 false

#if ENABLED(AUTO_BED_LEVELING_UBL)
    // Gradually reduce leveling correction until a set height is reached,
    // at which point movement will be level to the machine's XY plane.
    // The height can be set with M420 Z<height>
    #define ENABLE_LEVELING_FADE_HEIGHT

    // For Cartesian machines, instead of dividing moves on mesh boundaries,
    // split up moves into short segments like a Delta. This follows the
    // contours of the bed more closely than edge-to-edge straight moves.
    #define SEGMENT_LEVELED_MOVES
    #define LEVELED_SEGMENT_LENGTH 5.0 // (mm) Length of all segments (except the last one)

    /// (mm) If distance between min and max Z during probing exceeds this value, we offer a Z alignment calibration
    //#define MBL_Z_DIFF_CALIB_WARNING_THRESHOLD 2
#endif

#if ENABLED(AUTO_BED_LEVELING_UBL)

//===========================================================================
//========================= Unified Bed Leveling ============================
//===========================================================================

//#define MESH_EDIT_GFX_OVERLAY   // Display a graphics overlay while editing the mesh

    #define GRID_BORDER 1 // border we are never gonna probe, only border of size 1 is currently supported
    #define GRID_MAJOR_STEP 3 // the offset between major points
    #define GRID_MAJOR_POINTS_X 7 // number of major probes on the X axis
    #define GRID_MAJOR_POINTS_Y 7 // number of major probes on the Y axis
    #define GRID_MAX_POINTS_X 21
    #define GRID_MAX_POINTS_Y 21
    //#define GRID_MAX_POINTS_X (GRID_BORDER * 2 + GRID_MAJOR_POINTS_X + ((GRID_MAJOR_POINTS_X - 1) * (GRID_MAJOR_STEP - 1))) // full resolution of the grid (X axis)
    //#define GRID_MAX_POINTS_Y (GRID_BORDER * 2 + GRID_MAJOR_POINTS_Y + ((GRID_MAJOR_POINTS_Y - 1) * (GRID_MAJOR_STEP - 1))) // full resolution of the grid (X axis)

    #define UBL_MESH_EDIT_MOVES_Z // Sophisticated users prefer no movement of nozzle
    #define UBL_SAVE_ACTIVE_ON_M500 // Save the currently active mesh in the current slot on M500
#endif // BED_LEVELING

/**
 * Add a bed leveling sub-menu for ABL or MBL.
 * Include a guided procedure if manual probing is enabled.
 */
//#define LCD_BED_LEVELING

#if ENABLED(LCD_BED_LEVELING)
    #define MESH_EDIT_Z_STEP 0.025 // (mm) Step size while manually probing Z axis.
    #define LCD_PROBE_Z_RANGE 4 // (mm) Z Range centered on Z_MIN_POS for LCD Z adjustment
//#define MESH_EDIT_MENU        // Add a menu to edit mesh points
#endif

// Add a menu item to move between bed corners for manual bed adjustment
//#define LEVEL_BED_CORNERS

#if ENABLED(LEVEL_BED_CORNERS)
    #define LEVEL_CORNERS_INSET 30 // (mm) An inset for corner leveling
    #define LEVEL_CORNERS_Z_HOP 4.0 // (mm) Move nozzle up before moving between corners
//#define LEVEL_CENTER_TOO        // Move to the center after the last corner
#endif

/**
 * Commands to execute at the end of G29 probing.
 * Useful to retract or move the Z probe out of the way.
 */
//#define Z_PROBE_END_SCRIPT "G1 Z10 F12000\nG1 X15 Y330\nG1 Z0.5\nG1 Z10"

// @section homing

// The center of the bed is at (X=0, Y=0)
//#define BED_CENTER_AT_0_0

// Manually set the home position. Leave these undefined for automatic settings.
//#define MANUAL_X_HOME_POS 0
//#define MANUAL_Y_HOME_POS 0
//#define MANUAL_Z_HOME_POS 0

// Use "Z Safe Homing" to avoid homing with a Z probe outside the bed area.
//
// With this feature enabled:
//
// - Allow Z homing only after X and Y homing AND stepper drivers still enabled.
// - If stepper drivers time out, it will need X and Y homing again before Z homing.
// - Move the Z probe (or nozzle) to a defined XY point before Z Homing when homing all axes (G28).
// - Prevent Z homing when the Z probe is outside bed area.
//
#define Z_SAFE_HOMING

#if ENABLED(Z_SAFE_HOMING)
    #define Z_SAFE_HOMING_X_POINT (14) // X point for Z homing when homing all axes (G28).
    #define Z_SAFE_HOMING_Y_POINT (-4) // Y point for Z homing when homing all axes (G28).

    #if HAS_PRINT_SHEET_DETECTION()
        #define DETECT_PRINT_SHEET_X_POINT (-1)
        #define DETECT_PRINT_SHEET_Y_POINT (-4)
        #define DETECT_PRINT_SHEET_Z_POINT (-2)
        #define DETECT_PRINT_SHEET_Z_AFTER_FAILURE (100)
    #endif
#endif

// Homing speeds (mm/m)
#define HOMING_FEEDRATE_XY (62 * 60)
#define HOMING_FEEDRATE_Z 1150
#define HOMING_FEEDRATE_INVERTED_Z (buddy::hw::Configuration::Instance().has_trinamic_oscillators() ? 60 : 30) // mm/s

// Validate that endstops are triggered on homing moves
//#define VALIDATE_HOMING_ENDSTOPS

// @section calibrate

//=============================================================================
//============================= Additional Features ===========================
//=============================================================================

// @section extras

//
// EEPROM
//
// The microcontroller can store settings in the EEPROM, e.g. max velocity...
// M500 - stores parameters in EEPROM
// M501 - reads parameters from EEPROM (if you need reset them after you changed them temporarily).
// M502 - reverts to the default "factory settings".  You still need to store them in EEPROM afterwards if you want to.
//
//#define DISABLE_M503    // Saves ~2700 bytes of PROGMEM. Disable for release!

//
// Host Keepalive
//
// When enabled Marlin will send a busy status message to the host
// every couple of seconds when it can't accept commands.
//
#define HOST_KEEPALIVE_FEATURE // Disable this if your host doesn't like keepalive messages
#define DEFAULT_KEEPALIVE_INTERVAL 2 // Number of seconds between "busy" messages. Set with M113.
#define BUSY_WHILE_HEATING // Some hosts require "busy" messages even during heating

// @section temperature

// Preheat Constants
#define PREHEAT_1_LABEL "PLA"
#define PREHEAT_1_TEMP_HOTEND 215
#define PREHEAT_1_TEMP_BED 60
#define PREHEAT_1_FAN_SPEED 0 // Value from 0 to 255

#define PREHEAT_2_LABEL "PET"
#define PREHEAT_2_TEMP_HOTEND 230
#define PREHEAT_2_TEMP_BED 85
#define PREHEAT_2_FAN_SPEED 0 // Value from 0 to 255

/**
 * Nozzle Park
 *
 * Park the nozzle at the given XYZ position on idle or G27.
 *
 * The "P" parameter controls the action applied to the Z axis:
 *
 *    P0  (Default) If Z is below park Z raise the nozzle.
 *    P1  Raise the nozzle always to Z-park height.
 *    P2  Raise the nozzle by Z-park amount, limited to Z_MAX_POS.
 */
    // Specify a park position as { X, Y, Z }
    #define X_NOZZLE_PARK_POINT (X_MAX_POS - 10.0f)
    #define Y_NOZZLE_PARK_POINT 170.0f
    #define Z_NOZZLE_PARK_POINT 20.0f

    /// Always raise the nozzle by this amount when parking on print end
    /// Give the user some space to remove the purge after auto_retract
    #define Z_NOZZLE_PARK_POINT_MIN 40.0f

    #define Z_NOZZLE_PARK_RISE 20.0f // Relative Z rise

    #define XYZ_NOZZLE_PARK_POINT \
        {X_NOZZLE_PARK_POINT, Y_NOZZLE_PARK_POINT, Z_NOZZLE_PARK_POINT}

    #define XYZ_NOZZLE_PARK_POINT_ON_PRINT_END XYZ_NOZZLE_PARK_POINT

    #define XYZ_LOADCELL_SELFTEST_POINT XYZ_NOZZLE_PARK_POINT

    #define X_NOZZLE_PARK_POINT_M600    (X_MAX_POS - 10.0f)
    #define Y_NOZZLE_PARK_POINT_M600    (Y_MIN_POS + 1.0f)
    #define Z_NOZZLE_PARK_POINT_M600    20.0f
    #define Z_NOZZLE_PARK_RISE_M600 Z_NOZZLE_PARK_RISE
    #define XYZ_NOZZLE_PARK_POINT_M600 \
        {X_NOZZLE_PARK_POINT_M600, Y_NOZZLE_PARK_POINT_M600, Z_NOZZLE_PARK_POINT_M600}

    #define NOZZLE_PARK_XY_FEEDRATE 100 // (mm/s) X and Y axes feedrate (also used for delta Z axis)
    #define NOZZLE_PARK_Z_FEEDRATE 5 // (mm/s) Z axis feedrate (not used for delta printers)

    #define X_AXIS_LOAD_POS mapi::ParkingPosition::ParkingPosition::unchanged
    #define Y_AXIS_LOAD_POS mapi::ParkingPosition::ParkingPosition::unchanged
    #define Z_AXIS_LOAD_POS 40.0f

    #define X_AXIS_UNLOAD_POS mapi::ParkingPosition::unchanged
    #define Y_AXIS_UNLOAD_POS mapi::ParkingPosition::unchanged
    #define Z_AXIS_UNLOAD_POS 20.0f

    /**
     * Park the nozzle after print is finished
     * When disabled, similar functionality can be still achieved with slicer "End G-code"
     */
    #define PARK_HEAD_ON_PRINT_FINISH

    #define Z_NOZZLE_CLEANING_FAILED_POINT 60
    #define XYZ_NOZZLE_CLEANINIG_FAILED_POINT \
        {X_NOZZLE_PARK_POINT_M600, Y_NOZZLE_PARK_POINT_M600, Z_NOZZLE_CLEANING_FAILED_POINT}

/**
 * Print Job Timer
 *
 * Automatically start and stop the print job timer on M104/M109/M190.
 *
 *   M104 (hotend, no wait) - high temp = none,        low temp = stop timer
 *   M109 (hotend, wait)    - high temp = start timer, low temp = stop timer
 *   M190 (bed, wait)       - high temp = start timer, low temp = none
 *
 * The timer can also be controlled with the following commands:
 *
 *   M75 - Start the print job timer
 *   M76 - Pause the print job timer
 *   M77 - Stop the print job timer
 */
#define PRINTJOB_TIMER_AUTOSTART

//
// Extensible UI
//
// Enable third-party or vendor customized user interfaces that aren't
// packaged with Marlin. Source code for the user interface will need to
// be placed in "src/lcd/extensible_ui/lib"
//
#define EXTENSIBLE_UI

//=============================================================================
//=============================== Extra Features ==============================
//=============================================================================

// @section extras

//Enable autopower control
//Automaticly turn off power when is not need it
#define AUTO_POWER_CONTROL

//Timeout for disenable psu [s]
#define POWER_TIMEOUT 1

//Enable psu control
#define PSU_CONTROL

//Psu active logit state
#define PSU_ACTIVE_HIGH 1

//Ignore Z axes enable mod
#define POWER_IGNORE_Z 1

// SkeinForge sends the wrong arc g-codes when using Arc Point as fillet procedure
//#define SF_ARC_FIX
