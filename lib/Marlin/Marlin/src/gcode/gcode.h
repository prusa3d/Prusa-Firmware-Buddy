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
#include <option/has_pause.h>
#include <option/has_tool_offset_pin_calibration.h>

/**
 * gcode.h - Temporary container for all gcode handlers
 */

/**
 * -----------------
 * G-Codes in Marlin
 * -----------------
 *
 * Helpful G-code references:
 *  - http://marlinfw.org/meta/gcode
 *  - https://reprap.org/wiki/G-code
 *  - http://linuxcnc.org/docs/html/gcode.html
 *
 * Help to document Marlin's G-codes online:
 *  - https://github.com/MarlinFirmware/MarlinDocumentation
 *
 * -----------------
 *
 * "G" Codes
 *
 * G0   -> G1
 * G1   - Coordinated Movement X Y Z E
 * G2   - CW ARC
 * G3   - CCW ARC
 * G4   - Dwell S<seconds> or P<milliseconds>
 * G5   - Cubic B-spline with XYZE destination and IJPQ offsets
 * G17  - Select Plane XY (Requires CNC_WORKSPACE_PLANES)
 * G18  - Select Plane ZX (Requires CNC_WORKSPACE_PLANES)
 * G19  - Select Plane YZ (Requires CNC_WORKSPACE_PLANES)
 * G27  - Park Nozzle
 * G28  - Home one or more axes
 * G29  - Start or continue the bed leveling probe procedure (Requires bed leveling)
 * G30  - Single Z probe, probes bed at X Y location (defaults to current XY location)
 * G34  - Z Stepper automatic alignment using probe: I<iterations> T<accuracy> A<amplification> (Requires Z_STEPPER_AUTO_ALIGN)
 * G38  - Probe in any direction using the Z_MIN_PROBE (Requires G38_PROBE_TARGET)
 * G42  - Coordinated move to a mesh point (Requires AUTO_BED_LEVELING_BLINEAR or AUTO_BED_LEVELING_UBL)
 * G80  - Cancel current motion mode (Requires GCODE_MOTION_MODES)
 * G90  - Use Absolute Coordinates
 * G91  - Use Relative Coordinates
 * G92  - Set current position to coordinates given
 *
 * "M" Codes
 *
 * M0   - Unconditional stop - Wait for user to press a button on the LCD.
 * M1   -> M0
 * M16  - Expected printer check. (Requires EXPECTED_PRINTER_CHECK)
 * M17  - Enable/Power all stepper motors
 * M18  - Disable all stepper motors; same as M84
 * M20  - List SD card. (Requires SDSUPPORT)
 * M21  - Init SD card. (Requires SDSUPPORT)
 * M22  - Release SD card. (Requires SDSUPPORT)
 * M23  - Select SD file: "M23 /path/file.gco". (Requires SDSUPPORT)
 * M24  - Start/resume SD print. (Requires SDSUPPORT)
 * M25  - Pause SD print. (Requires SDSUPPORT)
 * M26  - Set SD position in bytes: "M26 S12345". (Requires SDSUPPORT)
 * M27  - Report SD print status. (Requires SDSUPPORT)
 *        OR, with 'S<seconds>' set the SD status auto-report interval. (Requires AUTO_REPORT_SD_STATUS)
 *        OR, with 'C' get the current filename.
 * M28  - Start SD write: "M28 /path/file.gco". (Requires SDSUPPORT)
 * M29  - Stop SD write. (Requires SDSUPPORT)
 * M30  - Delete file from SD: "M30 /path/file.gco"
 * M31  - Report time since last M109 or SD card start to serial.
 * M32  - Select file and start SD print: "M32 [S<bytepos>] !/path/file.gco#". (Requires SDSUPPORT)
 *        Use P to run other files as sub-programs: "M32 P !filename#"
 *        The '#' is necessary when calling from within sd files, as it stops buffer prereading
 * M33  - Get the longname version of a path. (Requires LONG_FILENAME_HOST_SUPPORT)
 * M34  - Set SD Card sorting options. (Requires SDCARD_SORT_ALPHA)
 * M48  - Measure Z Probe repeatability: M48 P<points> X<pos> Y<pos> V<level> E<engage> L<legs> S<chizoid>. (Requires Z_MIN_PROBE_REPEATABILITY_TEST)
 * M73  - Set the progress percentage.
 * M75  - Start the print job timer.
 * M76  - Pause the print job timer.
 * M77  - Stop the print job timer.
 * M80  - Turn on Power Supply. (Requires PSU_CONTROL)
 * M81  - Turn off Power Supply. (Requires PSU_CONTROL)
 * M82  - Set E codes absolute (default).
 * M83  - Set E codes relative while in Absolute (G90) mode.
 * M84  - Disable steppers until next move, or use S<seconds> to specify an idle
 *        duration after which steppers should turn off. S0 disables the timeout.
 * M86  - Set Safety Timer expiration time (S<seconds>). Set to zero to disable the timer.
 * M92  - Set planner.settings.axis_steps_per_mm for one or more axes.
 * M104 - Set extruder target temp.
 * M105 - Report current temperatures.
 * M106 - Set print fan speed.
 * M107 - Print fan off.
 * M108 - Break out of heating loops (M109, M190, M303). With no controller, breaks out of M0/M1. (Requires EMERGENCY_PARSER)
 * M109 - S<temp> Wait for extruder current temp to reach target temp. ** Wait only when heating! **
 *        R<temp> Wait for extruder current temp to reach target temp. ** Wait for heating or cooling. **
 * M110 - Set the current line number. (Used by host printing)
 * M111 - Set debug flags: "M111 S<flagbits>". See flag bits defined in enum.h.
 * M112 - Full Shutdown.
 * M113 - Get or set the timeout interval for Host Keepalive "busy" messages. (Requires HOST_KEEPALIVE_FEATURE)
 * M114 - Report current position.
 * M115 - Report capabilities.
 * M117 - Display a message on the controller screen. (Requires an LCD)
 * M118 - Display a message in the host console.
 * M119 - Report endstops status.
 * M120 - Enable endstops detection.
 * M121 - Disable endstops detection.
 * M122 - Debug stepper (Requires at least one _DRIVER_TYPE defined as TMC2130/2160/5130/5160/2208/2209/2660)
 * M140 - Set bed target temp. S<temp>
 * M150 - Set Status LED Color as R<red> U<green> B<blue> P<bright>. Values 0-255.
 * M155 - Auto-report temperatures with interval of S<seconds>. (Requires AUTO_REPORT_TEMPERATURES)
 * M190 - S<temp> Wait for bed current temp to reach target temp. ** Wait only when heating! **
 *        R<temp> Wait for bed current temp to reach target temp. ** Wait for heating or cooling. **
 * M200 - Set filament diameter, D<diameter>, setting E axis units to cubic. (Use S0 to revert to linear units.)
 * M201 - Set max acceleration in units/s^2 for print moves: "M201 X<accel> Y<accel> Z<accel> E<accel>"
 * M202 - Set max acceleration in units/s^2 for travel moves: "M202 X<accel> Y<accel> Z<accel> E<accel>" ** UNUSED IN MARLIN! **
 * M203 - Set maximum feedrate: "M203 X<fr> Y<fr> Z<fr> E<fr>" in units/sec.
 * M204 - Set default acceleration in units/sec^2: P<printing> R<extruder_only> T<travel>
 * M205 - Set advanced settings. Current units apply:
            S<print> T<travel> minimum speeds
            B<minimum segment time>
            X<max X jerk>, Y<max Y jerk>, Z<max Z jerk>, E<max E jerk>
 * M206 - Set additional homing offset. (Disabled by NO_WORKSPACE_OFFSETS)
 * M211 - Enable, Disable, and/or Report software endstops: S<0|1> (Requires MIN_SOFTWARE_ENDSTOPS or MAX_SOFTWARE_ENDSTOPS)
 * M217 - Set filament swap parameters: "M217 S<length> P<feedrate> R<feedrate>". (Requires SINGLENOZZLE)
 * M218 - Set/get a tool offset: "M218 T<index> X<offset> Y<offset>". (Requires 2 or more extruders)
 * M220 - Set Feedrate Percentage: "M220 S<percent>" (i.e., "FR" on the LCD)
 * M221 - Set Flow Percentage: "M221 S<percent>"
 * M260 - i2c Send Data
 * M261 - i2c Request Data
 * M290 - Babystepping (Requires BABYSTEPPING)
 * M300 - Play beep sound S<frequency Hz> P<duration ms>
 * M301 - Set PID parameters P I and D. (Requires PIDTEMP)
 * M302 - Allow cold extrudes, or set the minimum extrude S<temperature>. (Requires PREVENT_COLD_EXTRUSION)
 * M303 - PID relay autotune S<temperature> sets the target temperature. Default 150C. (Requires PIDTEMP)
 * M304 - Set bed PID parameters P I and D. (Requires PIDTEMPBED)
 * M350 - Set microstepping mode. (Requires digital microstepping pins.)
 * M351 - Toggle MS1 MS2 pins directly. (Requires digital microstepping pins.)
 * M355 - Set Case Light on/off and set brightness. (Requires CASE_LIGHT_PIN)
 * M400 - Finish all moves.
 * M401 - Deploy and activate Z probe. (Requires a probe)
 * M402 - Deactivate and stow Z probe. (Requires a probe)
 * M403 - Set filament type for PRUSA MMU2
 * M410 - Quickstop. Abort all planned moves.
 * M420 - Enable/Disable Leveling (with current values) S1=enable S0=disable (Requires ABL)
 * M421 - Set a single Z coordinate in the Mesh Leveling grid. X<units> Y<units> Z<units> (Requires AUTO_BED_LEVELING_UBL)
 * M422 - Set Z Stepper automatic alignment position using probe. X<units> Y<units> A<axis> (Requires Z_STEPPER_AUTO_ALIGN)
 * M425 - Enable/Disable and tune backlash correction. (Requires BACKLASH_COMPENSATION and BACKLASH_GCODE)
 * M428 - Set the home_offset based on the current_position. Nearest edge applies. (Disabled by NO_WORKSPACE_OFFSETS)
 * M500 - Store parameters in EEPROM. (Requires EEPROM_SETTINGS)
 * M501 - Restore parameters from EEPROM. (Requires EEPROM_SETTINGS)
 * M502 - Revert to the default "factory settings". ** Does not write them to EEPROM! **
 * M503 - Print the current settings (in memory): "M503 S<verbose>". S0 specifies compact output.
 * M504 - Validate EEPROM contents. (Requires EEPROM_SETTINGS)
 * M540 - Enable/disable SD card abort on endstop hit: "M540 S<state>". (Requires SD_ABORT_ON_ENDSTOP_HIT)
 * M569 - Enable stealthChop on an axis. (Requires at least one _DRIVER_TYPE to be TMC2130/2160/2208/2209/5130/5160)
 * M572 - Set parameters for pressure advance.
 * M593 - Set parameters for input shapers.
 * M600 - Pause for filament change: "M600 X<pos> Y<pos> Z<raise> E<first_retract> L<later_retract>".
 * M601 - Pause and park print-head in marlin's defined position.
 * M602 - Unpark print-head parked with M601 called before and unpause print process.
 * M603 - Configure filament change: "M603 T<tool> U<unload_length> L<load_length>".
 * M604 - Abort (serial) print
 * M666 - Set/get offsets for triple endstops (Z_TRIPLE_ENDSTOPS).
 * M701 - Load filament
 * M702 - Unload filament
 * M851 - Set Z probe's XYZ offsets in current units. (Negative values: X=left, Y=front, Z=below)
 * M876 - Handle Prompt Response. (Requires HOST_PROMPT_SUPPORT and not EMERGENCY_PARSER)
 * M900 - Get or Set Linear Advance K-factor. (PA Compatibility with old LIN_ADVANCE)
 * M906 - Set or get motor current in milliamps using axis codes X, Y, Z, E. Report values if no axis codes given. P selects a motor current profile (HAS_MOTOR_CURRENT_PROFILES). (Requires at least one _DRIVER_TYPE defined as TMC2130/2160/5130/5160/2208/2209/2660)
 * M911 - Report stepper driver overtemperature pre-warn condition. (Requires at least one _DRIVER_TYPE defined as TMC2130/2160/5130/5160/2208/2209/2660)
 * M912 - Clear stepper driver overtemperature pre-warn condition flag. (Requires at least one _DRIVER_TYPE defined as TMC2130/2160/5130/5160/2208/2209/2660)
 * M913 - Set HYBRID_THRESHOLD speed. (Requires HYBRID_THRESHOLD)
 * M914 - Set StallGuard sensitivity. (Requires SENSORLESS_HOMING or SENSORLESS_PROBING)
 * M7219 - Control Max7219 Matrix LEDs. (Requires MAX7219_GCODE)
 *
 * ************ Custom codes - This can change to suit future G-code regulations
 * G425 - Calibrate using a conductive object. (Requires HAS_TOOL_OFFSET_PIN_CALIBRATION)
 * G426 - Measure tool offset contactlessly. (Requires HAS_TOOL_OFFSET_SENSOR)
 * M958 - Excite harmonic vibration and measure amplitude
 * M959 - Tune input shaper
 * M970 - Set/enable phase stepping
 * M971 - Read/reset/write phase-stepping motor current correction
 * M972 - Calibrate motor for phase stepping
 * M973 - Perform phase and magnitude correction sweep
 * M974 - Perform motor resonance measurement during a speed sweep
 * M997 - Perform in-application firmware update
 * M999 - Restart after being stopped by error
 *
 * "T" Codes
 *
 * T0-T3 - Select an extruder (tool) by index: "T<n> F<units/min>"
 *
 */

