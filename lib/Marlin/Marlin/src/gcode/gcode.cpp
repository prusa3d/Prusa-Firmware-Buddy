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

/**
 * gcode.cpp - Temporary container for all gcode handlers
 *             Most will migrate to classes, by feature.
 */

#include "gcode.h"
GcodeSuite gcode;

#include "parser.h"
#include "queue.h"
#include "../module/motion.h"
#include "../module/planner.h"
#include <utils/variant_utils.hpp>

#if ENABLED(HOST_PROMPT_SUPPORT)
  #include "../feature/host_actions.h"
#endif

#include <option/has_cancel_object.h>
#include <option/has_crash_detection.h>
#include <option/has_tool_offset_pin_calibration.h>
#if HAS_CANCEL_OBJECT()
  #include <feature/cancel_object/cancel_object.hpp>
#endif

#if HAS_CRASH_DETECTION()
  #include "../feature/prusa/crash_recovery.hpp"
#endif

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
  #include "module/prusa/toolchanger.h"
#endif

#include "../Marlin.h" // for idle() and suspend_auto_report

#include "odometer.hpp"

#include <option/has_tool_mapping.h>
#if HAS_TOOL_MAPPING()
  #include "module/prusa/tool_mapper.hpp"
#endif

#include <option/has_mmu2.h>
#include <option/has_i2c_expander.h>
#include <option/has_local_accelerometer.h>
#include <option/has_modular_bed.h>
#include <option/has_remote_accelerometer.h>
#include <option/has_gcode_compatibility.h>
#include <option/has_pause.h>
#include <option/has_phase_stepping.h>
#include <option/has_phase_stepping_calibration.h>
#include <marlin_vars.hpp>
#include <utils/serial_logging_disabler.hpp>
#include <feature/safety_timer/safety_timer.hpp>

// Relative motion mode for each logical axis
static constexpr xyze_bool_t ar_init = AXIS_RELATIVE_MODES;
uint8_t GcodeSuite::axis_relative = (
    (ar_init.x ? _BV(REL_X) : 0)
  | (ar_init.y ? _BV(REL_Y) : 0)
  | (ar_init.z ? _BV(REL_Z) : 0)
  | (ar_init.e ? _BV(REL_E) : 0)
);

PrinterGCodeCompatibilityReport GcodeSuite::compatibility;

#if ENABLED(HOST_KEEPALIVE_FEATURE)
  GcodeSuite::MarlinBusyState GcodeSuite::busy_state = NOT_BUSY;
  uint8_t GcodeSuite::host_keepalive_interval = DEFAULT_KEEPALIVE_INTERVAL;
#endif

#if ENABLED(CNC_WORKSPACE_PLANES)
  // #error dead code found by automatic analyses (see BFW-5461)
  GcodeSuite::WorkspacePlane GcodeSuite::workspace_plane = PLANE_XY;
#endif

GcodeSuite::VirtualToolFromCommand GcodeSuite::get_virtual_tool_from_command(uint8_t tool_index, bool tool_mapping) {
  if(tool_mapping) {
    if (tool_index > GcodeToolIndex::count) {
      return ToolParsingError { .msg =  "Invalid tool index" };

    } else if (tool_index == GcodeToolIndex::count) {
      return NoTool{};
    }

    return stdext::to_variant(GcodeToolIndex::from_raw(tool_index).to_virtual());

  } else {
    if (tool_index > VirtualToolIndex::count) {
      return ToolParsingError { .msg = "Invalid tool index" };

    } else if (tool_index == VirtualToolIndex::count) {
      return NoTool{};

    } else {
      return VirtualToolIndex::from_raw(tool_index);
    }
  }
}

