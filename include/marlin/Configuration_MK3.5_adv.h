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

// clang-format off

#include <option/has_precise_homing.h>
#include <option/has_crash_detection.h>
#include <option/has_pause.h>
#include <option/has_power_panic.h>

/**
 * Configuration_adv.h
 *
 * Advanced settings.
 * Only change these if you know exactly what you're doing.
 * Some of these settings can damage your printer if improperly set!
 *
 * Basic settings can be found in Configuration.h
 *
 */
#define CONFIGURATION_ADV_H_VERSION 020000

// @section temperature

//===========================================================================
//=============================Thermal Settings  ============================
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
 * The solution: Once the temperature reaches the target, start observing.
 * If the temperature stays too far below the target (hysteresis) for too
 * long (period), the firmware will halt the machine as a safety precaution.
 *
 * If you get false positives for "Thermal Runaway", increase
 * THERMAL_PROTECTION_HYSTERESIS and/or THERMAL_PROTECTION_PERIOD
 */
#if ENABLED(THERMAL_PROTECTION_HOTENDS)
    #define THERMAL_PROTECTION_PERIOD 60 //40        // Seconds
    #define THERMAL_PROTECTION_HYSTERESIS 12 //4     // Degrees Celsius

    /**
   * Whenever an M104, M109, or M303 increases the target temperature, the
   * firmware will wait for the WATCH_TEMP_PERIOD to expire. If the temperature
   * hasn't increased by WATCH_TEMP_INCREASE degrees, the machine is halted and
   * requires a hard reset. This test restarts with any M104/M109/M303, but only
   * if the current temperature is far enough below the target for a reliable
   * test.
   *
   * If you get false positives for "Heating failed", increase WATCH_TEMP_PERIOD
   * and/or decrease WATCH_TEMP_INCREASE. WATCH_TEMP_INCREASE should not be set
   * below 2.
   */
    #define WATCH_TEMP_PERIOD 20 // Seconds
    #define WATCH_TEMP_INCREASE 2 // Degrees Celsius
#endif

/**
 * Thermal Protection parameters for the bed are just as above for hotends.
 */
#if ENABLED(THERMAL_PROTECTION_BED)
    #define THERMAL_PROTECTION_BED_PERIOD 60 // Seconds
    #define THERMAL_PROTECTION_BED_HYSTERESIS 10 // Degrees Celsius

    /**
   * As described above, except for the bed (M140/M190/M303).
   */
    #define WATCH_BED_TEMP_PERIOD 60 // Seconds
    #define WATCH_BED_TEMP_INCREASE 2 // Degrees Celsius
#endif

#if ENABLED(PIDTEMP)
    // this adds an experimental additional term to the heating power, proportional to the extrusion speed.
    // if Kc is chosen well, the additional required power due to increased melting should be compensated.
    //#define PID_EXTRUSION_SCALING
    #if ENABLED(PID_EXTRUSION_SCALING)
        /**
         * Increase in PWM duty cycle needed to to extrude 1 mm^2 per second
         * of filament if extruder temperature is 1 Kelvin above ambient
         * temperature [s*K^-1*mm^-2]
         */
        #define DEFAULT_Kc 0.009517f
    #endif
#endif

// @section extruder

/**
 * Extruder cooling fans
 *
 * Extruder auto fans automatically turn on when their extruders'
 * temperatures go above EXTRUDER_AUTO_FAN_TEMPERATURE.
 *
 * Your board's pins file specifies the recommended pins. Override those here
 * or set to -1 to disable completely.
 *
 * Multiple extruders can be assigned to the same pin in which case
 * the fan will turn on when any selected extruder is above the threshold.
 */
#define HAS_HOTEND_AUTO_FAN
#define EXTRUDER_AUTO_FAN_TEMPERATURE 50

/**
 * M355 Case Light on-off / brightness
 */
//#define CASE_LIGHT_ENABLE
#if ENABLED(CASE_LIGHT_ENABLE)
    //#define CASE_LIGHT_PIN 4                  // Override the default pin if needed
    #define INVERT_CASE_LIGHT false // Set true if Case Light is ON when pin is LOW
    #define CASE_LIGHT_DEFAULT_ON true // Set default power-up state on
    #define CASE_LIGHT_DEFAULT_BRIGHTNESS 105 // Set default power-up brightness (0-255, requires PWM pin)
    //#define MENU_ITEM_CASE_LIGHT              // Add a Case Light option to the LCD main menu
#endif

//===========================================================================
//============================ Mechanical Settings ==========================
//===========================================================================

// @section homing

// If you want endstops to stay on (by default) even when not homing
// enable this option. Override at any time with M120, M121.
//#define ENDSTOPS_ALWAYS_ON_DEFAULT

// @section extras

//#define Z_LATE_ENABLE // Enable Z the last moment. Needed if your Z driver overheats.

//#define Z_TRIPLE_STEPPER_DRIVERS
#if ENABLED(Z_TRIPLE_STEPPER_DRIVERS)
    //#define Z_TRIPLE_ENDSTOPS
    #if ENABLED(Z_TRIPLE_ENDSTOPS)
        #define Z2_USE_ENDSTOP _XMAX_
        #define Z3_USE_ENDSTOP _YMAX_
        #define Z_TRIPLE_ENDSTOPS_ADJUSTMENT2 0
        #define Z_TRIPLE_ENDSTOPS_ADJUSTMENT3 0
    #endif
#endif

// @section homing