#include "../inc/MarlinConfig.h"
#include "parser.h"

#include <tool_index.hpp>
#include <common/printer_model.hpp>

#include <option/has_i2c_expander.h>
#include <option/has_local_accelerometer.h>
#include <option/has_modular_bed.h>
#include <option/has_print_sheet_detection.h>
#include <option/has_remote_accelerometer.h>
#include <option/has_precise_homing_corexy.h>
#include <option/has_precise_homing.h>
#include <option/has_phase_stepping.h>
#include <option/has_phase_stepping_calibration.h>
#include <option/has_gcode_compatibility.h>
#include <option/has_cancel_object.h>
#include <option/has_tool_offset_sensor.h>

#if ANY(POLAR) || defined(G0_FEEDRATE)
  // #error dead code found by automatic analyses (see BFW-5461)
  #define HAS_FAST_MOVES 1
#endif

enum AxisRelative : uint8_t { REL_X, REL_Y, REL_Z, REL_E, E_MODE_ABS, E_MODE_REL };

struct G28Flags {
  bool only_if_needed = false;
  float z_raise = NAN;
  bool no_change = false;
  bool can_calibrate = false;
  bool force_calibrate = false;
  #if ENABLED(MARLIN_DEV_MODE)
    // #error dead code found by automatic analyses (see BFW-5461)
    bool simulate = false;
  #endif