GcodeSuite::VirtualToolFromCommand GcodeSuite::get_target_virtual_from_optional(std::optional<uint8_t> extruder, const bool tool_map) {
  auto maybe_virtual = extruder.transform([&](uint8_t e){
    return get_virtual_tool_from_command(e, tool_map);
  }).value_or(stdext::to_variant(VirtualToolIndex::currently_selected()));

  const auto report_error = [&]() {
    SERIAL_ECHO_START();
    SERIAL_ECHOLNPAIR(" " MSG_INVALID_EXTRUDER " ", static_cast<std::optional<int>>(extruder).value_or(-1));
  };

  return match(maybe_virtual,
    [&](VirtualToolIndex vt) -> VirtualToolFromCommand {
      if (vt.is_enabled()) {
        return vt;
      } else {
        report_error();
        return ToolParsingError { .msg = "Tool is disabled" };
      }
    },
    [&](NoTool) -> VirtualToolFromCommand {
      report_error();
      return NoTool{};
    },
    [&](ToolNotMapped tnm) -> VirtualToolFromCommand {
      report_error();
      return tnm;
    },
    [&](ToolParsingError err) -> VirtualToolFromCommand {
      report_error();
      SERIAL_ECHOLNPAIR(" ", err.msg);
      return err;
    }
  );
}

/**
 * Get the target extruder from the T parameter or the currently selected tool.
 * Returns VirtualToolFromCommand variant (VirtualToolIndex, NoTool, ToolNotMapped, or ToolParsingError).
 */
GcodeSuite::VirtualToolFromCommand GcodeSuite::get_target_virtual_from_command() {
  return get_target_virtual_from_optional(parser.seenval('T') ? std::optional(parser.value_byte()) : std::nullopt, true);
}

/**
 * + specify if target extruder is logical or physical
 */
GcodeSuite::VirtualToolFromCommand GcodeSuite::get_target_virtual_from_command_p() {
  return get_target_virtual_from_optional(parser.seenval('T') ? std::optional(parser.value_byte()) : std::nullopt,
  parser.seen('P') ? !parser.value_bool() : true);
}

GcodeSuite::PhysicalToolFromCommand GcodeSuite::get_target_physical_from_optional(std::optional<uint8_t> extruder, const bool tool_map) {
  if (extruder.has_value()) {
    // Parameter provided: parse as virtual (with validation), convert to physical
    VirtualToolFromCommand virt = get_target_virtual_from_optional(extruder, tool_map);
    return match(virt,
      [](VirtualToolIndex vt) -> PhysicalToolFromCommand { return vt.to_physical(); },
      [](auto other) -> PhysicalToolFromCommand { return other; }
    );
  } else {
    // No parameter: use currently selected physical tool
    const std::optional<PhysicalToolIndex> current = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected());
    if (current.has_value()) {
      return *current;
    } else {
      SERIAL_ECHO_START();
      SERIAL_ECHOLNPAIR(" " MSG_INVALID_EXTRUDER " ", -1);
      return NoTool {};
    }
  }
}

GcodeSuite::PhysicalToolFromCommand GcodeSuite::get_target_physical_from_command() {
  return get_target_physical_from_optional(parser.seenval('T') ? std::optional(parser.value_byte()) : std::nullopt, true);
}

GcodeSuite::PhysicalToolFromCommand GcodeSuite::get_target_physical_from_command_p() {
  return get_target_physical_from_optional(parser.seenval('T') ? std::optional(parser.value_byte()) : std::nullopt,
  parser.seen('P') ? !parser.value_bool() : true);
}

/**
 * Get the target e stepper from the T parameter
 * Return -1 if the T parameter is out of range or unspecified
 */
int8_t GcodeSuite::get_target_e_stepper_from_command() {
  int8_t e = parser.intval('T', -1);
  #if HAS_TOOL_MAPPING()
    // map gcode tool to virtual tool if mapping is enabled
    const uint8_t mapped = tool_mapper.to_virtual(e);
    e = mapped == ToolMapper::NO_TOOL_MAPPED ? -1 : mapped;
  #endif

  if (WITHIN(e, 0, E_STEPPERS - 1)) return e;

  SERIAL_ECHO_START();
  SERIAL_CHAR('M'); SERIAL_ECHO(parser.codenum);
  if (e == -1)
    SERIAL_ECHOLNPGM(" " MSG_E_STEPPER_NOT_SPECIFIED);
  else
    SERIAL_ECHOLNPAIR(" " MSG_INVALID_E_STEPPER " ", int(e));
  return -1;
}