//kill command after probing fails
//#define HALT_ON_PROBING_ERROR
//after enabling HOMING_MAX_ATTEMPTS, homing can fail
#if HAS_PRECISE_HOMING() // we do not want homing to fail, when precise homing is disabled
    #define HOMING_MAX_ATTEMPTS 10
#endif

// Homing hits each endstop, retracts by these distances, then does a slower bump.
#define X_HOME_BUMP_MM 10
#define Y_HOME_BUMP_MM 10
#define Z_HOME_BUMP_MM 3
#define HOMING_BUMP_DIVISOR \
    { 1, 1, 1 } // Re-Bump Speed Divisor (Divides the Homing Feedrate)
#define HOMING_BUMP_DIVISOR_MAX \
    { 1.09f, 1.09f, 1 }
#define HOMING_BUMP_DIVISOR_MIN \
    { 0.96f, 0.96f, 1 }


// If homing includes X and Y, do a diagonal move initially
#define QUICK_HOME

// Move away from the endstops after homing
#define HOMING_BACKOFF_POST_MM { 2, 2, 0 }

// When G28 is called, this option will make Y home before X
//#define HOME_Y_BEFORE_X

// Enable this if X or Y can't home without homing the other axis first.
//#define CODEPENDENT_XY_HOMING

/**
 * Z Steppers Auto-Alignment
 * Add the G34 command to align multiple Z steppers using a bed probe.
 */
//#define Z_STEPPER_AUTO_ALIGN
#if ENABLED(Z_STEPPER_AUTO_ALIGN)
    // Define probe X and Y positions for Z1, Z2 [, Z3]
    #define Z_STEPPER_ALIGN_X \
        { 10, 150, 290 }
    #define Z_STEPPER_ALIGN_Y \
        { 290, 10, 290 }
    // Set number of iterations to align
    #define Z_STEPPER_ALIGN_ITERATIONS 3
    // Enable to restore leveling setup after operation
    #define RESTORE_LEVELING_AFTER_G34
    // Use the amplification factor to de-/increase correction step.
    // In case the stepper (spindle) position is further out than the test point
    // Use a value > 1. NOTE: This may cause instability
    #define Z_STEPPER_ALIGN_AMP 1.0
    // Stop criterion. If the accuracy is better than this stop iterating early
    #define Z_STEPPER_ALIGN_ACC 0.02
#endif

// @section machine

#define AXIS_RELATIVE_MODES \
    { false, false, false, false }

// By default pololu step drivers require an active high signal. However, some high power drivers require an active low signal as step.
#define INVERT_X_STEP_PIN false
#define INVERT_Y_STEP_PIN false
#define INVERT_Z_STEP_PIN false
#define INVERT_E_STEP_PIN false

// Default stepper release if idle. Set to 0 to deactivate.
// Steppers will shut down DEFAULT_STEPPER_DEACTIVE_TIME seconds after the last move when DISABLE_INACTIVE_? is true.
// Time can be set by M18 and M84.
#define DEFAULT_STEPPER_DEACTIVE_TIME 120
#define DISABLE_INACTIVE_X true
#define DISABLE_INACTIVE_Y true
#define DISABLE_INACTIVE_Z false // set to false if the nozzle will fall down on your printed part when print has finished.
#define DISABLE_INACTIVE_E true

#define Z_ALWAYS_ON // Keep Z motors enabled all the time (Calls enable_Z(); in Marlin.cpp in setup)

#define DEFAULT_MINIMUMFEEDRATE 0.0 // minimum feedrate
#define DEFAULT_MINTRAVELFEEDRATE 0.0

#define HOME_AFTER_DEACTIVATE  // Require rehoming after steppers are deactivated

// @section lcd

#define MANUAL_FEEDRATE \
    { 50 * 60, 50 * 60, 12 * 60, 4 * 60 } // Feedrates for manual moves along X, Y, Z, E from panel

// @section extras

// minimum time in microseconds that a movement needs to take if the buffer is emptied.
#define DEFAULT_MINSEGMENTTIME 20000

// If defined the movements slow down when the look ahead buffer is only half full
//#define SLOWDOWN

// Frequency limit
// See nophead's blog for more info
// Not working O
//#define XY_FREQUENCY_LIMIT  15

// Minimum planner junction speed. Sets the default minimum speed the planner plans for at the end
// of the buffer and all stops. This should not be much greater than zero and should only be changed
// if unwanted behavior is observed on a user's machine when running at very slow speeds.
#define MINIMUM_PLANNER_SPEED 0.05 // (mm/s)

//
// Backlash Compensation
// Adds extra movement to axes on direction-changes to account for backlash.
//
//#define BACKLASH_COMPENSATION
#if ENABLED(BACKLASH_COMPENSATION)
    // Define values for backlash distance and correction.
    // If BACKLASH_GCODE is enabled these values are the defaults.
    #define BACKLASH_DISTANCE_MM \
        { 0, 0, 0 } // (mm)
    #define BACKLASH_CORRECTION 0.0 // 0.0 = no correction; 1.0 = full correction

// Set BACKLASH_SMOOTHING_MM to spread backlash correction over multiple segments
// to reduce print artifacts. (Enabling this is costly in memory and computation!)
//#define BACKLASH_SMOOTHING_MM 3 // (mm)