  /// Require the homing to be precise (if false, only guarantees 'imprecise' homing, which might be faster, but is not suitable for printing)
  /// Relates to HAS_PRECISE_HOMING_COREXY
  bool precise = true;

  #if HAS_PRINT_SHEET_DETECTION()
    bool check_sheet = false;
  #endif

  /// If set to false, homing_failed() function will not be called on homing failure
  // This means that the G28 will return false instead of a crash/redscreen
  bool throw_homing_failed = true;
};

class GcodeSuite {
public:

  static uint8_t axis_relative;

  static inline bool axis_is_relative(const AxisEnum a) {
    if (a == E_AXIS) {
      if (TEST(axis_relative, E_MODE_REL)) return true;
      if (TEST(axis_relative, E_MODE_ABS)) return false;
    }
    return TEST(axis_relative, a);
  }
  static inline void set_relative_mode(const bool rel) {
    #if HAS_GCODE_COMPATIBILITY()
        if (compatibility.mk3_compatibility_mode) {
            axis_relative = rel ? _BV(REL_X) | _BV(REL_Y) | _BV(REL_Z) : 0;
        } else {
            axis_relative = rel ? _BV(REL_X) | _BV(REL_Y) | _BV(REL_Z) | _BV(REL_E) : 0;
        }
    #else
        axis_relative = rel ? _BV(REL_X) | _BV(REL_Y) | _BV(REL_Z) | _BV(REL_E) : 0;
    #endif
  }
  static inline void set_e_relative() {
    CBI(axis_relative, E_MODE_ABS);
    SBI(axis_relative, E_MODE_REL);
  }
  static inline void set_e_absolute() {
    CBI(axis_relative, E_MODE_REL);
    SBI(axis_relative, E_MODE_ABS);
  }