/**
 * Set XYZE destination and feedrate from the current GCode command
 *
 *  - Set destination from included axis codes
 *  - Set to current for missing axis codes
 *  - Set the feedrate, if included
 */
void GcodeSuite::get_destination_from_command() {
  if (parser.linearval('F') > 0) {
    feedrate_mm_s = parser.value_feedrate();
  }

#if HAS_CANCEL_OBJECT()
  if (buddy::cancel_object().is_current_object_cancelled()) {
    destination = current_position;
    return;
  }
#endif

  XYZEval<float, LogicalPosTag> logical;
  LOOP_XYZE(i) {
    if (parser.seenval(axis_codes[i])) {
      logical[i] = parser.value_axis_units((AxisEnum)i);
    } else {
      logical[i] = NAN;
    }
  }

  const xyze_pos_t native = logical.asNative();

  LOOP_XYZE(i) {
    if (std::isnan(logical[i])) {
      destination[i] = current_position[i];

    } else if (axis_is_relative(AxisEnum(i))) {
      destination[i] = current_position[i] + logical[i];

    } else {
      destination[i] = native[i];
    }
  }

  LOOP_XYZ(i) {
    Odometer_s::instance().add_axis(Odometer_s::axis_t(i), destination[i] - current_position[i]);
  }
  if(auto tool = stdext::get_optional<PhysicalToolIndex>(PhysicalToolIndex::currently_selected())) {
    Odometer_s::instance().add_extruded(*tool, destination.e - current_position.e);
  }
}

/**
 * Dwell waits immediately. It does not synchronize. Use M400 instead of G4
 */
void GcodeSuite::dwell(millis_t time) {
  time += millis();
  while (!planner.draining() && PENDING(millis(), time)) idle(true);
}