// Add runtime configuration and tuning of backlash values (M425)
//#define BACKLASH_GCODE

    #if ENABLED(BACKLASH_GCODE)
        // Measure the Z backlash when probing (G29) and set with "M425 Z"
        #define MEASURE_BACKLASH_WHEN_PROBING

        #if ENABLED(MEASURE_BACKLASH_WHEN_PROBING)
            // When measuring, the probe will move up to BACKLASH_MEASUREMENT_LIMIT
            // mm away from point of contact in BACKLASH_MEASUREMENT_RESOLUTION
            // increments while checking for the contact to be broken.
            #define BACKLASH_MEASUREMENT_LIMIT 0.5 // (mm)
            #define BACKLASH_MEASUREMENT_RESOLUTION 0.005 // (mm)
            #define BACKLASH_MEASUREMENT_FEEDRATE Z_PROBE_SPEED_SLOW // (mm/m)
        #endif
    #endif
#endif

/**
 * Adaptive Step Smoothing increases the resolution of multi-axis moves, particularly at step frequencies
 * below 1kHz (for AVR) or 10kHz (for ARM), where aliasing between axes in multi-axis moves causes audible
 * vibration and surface artifacts. The algorithm adapts to provide the best possible step smoothing at the
 * lowest stepping frequencies.
 */
//#define ADAPTIVE_STEP_SMOOTHING

/**
 * Custom Microstepping
 * Override as-needed for your setup. Up to 3 MS pins are supported.
 */
//#define MICROSTEP1 LOW,LOW,LOW
//#define MICROSTEP2 HIGH,LOW,LOW
//#define MICROSTEP4 LOW,HIGH,LOW
//#define MICROSTEP8 HIGH,HIGH,LOW
//#define MICROSTEP16 LOW,LOW,HIGH
//#define MICROSTEP32 HIGH,LOW,HIGH

// Microstep setting (Only functional when stepper driver microstep pins are connected to MCU.
//#define MICROSTEP_MODES { 16, 16, 16, 16, 16, 16 } // [1,2,4,8,16]

/**
 *  @section  stepper motor current
 *
 *  Some boards have a means of setting the stepper motor current via firmware.
 *
 *  The power on motor currents are set by:
 *    PWM_MOTOR_CURRENT - used by MINIRAMBO & ULTIMAIN_2
 *                         known compatible chips: A4982
 *    DAC_MOTOR_CURRENT_DEFAULT - used by PRINTRBOARD_REVF & RIGIDBOARD_V2
 *                         known compatible chips: MCP4728
 *
 *  Motor currents can also be set by M907 - M910 and by the LCD.
 *    M907 - applies to all.
 *    M908 - BQ_ZUM_MEGA_3D, RAMBO, PRINTRBOARD_REVF, RIGIDBOARD_V2 & SCOOVO_X9H
 *    M909, M910 & LCD - only PRINTRBOARD_REVF & RIGIDBOARD_V2
 */
//#define PWM_MOTOR_CURRENT { 1300, 1300, 1250 }          // Values in milliamps
//#define DAC_MOTOR_CURRENT_DEFAULT { 70, 80, 90, 80 }    // Default drive percent - X, Y, Z, E axis

//===========================================================================
//=============================Additional Features===========================
//===========================================================================

// @section lcd

// Change values more rapidly when the encoder is rotated faster
#define ENCODER_RATE_MULTIPLIER
#if ENABLED(ENCODER_RATE_MULTIPLIER)
    #define ENCODER_10X_STEPS_PER_SEC 30 // (steps/s) Encoder rate for 10x speed
    #define ENCODER_100X_STEPS_PER_SEC 80 // (steps/s) Encoder rate for 100x speed
#endif

// Play a beep when the feedrate is changed from the Status Screen
//#define BEEP_ON_FEEDRATE_CHANGE
#if ENABLED(BEEP_ON_FEEDRATE_CHANGE)
    #define FEEDRATE_CHANGE_BEEP_DURATION 10
    #define FEEDRATE_CHANGE_BEEP_FREQUENCY 440
#endif

// @section lcd

/**
 * Babystepping enables movement of the axes by tiny increments without changing
 * the current position values. This feature is used primarily to adjust the Z
 * axis in the first layer of a print in real-time.
 *
 * Warning: Does not respect endstops!
 */
#define BABYSTEPPING
#if ENABLED(BABYSTEPPING)
    #define BABYSTEP_WITHOUT_HOMING
    //#define BABYSTEP_XY                     // Also enable X/Y Babystepping.
    #define BABYSTEP_INVERT_Z false // Change if Z babysteps should go the other way
    #define BABYSTEP_MULTIPLICATOR 1 // Babysteps are very small. Increase for faster motion.

    //#define DOUBLECLICK_FOR_Z_BABYSTEPPING  // Double-click on the Status Screen for Z Babystepping.
    #if ENABLED(DOUBLECLICK_FOR_Z_BABYSTEPPING)
        #define DOUBLECLICK_MAX_INTERVAL 1250 // Maximum interval between clicks, in milliseconds.
            // Note: Extra time may be added to mitigate controller latency.
        #define BABYSTEP_ALWAYS_AVAILABLE // Allow babystepping at all times (not just during movement).
    #endif
#endif

// @section leveling

#if ENABLED(AUTO_BED_LEVELING_UBL)
// Override the mesh area if the automatic (max) area is too large
// The numbers are from the distance equation combined with relative position due to probe offset MESH_X_DIST define and function position_reachable_by_probe from motion.h respectively

// 1|19 for corner probe positions
// 20 for number of travels between probe points (21 set by Marlin)
// offset is defined in printer config
// bounds were found by trial and error using G0 and looking whether position is "nice"

// min + 1 * (max - min) / 20 - offset = low_bound;
// min + 19 * (max - min) / 20 - offset = high_bound;.
// Highbounds:
// Rightmost position for X is 220 (to ensure P.I.N.D.A. stays on the sheet)
// Rearmost postition for Y is 204
// Lowbounds:
// Leftmost position for X is 0
// Frontmost position for Y is -4 (defined elsewhere: Y_MIN_POS)