  static PrinterGCodeCompatibilityReport compatibility;

  #if ENABLED(CNC_WORKSPACE_PLANES)
    // #error dead code found by automatic analyses (see BFW-5461)
    /**
     * Workspace planes only apply to G2/G3 moves
     * (and "canned cycles" - not a current feature)
     */
    enum WorkspacePlane : char { PLANE_XY, PLANE_ZX, PLANE_YZ };
    static WorkspacePlane workspace_plane;
  #endif

  struct ToolParsingError {
    const char* msg;
  };

  using VirtualToolFromCommand = std::variant<VirtualToolIndex, NoTool, ToolNotMapped, ToolParsingError>;
  using PhysicalToolFromCommand = std::variant<PhysicalToolIndex, NoTool, ToolNotMapped, ToolParsingError>;

  /// Parses tool_index from command.
  /// @param tool_mapping if true, the tool_index is considered GCodeToolIndex and mapping is applied, else it is considered a VirtualToolIndex
  /// Shows a RSOD if the input is invalid/tool cannot be mapped
  static VirtualToolFromCommand get_virtual_tool_from_command(uint8_t tool_index, bool tool_mapping);

  /// Validates that the option value is valid and may pass it through tool mapping
  static VirtualToolFromCommand get_target_virtual_from_optional(std::optional<uint8_t> option_value, const bool tool_map);