[[gnu::noinline]]
void GcodeSuite::process_parsed_command_standard() {
  switch (parser.command_letter) {
    case 'G': switch (parser.codenum) {

      case 0: case 1: G0_G1(                                      // G0: Fast Move, G1: Linear Move
                        #if defined(G0_FEEDRATE)
                          // #error dead code found by automatic analyses (see BFW-5461)
                          parser.codenum == 0
                        #endif
                      );
                      break;

      #if ENABLED(ARC_SUPPORT)
        case 2: case 3: G2_G3(parser.codenum == 2); break;        // G2: CW ARC, G3: CCW ARC
      #endif

      case 4: G4(); break;                                        // G4: Dwell

      #if ENABLED(BEZIER_CURVE_SUPPORT)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 5: G5(); break;                                      // G5: Cubic B_spline
      #endif

      #if ENABLED(CNC_WORKSPACE_PLANES)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 17: G17(); break;                                    // G17: Select Plane XY
        case 18: G18(); break;                                    // G18: Select Plane ZX
        case 19: G19(); break;                                    // G19: Select Plane YZ
      #endif

        case 21: NOOP; break;                                     // No error on unknown G21

        case 27: G27(); break;                                    // G27: Nozzle Park

      case 28: G28(); break;                                 // G28: Home all axes, one at a time

      #if HAS_LEVELING
        case 29: G29(); break;                                    // G29: Bed leveling calibration
      #endif // HAS_LEVELING

      #if HAS_BED_PROBE
        case 30: G30(); break;                                    // G30: Single Z probe
        // G31: dock the sled REMOVED
        // G32: undock the sled REMOVED
      #endif

      #if ENABLED(Z_STEPPER_AUTO_ALIGN)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 34: G34(); break;                                    // G34: Z Stepper automatic alignment using probe
      #endif

      #if ENABLED(G38_PROBE_TARGET)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 38:                                                  // G38.2, G38.3: Probe towards target
          if (WITHIN(parser.subcode, 2,
            #if ENABLED(G38_PROBE_AWAY)
              // #error dead code found by automatic analyses (see BFW-5461)
              5
            #else
              // #error dead code found by automatic analyses (see BFW-5461)
              3
            #endif
          )) G38(parser.subcode);                                 // G38.4, G38.5: Probe away from target
          break;
      #endif

      #if ENABLED(GCODE_MOTION_MODES) || HAS_GCODE_COMPATIBILITY()
        case 80: G80(); break;                                    // G80: Reset the current motion mode
      #endif

      case 90: set_relative_mode(false); break;                   // G90: Absolute Mode
      case 91: set_relative_mode(true);  break;                   // G91: Relative Mode

      case 92: G92(); break;                                      // G92: Set current axis position(s)

      #if HAS_MESH
        case 42: G42(); break;                                    // G42: Coordinated move to a mesh point
      #endif

      #if HAS_TOOL_OFFSET_PIN_CALIBRATION()
        case 425: G425(); break;                                  // G425: Perform calibration with calibration cube
      #endif

      #if HAS_TOOL_OFFSET_SENSOR()
        case 426: G426(); break;                                  // G426: Measure tool offset contactlessly
      #endif

      #if ENABLED(DEBUG_GCODE_PARSER)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 800: parser.debug(); break;                          // G800: GCode Parser Test for G
      #endif

      default: parser.unknown_command_error(); break;
    }
    break;

    case 'M': switch (parser.codenum) {
      #if HAS_RESUME_CONTINUE
        case 0:                                                   // M0: Unconditional stop - Wait for user button press on LCD
        case 1: M0_M1(); break;                                   // M1: Conditional stop - Wait for user button press on LCD
      #endif

      // M7, M8, M9: coolant related gcodes was REMOVED

      #if ENABLED(EXPECTED_PRINTER_CHECK)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 16: M16(); break;                                    // M16: Expected printer check
      #endif

      case 17: M17(); break;                                      // M17: Enable all stepper motors

      #if ENABLED(SDCARD_GCODES)
        case 20: M20(); break;                                    // M20: List SD card
        case 21: M21(); break;                                    // M21: Init SD card
        case 22: M22(); break;                                    // M22: Release SD card
        case 23: M23(); break;                                    // M23: Select file
        case 24: M24(); break;                                    // M24: Start SD print
        case 25: M25(); break;                                    // M25: Pause SD print
        case 26: M26(); break;                                    // M26: Set SD index
        case 27: M27(); break;                                    // M27: Get SD status
        case 28: M28(); break;                                    // M28: Start SD write
        case 29: M29(); break;                                    // M29: Stop SD write
        case 30: M30(); break;                                    // M30 <filename> Delete File
        case 32: M32(); break;                                    // M32: Select file and start SD print

        #if ENABLED(LONG_FILENAME_HOST_SUPPORT)
          // #error dead code found by automatic analyses (see BFW-5461)
          case 33: M33(); break;                                  // M33: Get the long full path to a file or folder
        #endif

        #if BOTH(SDCARD_SORT_ALPHA, SDSORT_GCODE)
          // #error dead code found by automatic analyses (see BFW-5461)
          case 34: M34(); break;                                  // M34: Set SD card sorting options
        #endif

      #endif // SDCARD_GCODES

      case 31: M31(); break;                                      // M31: Report time since the start of SD print or last M109
      case 46: M46(); break;                                      // M46: Report ip4 address

      #if ENABLED(Z_MIN_PROBE_REPEATABILITY_TEST)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 48: M48(); break;                                    // M48: Z probe repeatability test
      #endif

      #if ENABLED(M73_PRUSA)
        case 73: M73_PE(); break;                                 // M73 PrusaEdition
      #endif

      case 74: M74(); break;                                      // M74: Set mass

      case 75: M75(); break;                                      // M75: Start print timer
      case 76: M76(); break;                                      // M76: Pause print timer
      case 77: M77(); break;                                      // M77: Stop print timer

      case 104: M104(); break;                                    // M104: Set hot end temperature
      case 109: M109(); break;                                    // M109: Wait for hotend temperature to reach target

      case 105: M105(); return;                                   // M105: Report Temperatures (and say "ok")
      
      case 106: M106(); break;                                    // M106: Fan On
      case 107: M107(); break;                                    // M107: Fan Off

      case 110: M110(); break;                                    // M110: Set Current Line Number
      case 111: M111(); break;                                    // M111: Set debug level

      #if DISABLED(EMERGENCY_PARSER)
        case 108: M108(); break;                                  // M108: Cancel Waiting
        case 112: M112(); break;                                  // M112: Full Shutdown
        case 410: M410(); break;                                  // M410: Quickstop - Abort all the planned moves.
        #if ENABLED(HOST_PROMPT_SUPPORT)
          case 876: M876(); break;                                // M876: Handle Host prompt responses
        #endif
      #else
        // #error dead code found by automatic analyses (see BFW-5461)
        case 108: case 112: case 410:
        #if ENABLED(HOST_PROMPT_SUPPORT)
          // #error dead code found by automatic analyses (see BFW-5461)
          case 876:
        #endif
        break;
      #endif

      #if ENABLED(HOST_KEEPALIVE_FEATURE)
        case 113: M113(); break;                                  // M113: Set Host Keepalive interval
      #endif

      #if HAS_HEATED_BED
        case 140: M140(); break;                                  // M140: Set bed temperature
        case 190: M190(); break;                                  // M190: Wait for bed temperature to reach target
      #endif

      #if ENABLED(AUTO_REPORT_TEMPERATURES) && HAS_TEMP_SENSOR
        case 155: M155(); break;                                  // M155: Set temperature auto-report interval
      #endif

      #if HAS_POWER_SWITCH
        case 80: M80(); break;                                    // M80: Turn on Power Supply
      #endif
      case 81: M81(); break;                                      // M81: Turn off Power, including Power Supply, if possible

      case 82: M82(); break;                                      // M82: Set E axis normal mode (same as other axes)
      case 83: M83(); break;                                      // M83: Set E axis relative mode
      case 18: case 84: M18_M84(); break;                         // M18/M84: Disable Steppers / Set Timeout
      case 86: M86(); break;                                      // M86: Set Safety Timer expiration time
      case 92: M92(); break;                                      // M92: Set the steps-per-unit for one or more axes
      case 114: M114(); break;                                    // M114: Report current position
      case 115: M115(); break;                                    // M115: Report capabilities
      case 117: M117(); break;                                    // M117: Set LCD message text, if possible
      case 118: M118(); break;                                    // M118: Display a message in the host console
      case 119: M119(); break;                                    // M119: Report endstop states
      case 120: M120(); break;                                    // M120: Enable endstops
      case 121: M121(); break;                                    // M121: Disable endstops

      #if HAS_TEMP_HEATBREAK_CONTROL
        case 142: M142(); break;
      #endif

      #if DISABLED(NO_VOLUMETRICS)
        case 200: M200(); break;                                  // M200: Set filament diameter, E to cubic units
      #endif

      case 201: M201(); break;                                    // M201: Set max acceleration for print moves (units/s^2)

      #if 0
        // #error dead code found by automatic analyses (see BFW-5461)
        case 202: M202(); break;                                  // M202: Not used for Sprinter/grbl gen6
      #endif

      case 203: M203(); break;                                    // M203: Set max feedrate (units/sec)
      case 204: M204(); break;                                    // M204: Set acceleration
      case 205: M205(); break;                                    // M205: Set advanced settings

      #if HAS_M206_COMMAND
        case 206: M206(); break;                                  // M206: Set home offsets
      #endif

      #if HAS_SOFTWARE_ENDSTOPS
        case 211: M211(); break;                                  // M211: Enable, Disable, and/or Report software endstops
      #endif

      #if EXTRUDERS > 1
        case 217: M217(); break;                                  // M217: Set filament swap parameters
      #endif

      #if HAS_HOTEND_OFFSET
        case 218: M218(); break;                                  // M218: Set a tool offset
      #endif

      case 220: M220(); break;                                    // M220: Set Feedrate Percentage: S<percent> ("FR" on your LCD)

      case 221: M221(); break;                                    // M221: Set Flow Percentage

      #if ENABLED(BABYSTEPPING)
        case 290: M290(); break;                                  // M290: Babystepping
      #endif

      #if ENABLED(PIDTEMP)
        case 301: M301(); break;                                  // M301: Set hotend PID parameters
      #endif

      #if ENABLED(PIDTEMPBED)
        case 304: M304(); break;                                  // M304: Set bed PID parameters
      #endif

      #if HAS_I2C_EXPANDER()
        case 260: M260(); break;                                  // M260: Send data to an i2c slave
        case 261: M261(); break;                                  // M261: Request data from an i2c slave
      #endif

      #if ENABLED(PREVENT_COLD_EXTRUSION)
        case 302: M302(); break;                                  // M302: Allow cold extrudes (set the minimum extrude temperature)
      #endif

      #if HAS_PID_HEATING
        case 303: M303(); break;                                  // M303: PID autotune
      #endif

      // M380: Activate solenoid REMOVED
      // M381: Disable all solenoids REMOVED

      case 400: M400(); break;                                    // M400: Finish all moves

      #if HAS_BED_PROBE
        case 401: M401(); break;                                  // M401: Deploy probe
        case 402: M402(); break;                                  // M402: Stow probe
      #endif

      #if HAS_MMU2()
        case 403: M403(); break;
      #endif

      #if HAS_LEVELING
        case 420: M420(); break;                                  // M420: Enable/Disable Bed Leveling
      #endif

      #if HAS_MESH
        case 421: M421(); break;                                  // M421: Set a Mesh Bed Leveling Z coordinate
      #endif

      #if ENABLED(BACKLASH_GCODE)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 425: M425(); break;                                  // M425: Tune backlash compensation
      #endif

      #if HAS_M206_COMMAND
        case 428: M428(); break;                                  // M428: Apply current_position to home_offset
      #endif

      case 500: M500(); break;                                    // M500: Store settings in EEPROM
      case 501: M501(); break;                                    // M501: Read settings from EEPROM
      case 502: M502(); break;                                    // M502: Revert to default settings
      #if DISABLED(DISABLE_M503)
        case 503: M503(); break;                                  // M503: print settings currently in memory
      #endif

      #if ENABLED(SD_ABORT_ON_ENDSTOP_HIT)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 540: M540(); break;                                  // M540: Set abort on endstop hit for SD printing
      #endif

      case 555: M555(); break;                                    // M555: Set print area

      #if HAS_MODULAR_BED()
        case 556: M556(); break;                                  // M556: Override modular bedled active
        case 557: M557(); break;                                  // M557: Set modular bed gradient parameters
      #endif

      case 572: M572(); break;                                    // M572: Set parameters for pressure advance.

      #if ENABLED(BAUD_RATE_GCODE)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 575: M575(); break;                                  // M575: Set serial baudrate
      #endif

      case 593: M593(); break;                                    // M593: Set parameters for input shapers.

      #if HAS_BED_PROBE
        case 851: M851(); break;                                  // M851: Set Z Probe Z Offset
      #endif

      // M852: Set Skew factors REMOVED

      #if HAS_PAUSE()
        case 600: M600(); break;                                  // M600: Pause for Filament Change
        case 601: M601(); break;                                  // M601: Pause & park
        case 602: M602(); break;                                  // M602: Unpark & UnPause print
        case 603: M603(); break;                                  // M603: Configure Filament Change
      #endif

      case 604: M604(); break;                                    // M604: Abort (serial) print

      case 701: M701(); break;                                    // M701: Load Filament
      case 702: M702(); break;                                    // M702: Unload Filament

      #if ENABLED(MAX7219_GCODE)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 7219: M7219(); break;                                // M7219: Set LEDs, columns, and rows
      #endif

      // Linear Advance / Pressure Advance compatibility
      case 900: M900(); break;                                    // M900: Set advance K factor.

      #if HAS_TRINAMIC
        case 122: M122(); break;                                  // M122: Report driver configuration and status
        case 906: M906(); break;                                  // M906: Set motor current in milliamps using axis codes X, Y, Z, E
        #if HAS_STEALTHCHOP
          case 569: M569(); break;                                // M569: Enable stealthChop on an axis.
        #endif
        #if ENABLED(MONITOR_DRIVER_STATUS)
          case 911: M911(); break;                                // M911: Report TMC2130 prewarn triggered flags
          case 912: M912(); break;                                // M912: Clear TMC2130 prewarn triggered flags
        #endif
        #if ENABLED(HYBRID_THRESHOLD)
          case 913: M913(); break;                                // M913: Set HYBRID_THRESHOLD speed.
        #endif
        #if USE_SENSORLESS
          case 914: M914(); break;                                // M914: Set StallGuard sensitivity.
        #endif
      #endif

      #if HAS_DRIVER(TMC2130)
        case 350: M350(); break;                                  // M350: Set microstepping mode. Warning: Steps per unit remains unchanged. S code sets stepping mode for all drivers.
      #endif
      #if HAS_CASE_LIGHT
        // #error dead code found by automatic analyses (see BFW-5461)
        case 355: M355(); break;                                  // M355: Set case light brightness
      #endif

      #if HAS_CANCEL_OBJECT()
        case 486: M486(); break;                                  // M486: Identify and cancel objects
      #endif

      #if ENABLED(DEBUG_GCODE_PARSER)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 800: parser.debug(); break;                          // M800: GCode Parser Test for M
      #endif

      #if HAS_LOCAL_ACCELEROMETER() || HAS_REMOTE_ACCELEROMETER()
        case 958: M958(); break;
        case 959: M959(); break;
      #endif

      #if HAS_PHASE_STEPPING()
        case 970: M970(); break;
        case 971: M971(); break;
      #endif
      #if HAS_PHASE_STEPPING_CALIBRATION()
        case 972: M972(); break;
        case 973: M973(); break;
        case 974: M974(); break;
      #endif

      #if ENABLED(Z_STEPPER_AUTO_ALIGN)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 422: M422(); break;                                  // M422: Set Z Stepper automatic alignment position using probe
      #endif

      #if ENABLED(PLATFORM_M997_SUPPORT)
        // #error dead code found by automatic analyses (see BFW-5461)
        case 997: M997(); break;                                  // M997: Perform in-application firmware update
      #endif

      case 999: M999(); break;                                    // M999: Restart after being Stopped

      default: parser.unknown_command_error(); break;
    }
    break;

    #if EXTRUDERS > 1
      case 'T': T(); break;                           // Tn: Tool Change
    #endif

    default:
      parser.unknown_command_error();
  }
}