#define MESH_MIN_X (10.5f)
#define MESH_MIN_Y (-10.5f)
#define MESH_MAX_X (X_BED_SIZE + MESH_MIN_X - 5)
#define MESH_MAX_Y (Y_BED_SIZE - MESH_MIN_Y)
#endif


// @section extras

//
// G2/G3 Arc Support
//
#define ARC_SUPPORT                      // Requires ~3226 bytes
#if ENABLED(ARC_SUPPORT)
  #define MIN_ARC_SEGMENT_MM        0.1  // (mm) Minimum length of each arc segment
  #define MAX_ARC_SEGMENT_MM        2.0  // (mm) Maximum length of each arc segment
  #define MIN_ARC_SEGMENTS_PER_SEC 50    // Use the feedrate to choose the segment length
  #define MAX_ARC_DEVIATION         0.02 // Maximum deviation from ideal arc due to segmentation
  #define N_ARC_CORRECTION         25    // Number of interpolated segments between corrections
  //#define ARC_P_CIRCLES                // Enable the 'P' parameter to specify complete circles
  //#define SF_ARC_FIX                   // Enable only if using SkeinForge with "Arc Point" fillet procedure
#endif

// G5 Bézier Curve Support with XYZE destination and IJPQ offsets
//#define BEZIER_CURVE_SUPPORT        // Requires ~2666 bytes

#if EITHER(ARC_SUPPORT, BEZIER_CURVE_SUPPORT)
  //#define CNC_WORKSPACE_PLANES      // Allow G2/G3/G5 to operate in XY, ZX, or YZ planes
#endif

/**
 * Direct Stepping
 *
 * Comparable to the method used by Klipper, G6 direct stepping significantly
 * reduces motion calculations, increases top printing speeds, and results in
 * less step aliasing by calculating all motions in advance.
 * Preparing your G-code: https://github.com/colinrgodsey/step-daemon
 */
//#define DIRECT_STEPPING

/**
 * G38 Probe Target
 *
 * This option adds G38.2 and G38.3 (probe towards target)
 * and optionally G38.4 and G38.5 (probe away from target).
 * Set MULTIPLE_PROBING for G38 to probe more than once.
 */
//#define G38_PROBE_TARGET
#if ENABLED(G38_PROBE_TARGET)
    //#define G38_PROBE_AWAY        // Include G38.4 and G38.5 to probe away from target
    #define G38_MINIMUM_MOVE 0.0275 // (mm) Minimum distance that will produce a move.
#endif

// Moves (or segments) with fewer steps than this will be joined with the next move
#define MIN_STEPS_PER_SEGMENT 6

/**
 * Minimum delay after setting the stepper DIR (in ns)
 *     0 : No delay (Expect at least 10µS since one Stepper ISR must transpire)
 *    20 : Minimum for TMC2xxx drivers
 *   200 : Minimum for A4988 drivers
 *   400 : Minimum for A5984 drivers
 *   500 : Minimum for LV8729 drivers (guess, no info in datasheet)
 *   650 : Minimum for DRV8825 drivers
 *  1500 : Minimum for TB6600 drivers (guess, no info in datasheet)
 * 15000 : Minimum for TB6560 drivers (guess, no info in datasheet)
 *
 * Override the default value based on the driver type set in Configuration.h.
 */
//#define MINIMUM_STEPPER_DIR_DELAY 650

/**
 * Minimum stepper driver pulse width (in µs)
 *   0 : Smallest possible width the MCU can produce, compatible with TMC2xxx drivers
 *   1 : Minimum for A4988, A5984, and LV8729 stepper drivers
 *   2 : Minimum for DRV8825 stepper drivers
 *   3 : Minimum for TB6600 stepper drivers
 *  30 : Minimum for TB6560 stepper drivers
 *
 * Override the default value based on the driver type set in Configuration.h.
 */
//#define MINIMUM_STEPPER_PULSE 2

/**
 * Maximum stepping rate (in Hz) the stepper driver allows
 *  If undefined, defaults to 1MHz / (2 * MINIMUM_STEPPER_PULSE)
 *  500000 : Maximum for A4988 stepper driver
 *  400000 : Maximum for TMC2xxx stepper drivers
 *  250000 : Maximum for DRV8825 stepper driver
 *  150000 : Maximum for TB6600 stepper driver
 *  130000 : Maximum for LV8729 stepper driver
 *   15000 : Maximum for TB6560 stepper driver
 *
 * Override the default value based on the driver type set in Configuration.h.
 */
//#define MAXIMUM_STEPPER_RATE 250000

// @section temperature

//===========================================================================
//================================= Buffers =================================
//===========================================================================

// @section hidden

// The number of linear motions that can be in the plan at any give time.
// THE BLOCK_BUFFER_SIZE NEEDS TO BE A POWER OF 2 (e.g. 8, 16, 32) because shifts and ors are used to do the ring-buffering.
#define BLOCK_BUFFER_SIZE 32

#define MOVE_SEGMENT_QUEUE_SIZE (2 * BLOCK_BUFFER_SIZE)
#define STEP_EVENT_QUEUE_SIZE   1024
#define ADVANCED_STEP_GENERATORS 1

// @section serial

// The ASCII buffer for serial input
#define MAX_CMD_SIZE 96
#define BUFSIZE 8