  static VirtualToolFromCommand get_target_virtual_from_command();
  static VirtualToolFromCommand get_target_virtual_from_command_p();

  /// Like get_target_virtual_from_optional, but returns a PhysicalToolIndex.
  /// If the parameter is present, parses as virtual and converts to physical.
  /// If the parameter is absent, defaults to PhysicalToolIndex::currently_selected().
  static PhysicalToolFromCommand get_target_physical_from_optional(std::optional<uint8_t> option_value, bool tool_map);

  static PhysicalToolFromCommand get_target_physical_from_command();
  static PhysicalToolFromCommand get_target_physical_from_command_p();

  static int8_t get_target_e_stepper_from_command();
  static void get_destination_from_command();

  static void process_parsed_command(const bool no_ok=false);
  static void process_parsed_command_standard();
  static void process_next_command();

  #if ENABLED(PROCESS_CUSTOM_GCODE)
    static bool process_parsed_command_custom(const bool no_ok=false);
  #endif

  // Execute G-code in-place, preserving current G-code parameters
  static void process_subcommands_now_P(PGM_P pgcode);
  static void process_subcommands_now(char * gcode);

  #if ENABLED(HOST_KEEPALIVE_FEATURE)
    /**
     * States for managing Marlin and host communication
     * Marlin sends messages if blocked or busy
     */
    enum MarlinBusyState : char {
      NOT_BUSY,           // Not in a handler
      IN_HANDLER,         // Processing a GCode
      IN_PROCESS,         // Known to be blocking command input (as in G29)
      PAUSED_FOR_USER,    // Blocking pending any input
      PAUSED_FOR_INPUT    // Blocking pending text input (concept)
    };

    static MarlinBusyState busy_state;
    static uint8_t host_keepalive_interval;