/**
 * Process the parsed command and dispatch it to its handler
 */
void GcodeSuite::process_parsed_command(const bool no_ok/*=false*/) {
  KEEPALIVE_STATE(IN_HANDLER);

  #if HAS_CRASH_DETECTION()
    // this is done one step down from process_next_command in order to handle subcommands
    // and injected commands correctly: the state needs to reset at each logical move
    crash_s.start_new_gcode(queue.get_current_sdpos());
  #endif

  #if ENABLED(PROCESS_CUSTOM_GCODE)
    if (process_parsed_command_custom(/*no_ok=*/no_ok))
      return;
  #endif

  // Handle a known G, M, or T
  process_parsed_command_standard();

  if (!no_ok) queue.ok_to_send();
}

/**
 * Process a single command and dispatch it to its handler
 * This is called from the main loop()
 */
void GcodeSuite::process_next_command() {
  // We're doing something, don't go to sleep
  buddy::safety_timer().reset_norestore();

  char * const current_command = queue.command_buffer[queue.index_r];

  PORT_REDIRECT(queue.port[queue.index_r]);

  if (DEBUGGING(ECHO)) {
    SERIAL_ECHO_START();
    SERIAL_ECHOLN(current_command);
  }

  // Parse the next command in the queue
  parser.parse(current_command);

  marlin_vars().gcode_command = marlin_server::Cmd(parser.command_letter << 16 | parser.codenum);
  process_parsed_command();
  marlin_vars().gcode_command = marlin_server::Cmd();

  // The gcode might have taken a long time, again mark that we're doing something
  buddy::safety_timer().reset_norestore();
}