// Transmission to Host Buffer Size
// To save 386 bytes of PROGMEM (and TX_BUFFER_SIZE+3 bytes of RAM) set to 0.
// To buffer a simple "ok" you need 4 bytes.
// For ADVANCED_OK (M105) you need 32 bytes.
// For debug-echo: 128 bytes for the optimal speed.
// Other output doesn't need to be that speedy.
// :[0, 2, 4, 8, 16, 32, 64, 128, 256]
#define TX_BUFFER_SIZE 0

// Host Receive Buffer Size
// Without XON/XOFF flow control (see SERIAL_XON_XOFF below) 32 bytes should be enough.
// To use flow control, set this buffer size to at least 1024 bytes.
// :[0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048]
//#define RX_BUFFER_SIZE 1024

#if RX_BUFFER_SIZE >= 1024
// Enable to have the controller send XON/XOFF control characters to
// the host to signal the RX buffer is becoming full.
//#define SERIAL_XON_XOFF
#endif

// Enable an emergency-command parser to intercept certain commands as they
// enter the serial receive buffer, so they cannot be blocked.
// Currently handles M108, M112, M410
// Does not work on boards using AT90USB (USBCON) processors!
//#define EMERGENCY_PARSER

// Bad Serial-connections can miss a received command by sending an 'ok'
// Therefore some clients abort after 30 seconds in a timeout.
// Some other clients start sending commands while receiving a 'wait'.
// This "wait" is only sent when the buffer is empty. 1 second is a good value here.
//#define NO_TIMEOUTS 1000 // Milliseconds

// Some clients will have this feature soon. This could make the NO_TIMEOUTS unnecessary.
//#define ADVANCED_OK

// Printrun may have trouble receiving long strings all at once.
// This option inserts short delays between lines of serial output.
#define SERIAL_OVERRUN_PROTECTION

// @section extras

/**
 * Universal tool change settings.
 * Applies to all types of extruders except where explicitly noted.
 */
#if EXTRUDERS > 1
    // Z raise distance for tool-change, as needed for some extruders
    #define TOOLCHANGE_ZRAISE 2 // (mm)
#endif

/**
 * Advanced Pause
 * Experimental feature for filament change support and for parking the nozzle when paused.
 * Adds the GCode M600 for initiating filament change.
 *
 * Requires an LCD display.
 */
#if HAS_PAUSE()
    #define FILAMENT_MMU2_RAMMING_SEQUENCE \
        { \
            { 20, 1500 / 60.F}, \
            { -50, 2700 / 60.F}, \
            { -5, 50 / 60.F}, \
            { -50, 1500 / 60.F}, \
        }

    #define PARK_PAUSE_PRIME_LENGTH 4 // (mm) Length of a priming "poop".

    #define STANDARD_RETRACT_LENGTH 8 // (mm) Initial retract.
        // This retract is done immediately, before parking the nozzle.
    #define FILAMENT_CHANGE_UNLOAD_ACCEL 1100 // (mm/s^2) Lower acceleration may allow a faster feedrate.
    #define FILAMENT_CHANGE_UNLOAD_LENGTH 105 // (mm) The length of filament for a complete unload.
        //   For Bowden, the full length of the tube and nozzle.
        //   For direct drive, the full length of the nozzle.
        //   Set to 0 for manual unloading.

    #define FILAMENT_CHANGE_SLOW_LOAD_LENGTH 30 // (mm) Slow length, to allow time to insert material. //60
        // 0 to disable start loading and skip to fast load only

    #define FILAMENT_CHANGE_FAST_LOAD_ACCEL 100 // (mm/s^2) Lower acceleration may allow a faster feedrate.  //200
    #define FILAMENT_CHANGE_FAST_LOAD_LENGTH 50 // (mm) Load length of filament, from extruder gear to nozzle. //75
        //   For Bowden, the full length of the tube and nozzle.
        //   For direct drive, the full length of the nozzle.

    #define ADVANCED_PAUSE_PURGE_LENGTH 21 // (mm) Length to extrude after loading. //50
#endif

// @section tmc

/**
 * TMC26X Stepper Driver options
 *
 * The TMC26XStepper library is required for this stepper driver.
 * https://github.com/trinamic/TMC26XStepper
 */
#if HAS_DRIVER(TMC26X)

    #if AXIS_DRIVER_TYPE_X(TMC26X)
        #define X_MAX_CURRENT 1000 // (mA)
        #define X_SENSE_RESISTOR 91 // (mOhms)
        #define X_MICROSTEPS 16 // Number of microsteps
    #endif

    #if AXIS_DRIVER_TYPE_Y(TMC26X)
        #define Y_MAX_CURRENT 1000
        #define Y_SENSE_RESISTOR 91
        #define Y_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_Z(TMC26X)
        #define Z_MAX_CURRENT 1000
        #define Z_SENSE_RESISTOR 91
        #define Z_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_Z2(TMC26X)
        #define Z2_MAX_CURRENT 1000
        #define Z2_SENSE_RESISTOR 91
        #define Z2_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_Z3(TMC26X)
        #define Z3_MAX_CURRENT 1000
        #define Z3_SENSE_RESISTOR 91
        #define Z3_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_E0(TMC26X)
        #define E0_MAX_CURRENT 1000
        #define E0_SENSE_RESISTOR 91
        #define E0_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_E1(TMC26X)
        #define E1_MAX_CURRENT 1000
        #define E1_SENSE_RESISTOR 91
        #define E1_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_E2(TMC26X)
        #define E2_MAX_CURRENT 1000
        #define E2_SENSE_RESISTOR 91
        #define E2_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_E3(TMC26X)
        #define E3_MAX_CURRENT 1000
        #define E3_SENSE_RESISTOR 91
        #define E3_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_E4(TMC26X)
        #define E4_MAX_CURRENT 1000
        #define E4_SENSE_RESISTOR 91
        #define E4_MICROSTEPS 16
    #endif

    #if AXIS_DRIVER_TYPE_E5(TMC26X)
        #define E5_MAX_CURRENT 1000
        #define E5_SENSE_RESISTOR 91
        #define E5_MICROSTEPS 16
    #endif