    static void host_keepalive();

    #define KEEPALIVE_STATE(N) REMEMBER(_KA_, gcode.busy_state, gcode.N)
  #else
    #define KEEPALIVE_STATE(N) NOOP
  #endif

  // Dwell waits immediately with low precision (+10ms depending on GUI/background activities)
  // while allowing background processing. It does not synchronize.
  static void dwell(millis_t time);

  static void M104();

  /**
   * @brief Home.
   * @param flags @see G28Flags
   * @return true on success
   */
  static bool G28_no_parser(bool X, bool Y, bool Z, const G28Flags& flags = G28Flags());

  static void T();

private:

  friend class MarlinSettings;
  #if ENABLED(ARC_SUPPORT)
    friend void plan_arc(const xyze_pos_t&, const xy_float_t&, const bool, const uint8_t, const bool);
  #endif

  static void G0_G1(TERN_(HAS_FAST_MOVES, const bool fast_move=false));

  #if ENABLED(ARC_SUPPORT)
    static void G2_G3(const bool clockwise);
  #endif

  static void G4();

  #if ENABLED(BEZIER_CURVE_SUPPORT)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void G5();
  #endif

  #if ENABLED(DIRECT_STEPPING)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void G6();
  #endif

  #if ENABLED(CNC_WORKSPACE_PLANES)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void G17();
    static void G18();
    static void G19();
  #endif

  static void G27();

  static void G28();

  #if HAS_LEVELING
    static void G29();
  #endif

  #if HAS_BED_PROBE
    static void G30();
  #endif

  #if ENABLED(Z_STEPPER_AUTO_ALIGN)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void G34();
    static void M422();
  #endif

  #if ENABLED(G38_PROBE_TARGET)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void G38(const int8_t subcode);
  #endif

  #if HAS_MESH
    static void G42();
  #endif

  #if ENABLED(GCODE_MOTION_MODES) || HAS_GCODE_COMPATIBILITY()
    static void G80();
  #endif

  static void G92();

  #if HAS_TOOL_OFFSET_PIN_CALIBRATION()
    static void G425();
  #endif

  #if HAS_TOOL_OFFSET_SENSOR()
    static void G426();
  #endif

  #if HAS_RESUME_CONTINUE
    static void M0_M1();
  #endif

  #if ENABLED(EXPECTED_PRINTER_CHECK)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M16();
  #endif

  static void M17();

  static void M18_M84();

  #if ENABLED(SDCARD_GCODES)
    static void M20();
    static void M21();
    static void M22();
    static void M23();
    static void M24();
    static void M25();
    static void M26();
    static void M27();
    static void M28();
    static void M29();
    static void M30();
  #endif

  static void M31();

  #if ENABLED(SDCARD_GCODES)
    static void M32();
    #if ENABLED(LONG_FILENAME_HOST_SUPPORT)
      // #error dead code found by automatic analyses (see BFW-5461)
      static void M33();
    #endif
    #if BOTH(SDCARD_SORT_ALPHA, SDSORT_GCODE)
      // #error dead code found by automatic analyses (see BFW-5461)
      static void M34();
    #endif
  #endif

  static void M46();

  #if ENABLED(Z_MIN_PROBE_REPEATABILITY_TEST)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M48();
  #endif

#if ENABLED(M73_PRUSA)
  static void M73_PE();
#endif

  static void M74();

  static void M75();
  static void M76();
  static void M77();

  #if HAS_POWER_SWITCH
    static void M80();
  #endif

  static void M81();
  static void M82();
  static void M83();
  static void M86();
  static void M92();
  static void M109();
  static void M105();
  static void M106();
  static void M107();

  #if DISABLED(EMERGENCY_PARSER)
    static void M108();
    static void M112();
    static void M410();
    #if ENABLED(HOST_PROMPT_SUPPORT)
      static void M876();
    #endif
  #endif

  static void M110();
  static void M111();

  #if ENABLED(HOST_KEEPALIVE_FEATURE)
    static void M113();
  #endif