/**
 * Run a series of commands, bypassing the command queue to allow
 * G-code "macros" to be called from within other G-code handlers.
 */

void GcodeSuite::process_subcommands_now_P(PGM_P pgcode) {
  char * const saved_cmd = parser.command_ptr;        // Save the parser state
  for (;;) {
    PGM_P const delim = strchr_P(pgcode, '\n');       // Get address of next newline
    const size_t len = delim ? delim - pgcode : strlen_P(pgcode); // Get the command length
    char cmd[len + 1];                                // Allocate a stack buffer
    strncpy_P(cmd, pgcode, len);                      // Copy the command to the stack
    cmd[len] = '\0';                                  // End with a nul
    parser.parse(cmd);                                // Parse the command
    process_parsed_command(true);                     // Process it
    if (!delim) break;                                // Last command?
    pgcode = delim + 1;                               // Get the next command
  }
  parser.parse(saved_cmd);                            // Restore the parser state
}

void GcodeSuite::process_subcommands_now(char * gcode) {
  char * const saved_cmd = parser.command_ptr;        // Save the parser state
  for (;;) {
    char * const delim = strchr(gcode, '\n');         // Get address of next newline
    if (delim) *delim = '\0';                         // Replace with nul
    parser.parse(gcode);                              // Parse the current command
    process_parsed_command(true);                     // Process it
    if (!delim) break;                                // Last command?
    gcode = delim + 1;                                // Get the next command
  }
  parser.parse(saved_cmd);                            // Restore the parser state
}

#if ENABLED(HOST_KEEPALIVE_FEATURE)

  /**
   * Output a "busy" message at regular intervals
   * while the machine is not accepting commands.
   */
  void GcodeSuite::host_keepalive() {
    // Do not log keeaplive messages, only print to serial
    SerialLoggingDisabler sld;

    const millis_t ms = millis();
    static millis_t next_busy_signal_ms = 0;
    if (!suspend_auto_report && host_keepalive_interval && busy_state != NOT_BUSY) {
      if (PENDING(ms, next_busy_signal_ms)) return;
      switch (busy_state) {
        case IN_HANDLER:
        case IN_PROCESS:
          SERIAL_ECHO_MSG(MSG_BUSY_PROCESSING);
          break;
        case PAUSED_FOR_USER:
          SERIAL_ECHO_MSG(MSG_BUSY_PAUSED_FOR_USER);
          break;
        case PAUSED_FOR_INPUT:
          SERIAL_ECHO_MSG(MSG_BUSY_PAUSED_FOR_INPUT);
          break;
        default:
          break;
      }
    }
    next_busy_signal_ms = ms + host_keepalive_interval * 1000UL;
  }

#endif // HOST_KEEPALIVE_FEATURE