#endif // TMC26X

// @section tmc_smart

/**
 * To use TMC2130, TMC2160, TMC2660, TMC5130, TMC5160 stepper drivers in SPI mode
 * connect your SPI pins to the hardware SPI interface on your board and define
 * the required CS pins in your `pins_MYBOARD.h` file. (e.g., RAMPS 1.4 uses AUX3
 * pins `X_CS_PIN 53`, `Y_CS_PIN 49`, etc.).
 * You may also use software SPI if you wish to use general purpose IO pins.
 *
 * To use TMC2208 stepper UART-configurable stepper drivers connect #_SERIAL_TX_PIN
 * to the driver side PDN_UART pin with a 1K resistor.
 * To use the reading capabilities, also connect #_SERIAL_RX_PIN to PDN_UART without
 * a resistor.
 * The drivers can also be used with hardware serial.
 *
 * TMCStepper library is required to use TMC stepper drivers.
 * https://github.com/teemuatlut/TMCStepper
 */
#if HAS_TRINAMIC

    constexpr float HOLD_MULTIPLIER[4] = {1, 1, 1, 1};  // Scales down the holding current from run current
    #define INTERPOLATE true // Interpolate X/Y/Z_MICROSTEPS to 256

    #if AXIS_IS_TMC(X)
        #define X_CURRENT 300 // (mA) RMS current.  MK3 motors
        #define X_MICROSTEPS 16 // 0..256
        #define X_RSENSE 0.22f
        #define X_CHAIN_POS 0
    #endif

    #if AXIS_IS_TMC(Y)
        #define Y_CURRENT 370 // (mA) RMS current.  MK3 motors
        #define Y_MICROSTEPS 16
        #define Y_RSENSE 0.22f
        #define Y_CHAIN_POS 0
    #endif

    #if AXIS_IS_TMC(Z)
        #define Z_CURRENT 600
        #define Z_MICROSTEPS 16
        #define Z_RSENSE 0.22f
        #define Z_CHAIN_POS 0
    #endif

    #if AXIS_IS_TMC(Z2)
        #define Z2_CURRENT 800
        #define Z2_MICROSTEPS 16
        #define Z2_RSENSE 0.11
    #endif

    #if AXIS_IS_TMC(Z3)
        #define Z3_CURRENT 800
        #define Z3_MICROSTEPS 16
        #define Z3_RSENSE 0.11
    #endif

    #if AXIS_IS_TMC(E0)
        #define E0_CURRENT 490
        #define E0_MICROSTEPS 32
        #define E0_RSENSE 0.22f
        #define E0_CHAIN_POS 0
    #endif

    #if AXIS_IS_TMC(E1)
        #define E1_CURRENT 800
        #define E1_MICROSTEPS 16
        #define E1_RSENSE 0.11
    #endif

    #if AXIS_IS_TMC(E2)
        #define E2_CURRENT 800
        #define E2_MICROSTEPS 16
        #define E2_RSENSE 0.11
    #endif

    #if AXIS_IS_TMC(E3)
        #define E3_CURRENT 800
        #define E3_MICROSTEPS 16
        #define E3_RSENSE 0.11
    #endif

    #if AXIS_IS_TMC(E4)
        #define E4_CURRENT 800
        #define E4_MICROSTEPS 16
        #define E4_RSENSE 0.11
    #endif

    #if AXIS_IS_TMC(E5)
        #define E5_CURRENT 800
        #define E5_MICROSTEPS 16
        #define E5_RSENSE 0.11
    #endif

    /**
   * Override default SPI pins for TMC2130, TMC2160, TMC2660, TMC5130 and TMC5160 drivers here.
   * The default pins can be found in your board's pins file.
   */
    //#define X_CS_PIN          -1
    //#define Y_CS_PIN          -1
    //#define Z_CS_PIN          -1
    //#define Z2_CS_PIN         -1
    //#define Z3_CS_PIN         -1
    //#define E0_CS_PIN         -1
    //#define E1_CS_PIN         -1
    //#define E2_CS_PIN         -1
    //#define E3_CS_PIN         -1
    //#define E4_CS_PIN         -1
    //#define E5_CS_PIN         -1

    /**
   * Use software SPI for TMC2130.
   * Software option for SPI driven drivers (TMC2130, TMC2160, TMC2660, TMC5130 and TMC5160).
   * The default SW SPI pins are defined the respective pins files,
   * but you can override or define them here.
   */
    //#define TMC_USE_SW_SPI
    //#define TMC_SW_MOSI       -1
    //#define TMC_SW_MISO       -1
    //#define TMC_SW_SCK        -1

    /**
   * TMC2130, TMC2160, TMC2208, TMC5130 and TMC5160 only
   * Use Trinamic's ultra quiet stepping mode.
   * When disabled, Marlin will use spreadCycle stepping mode.
   */
    #define STEALTHCHOP_Z

    /**
   * Optimize spreadCycle chopper parameters by using predefined parameter sets
   * or with the help of an example included in the library.
   * Provided parameter sets are
   * CHOPPER_DEFAULT_12V
   * CHOPPER_DEFAULT_19V
   * CHOPPER_DEFAULT_24V
   * CHOPPER_DEFAULT_36V
   * CHOPPER_PRUSAMK3_24V // Imported parameters from the official Prusa firmware for MK3 (24V)
   * CHOPPER_MARLIN_119   // Old defaults from Marlin v1.1.9
   *
   * Define you own with
   * { <off_time[1..15]>, <hysteresis_end[-3..12]>, hysteresis_start[1..8] }
   */
    #define CHOPPER_TIMING CHOPPER_PRUSAMK3_24V