  static void M114();
  static void M115();
  static void M117();
  static void M118();
  static void M119();
  static void M120();
  static void M121();

  #if HAS_HEATED_BED
    static void M140();
    static void M190();
  #endif

  #if HAS_TEMP_HEATBREAK_CONTROL
    static void M142();
  #endif

  #if ENABLED(AUTO_REPORT_TEMPERATURES) && HAS_TEMP_SENSOR
    static void M155();
  #endif

  static void M200();
  static void M201();

  #if 0
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M202(); // Not used for Sprinter/grbl gen6
  #endif

  static void M203();
  static void M204();
  static void M205();

  #if HAS_M206_COMMAND
    static void M206();
  #endif

  static void M211();

  #if EXTRUDERS > 1
    static void M217();
  #endif

  #if HAS_HOTEND_OFFSET
    static void M218();
  #endif

  static void M220();
  static void M221();

  #if HAS_I2C_EXPANDER()
    static void M260();
    static void M261();
  #endif

  #if ENABLED(BABYSTEPPING)
    static void M290();
  #endif

  #if ENABLED(PIDTEMP)
    static void M301();
  #endif

  #if ENABLED(PREVENT_COLD_EXTRUSION)
    static void M302();
  #endif

  #if HAS_PID_HEATING
    static void M303();
  #endif

  #if ENABLED(PIDTEMPBED)
    static void M304();
  #endif

  #if HAS_DRIVER(TMC2130)
    static void M350();
  #endif

  #if HAS_CASE_LIGHT
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M355();
  #endif

  static void M400();

  #if HAS_BED_PROBE
    static void M401();
    static void M402();
  #endif

  #if HAS_MMU2()
    static void M403();
  #endif

  #if HAS_LEVELING
    static void M420();
    static void M421();
  #endif

  #if ENABLED(BACKLASH_GCODE)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M425();
  #endif

  #if HAS_M206_COMMAND
    static void M428();
  #endif

  #if HAS_CANCEL_OBJECT()
    static void M486();
  #endif

  static void M500();
  static void M501();
  static void M502();
  #if DISABLED(DISABLE_M503)
    static void M503();
  #endif

  #if ENABLED(SD_ABORT_ON_ENDSTOP_HIT)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M540();
  #endif

  static void M555();

  #if HAS_MODULAR_BED()
    static void M556();
    static void M557();
  #endif

  static void M572();
  static void M572_internal(float pressure_advance, float smooth_time);

  #if ENABLED(BAUD_RATE_GCODE)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M575();
  #endif

  static void M593();

  #if HAS_PAUSE()
    static void M600();
    static void M601();
    static void M602();
    static void M603();
  #endif
  static void M604();

  #if ENABLED(Z_TRIPLE_ENDSTOPS)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M666();
  #endif

  static void M701();
  static void M702();

  #if HAS_BED_PROBE
    static void M851();
  #endif

  // Linear Advance / Pressure Advance compatibility
  static void M900();

  #if HAS_TRINAMIC
    static void M122();
    static void M906();
    #if HAS_STEALTHCHOP
      static void M569();
    #endif
    #if ENABLED(MONITOR_DRIVER_STATUS)
      static void M911();
      static void M912();
    #endif
    #if ENABLED(HYBRID_THRESHOLD)
      static void M913();
    #endif
    #if USE_SENSORLESS
      static void M914();
    #endif
  #endif

#if HAS_LOCAL_ACCELEROMETER() || HAS_REMOTE_ACCELEROMETER()
  static void M958();
  static void M959();
#endif

#if HAS_PHASE_STEPPING()
  static void M970();
  static void M971();
#endif
#if HAS_PHASE_STEPPING_CALIBRATION()
  static void M972();
  static void M973();
  static void M974();
#endif

  #if ENABLED(PLATFORM_M997_SUPPORT)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M997();
  #endif

  static void M999();

  #if ENABLED(MAX7219_GCODE)
    // #error dead code found by automatic analyses (see BFW-5461)
    static void M7219();
  #endif
};

extern GcodeSuite gcode;