/**
   * Monitor Trinamic drivers for error conditions,
   * like overtemperature and short to ground. TMC2208 requires hardware serial.
   * In the case of overtemperature Marlin can decrease the driver current until error condition clears.
   * Other detected conditions can be used to stop the current print.
   * Relevant g-codes:
   * M906 - Set or get motor current in milliamps using axis codes X, Y, Z, E. Report values if no axis codes given.
   * M911 - Report stepper driver overtemperature pre-warn condition.
   * M912 - Clear stepper driver overtemperature pre-warn condition flag.
   * M122 - Report driver parameters (Requires TMC_DEBUG)
   */
#define MONITOR_DRIVER_STATUS

    #if ENABLED(MONITOR_DRIVER_STATUS)
        #define SHOW_ERROR_AFTER_OVERTEMP
        #define CURRENT_STEP_DOWN 0 // disabled
        #define REPORT_CURRENT_CHANGE
        #define STOP_ON_ERROR
    #endif

/**
   * TMC2130, TMC2160, TMC2208, TMC5130 and TMC5160 only
   * The driver will switch to spreadCycle when stepper speed is over HYBRID_THRESHOLD.
   * This mode allows for faster movements at the expense of higher noise levels.
   * STEALTHCHOP_(XY|Z|E) must be enabled to use HYBRID_THRESHOLD.
   * M913 X/Y/Z/E to live tune the setting
   */
#define HYBRID_THRESHOLD

    #define X_HYBRID_THRESHOLD 1000 // [mm/s]
    #define Y_HYBRID_THRESHOLD 1000
    #define Z_HYBRID_THRESHOLD 1
    #define E0_HYBRID_THRESHOLD 7

/**
 * Provides crash detection during printing and proper crash recovery.
 * Sensorless homing must be turned on and sensitivities set accordingly.
 */
#if HAS_CRASH_DETECTION()
    #define CRASH_STALL_GUARD { 2, 2 }    // internal value representing sensitivity
    #define CRASH_MAX_PERIOD { 210, 210 } // (steps per tick) - reciprocal value of minimal speed
    #define CRASH_FILTER (false)          // Stallguard filtering for crash detection
    #define CRASH_TIMER 45                // seconds before counter reset
    #define CRASH_COUNTER_MAX 3           // max crashes with automatic recovery
#endif

/**
 * Measure and check axis length on repeated crashes
 */
#define AXIS_MEASURE
#ifdef AXIS_MEASURE
    #define AXIS_MEASURE_STALL_GUARD 1
    #define AXIS_MEASURE_CRASH_PERIOD 210
#endif

/**
 * Recovery from power failure. This is a distinct implementation from
 * POWER_LOSS_RECOVERY specific to Prusa printers.
 */
#if HAS_POWER_PANIC()
    #define POWER_PANIC_Z_LIFT_CYCLES 4 // 4xFullStep cycles = ~0.64mm
    #define POWER_PANIC_MAX_BED_DIFF 10 // Maximum bed temperature (C) difference for auto-recovery

    // seconds to wait on hold before auto-restarting during short power failures
    #define POWER_PANIC_HOLD_RST_MS 5000

    #define POWER_PANIC_X_CURRENT 350 // (mA) RMS current for parking
    #define POWER_PANIC_X_FEEDRATE 200 // (mm/s, running at POWER_PANIC_X_CURRENT)

    #define POWER_PANIC_Z_CURRENT 350 // (mA) RMS current _after_ alignment
    #define POWER_PANIC_Z_FEEDRATE 50 // (mm/s, running at default current)

    #define POWER_PANIC_E_CURRENT 300 // (mA) RMS current
#endif

/**
   * TMC2130, TMC2160, TMC2209, TMC2660, TMC5130, and TMC5160 only
   * Use StallGuard2 to sense an obstacle and trigger an endstop.
   * Connect the stepper driver's DIAG1 pin to the X/Y endstop pin.
   * X, Y, and Z homing will always be done in spreadCycle mode.
   *
   * X/Y/Z_STALL_SENSITIVITY is used for tuning the trigger sensitivity.
   * Higher values make the system LESS sensitive.
   * Lower value make the system MORE sensitive.
   * Too low values can lead to false positives, while too high values will collide the axis without triggering.
   * It is advised to set X/Y/Z_HOME_BUMP_MM to 0.
   * M914 X/Y/Z to live tune the setting
   *
   * Stall threshold defines maximal period between steps to trigger a stallguard
   */
#define SENSORLESS_HOMING

/**
   * Use StallGuard2 to probe the bed with the nozzle.
   *
   * CAUTION: This could cause damage to machines that use a lead screw or threaded rod
   *          to move the Z axis. Take extreme care when attempting to enable this feature.
   */
//#define SENSORLESS_PROBING

    #if EITHER(SENSORLESS_HOMING, SENSORLESS_PROBING)
        // The range of stallguard sensitivities to probe and calibrate
        // (the required sensitivity varies by motor)
        #define XY_STALL_SENSITIVITY_MIN -7
        #define XY_STALL_SENSITIVITY_MAX -2

        // Read from config. May be int16 max if uncalibrated, which is
        // then handled in the Crash_s class.
        #define X_STALL_SENSITIVITY config_store().homing_sens_x.get()

        // Read from config. May be int16 max if uncalibrated, which is
        // then handled in the Crash_s class.
        #define Y_STALL_SENSITIVITY config_store().homing_sens_y.get()

        #define Z_STALL_SENSITIVITY 4

        #define STALL_THRESHOLD_TMC2130 400
        #define STALL_THRESHOLD_TMC2209 400

        #define IMPROVE_HOMING_RELIABILITY
        #ifdef IMPROVE_HOMING_RELIABILITY
            #define XY_HOMING_ACCELERATION 1250
            #define XY_HOMING_JERK 8
        #endif
    #endif

    // Create a 50/50 square wave step pulse optimal for stepper drivers.
    #define SQUARE_WAVE_STEPPING

    /**
   * Enable M122 debugging command for TMC stepper drivers.
   * M122 S0/1 will enable continous reporting.
   */
    //#define TMC_DEBUG

    /**
   * You can set your own advanced settings by filling in predefined functions.
   * A list of available functions can be found on the library github page
   * https://github.com/teemuatlut/TMC2130Stepper
   * https://github.com/teemuatlut/TMC2208Stepper
   *
   * Example:
   * #define TMC_ADV() { \
   *   stepperX.diag0_temp_prewarn(1); \
   *   stepperY.interpolate(0); \
   * }
   */
    #define TMC_ADV() \
        {}

#endif // HAS_TRINAMIC

// @section extras

/**
 * Auto-report temperatures with M155 S<seconds>
 */
#define AUTO_REPORT_TEMPERATURES

/**
 * Disable all Volumetric extrusion options
 */
//#define NO_VOLUMETRICS

#if DISABLED(NO_VOLUMETRICS)
/**
   * Volumetric extrusion default state
   * Activate to make volumetric extrusion the default method,
   * with DEFAULT_NOMINAL_FILAMENT_DIA as the default diameter.
   *
   * M200 D0 to disable, M200 Dn to set a new diameter.
   */
//#define VOLUMETRIC_DEFAULT_ON
#endif

/**
 * Enable this option for a leaner build of Marlin that removes all
 * workspace offsets, simplifying coordinate transformations, leveling, etc.
 *
 *  - M206 and M428 are disabled.
 *  - G92 will revert to its behavior from Marlin 1.0.
 */
//#define NO_WORKSPACE_OFFSETS

/**
 * Set the number of proportional font spaces required to fill up a typical character space.
 * This can help to better align the output of commands like `G29 O` Mesh Output.
 *
 * For clients that use a fixed-width font (like OctoPrint), leave this set to 1.0.
 * Otherwise, adjust according to your client and font.
 */
#define PROPORTIONAL_FONT_RATIO 1.0

/**
 * Spend 28 bytes of SRAM to optimize the GCode parser
 */
#define FASTER_GCODE_PARSER

/**
 * CNC G-code options
 * Support CNC-style G-code dialects used by laser cutters, drawing machine cams, etc.
 * Note that G0 feedrates should be used with care for 3D printing (if used at all).
 * High feedrates may cause ringing and harm print quality.
 */
//#define PAREN_COMMENTS      // Support for parentheses-delimited comments
//#define GCODE_MOTION_MODES  // Remember the motion mode (G0 G1 G2 G3 G5 G38.X) and apply for X Y Z E F, etc.

// Enable and set a (default) feedrate for all G0 moves
//#define G0_FEEDRATE 3000 // (mm/m)
#ifdef G0_FEEDRATE
//#define VARIABLE_G0_FEEDRATE // The G0 feedrate is set by F in G0 motion mode
#endif

/**
 * Host Action Commands
 *
 * Define host streamer action commands in compliance with the standard.
 *
 * See https://reprap.org/wiki/G-code#Action_commands
 * Common commands ........ poweroff, pause, paused, resume, resumed, cancel
 *
 * Some features add reason codes to extend these commands.
 *
 * Host Prompt Support enables Marlin to use the host for user prompts so
 * filament runout and other processes can be managed from the host side.
 */
//#define HOST_ACTION_COMMANDS
#if ENABLED(HOST_ACTION_COMMANDS)
    #define HOST_PROMPT_SUPPORT
#endif

/**
 * NanoDLP Sync support
 *
 * Add support for Synchronized Z moves when using with NanoDLP. G0/G1 axis moves will output "Z_move_comp"
 * string to enable synchronization with DLP projector exposure. This change will allow to use
 * [[WaitForDoneMessage]] instead of populating your gcode with M400 commands
 */
//#define NANODLP_Z_SYNC
#if ENABLED(NANODLP_Z_SYNC)
//#define NANODLP_ALL_AXIS  // Enables "Z_move_comp" output on any axis move.
// Default behaviour is limited to Z axis only.
#endif

/**
 * WiFi Support (Espressif ESP32 WiFi)
 */
//#define WIFISUPPORT
#if ENABLED(WIFISUPPORT)
    #define WIFI_SSID "Wifi SSID"
    #define WIFI_PWD "Wifi Password"
//#define WEBSUPPORT        // Start a webserver with auto-discovery
//#define OTASUPPORT        // Support over-the-air firmware updates
#endif

// Prusa M73 implementation
#define M73_PRUSA

// Enable SDCARD gcodes M20 - M30
#define SDCARD_GCODES

// @section develop

// Enable Marlin dev mode which adds some special commands
//#define MARLIN_DEV_MODE

/**
 * M862.x support for print checking Q commands (P are always supported)
 */
#define PRINT_CHECKING_Q_CMDS
