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
 * motion.cpp
 */

#include <array>

#include "motion.h"
#include "bsod.h"
#include "endstops.h"
#include "stepper.h"
#include "planner.h"
#include "temperature.h"

#include "../gcode/gcode.h"

#include "../inc/MarlinConfig.h"
#include "../Marlin.h"

#include "metric.h"

#if HAS_BED_PROBE
  #include "probe.h"
#endif

#if HAS_LEVELING
  #include "../feature/bedlevel/bedlevel.h"
#endif

#include "../feature/print_area.h"
#include <option/has_crash_detection.h>

#if ENABLED(NOZZLE_LOAD_CELL)
  #include "loadcell.hpp"
#endif

#if ENABLED(EXTENSIBLE_UI)
  #include "../lcd/ultralcd.h"
#endif

#if ENABLED(SENSORLESS_HOMING)
  #include "../feature/motordriver_util.h"

  #if HAS_CRASH_DETECTION()
    #include "../feature/prusa/crash_recovery.hpp"
  #endif
#endif

#if HAS_PRECISE_HOMING()
  #include "prusa/homing_cart.hpp"
#endif

#include <config_store/store_c_api.h>  // for has_inverted_axis

#if !(BOARD_IS_DWARF())
#include "configuration.hpp"
#endif

#include <option/has_nozzle_cleaner.h>
#include <option/has_wastebin.h>
#include <option/has_dwarf.h>
#include <option/has_emergency_stop.h>
#include <option/has_toolchanger.h>

#include <option/has_emergency_stop.h>
#if HAS_EMERGENCY_STOP()
#include <feature/emergency_stop/emergency_stop.hpp>
#endif

#include <option/has_ceiling_clearance.h>
#if HAS_CEILING_CLEARANCE()
  #include <feature/ceiling_clearance/ceiling_clearance.hpp>
#endif

#include <feature/gcode_exception/gcode_exception.hpp>

#include <serial_logging_disabler.hpp>
#include <raii/auto_restore.hpp>

#define XYZ_CONSTS(T, NAME, OPT) const PROGMEM XYZval<T> NAME##_P = { X_##OPT, Y_##OPT, Z_##OPT }

XYZ_CONSTS(float, base_min_pos,   MIN_POS);
XYZ_CONSTS(float, base_max_pos,   MAX_POS);
XYZ_CONSTS(float, base_home_pos,  HOME_POS);
XYZ_CONSTS(float, max_length,     MAX_LENGTH);
XYZ_CONSTS(float, home_bump_mm,   HOME_BUMP_MM);
XYZ_CONSTS(signed char, home_dir, HOME_DIR);

AxesHomeLevel axes_home_level = AxesHomeLevel::no_axes_homed;

// Relative Mode. Enable with G91, disable with G90.
bool relative_mode; // = false;

xyze_pos_t current_position = { X_HOME_POS, Y_HOME_POS, Z_HOME_POS };

void set_current_position(const xyze_pos_t &native) {
  current_position = native;
}

MachinePosXYZE current_machine_position() {
  return to_machine_pos(current_position);
}

/**
 * Cartesian Destination
 *   The destination for a move, filled in by G-code movement commands,
 *   and expected by functions like 'prepare_move_to'.
 *   G-codes can set destination using 'get_destination_from_command'
 */
xyze_pos_t destination; // {0}

// The active extruder (tool). Set with T<extruder> command.
#if EXTRUDERS > 1
  std::atomic<uint8_t> active_extruder; // = 0
#endif

// Extruder offsets
#if HAS_HOTEND_OFFSET
  StrongIndexArray<xyz_pos_t, HOTENDS, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> hotend_offset;  // Initialized by settings.load()
  xyz_pos_t hotend_currently_applied_offset;
  void reset_hotend_offset(PhysicalToolIndex tool_index) {
    hotend_offset[tool_index] = {};
  }

  void reset_hotend_offsets() {
    for (auto &offset : hotend_offset) {
      offset = {};
    }
  }
#endif

// The feedrate for the current move, often used as the default if
// no other feedrate is specified. Overridden for special moves.
// Set by the last G0 through G5 command's "F" parameter.
// Functions that override this for custom moves *must always* restore it!
feedRate_t feedrate_mm_s =
  #ifdef DEFAULT_FEEDRATE
    DEFAULT_FEEDRATE;
  #else
    MMM_TO_MMS(1500);
  #endif
int16_t feedrate_percentage = 100;

// Homing feedrate is const progmem - compare to constexpr in the header
const feedRate_t homing_feedrate_mm_s[XYZ] PROGMEM = {
  MMM_TO_MMS(HOMING_FEEDRATE_XY), MMM_TO_MMS(HOMING_FEEDRATE_XY),
  MMM_TO_MMS(HOMING_FEEDRATE_Z)
};

#if HAS_PRECISE_HOMING()
float homing_bump_divisor[] = { 0, 0, 0 }; // on printers with HAS_PRECISE_HOMING, the divisor will be loaded from eeprom
#else
float homing_bump_divisor[] = HOMING_BUMP_DIVISOR;
#endif

/**
 * The workspace can be offset by some commands, or
 * these offsets may be omitted to save on computation.
 */
#if HAS_POSITION_SHIFT
  // The distance that XYZ has been offset by G92. Reset by G28.
  xyz_pos_t position_shift{0};
#endif
#if HAS_HOME_OFFSET
  // This offset is added to the configured home position.
  // Set by M206, M428, or menu item. Saved to EEPROM.
  xyz_pos_t home_offset{0};
#endif
#if HAS_HOME_OFFSET && HAS_POSITION_SHIFT
  // The above two are combined to save on computes
  xyz_pos_t workspace_offset{0};
#endif

// Returns true if XY lies in the region where a printed model can exist:
// inside both the heated bed rectangle and the slicer-declared print area (M555).
bool is_xy_in_print_region(const xy_pos_t &xy) {
    const bool on_bed = xy.x >= 0 && xy.x <= X_BED_SIZE
                     && xy.y >= 0 && xy.y <= Y_BED_SIZE;
    return on_bed && print_area.get_bounding_rect().contains(xy);
}

/**
 * Output the current position to serial
 */
void report_current_position() {
  // Do not log coordinates, only print to serial
  SerialLoggingDisabler sld;
  const auto lpos = current_position.asLogical();
  SERIAL_ECHOPAIR("X:", lpos.x, " Y:", lpos.y, " Z:", lpos.z, " E:", current_position.e);

  stepper.report_positions();
}

/**
 * sync_plan_position
 *
 * Set the planner/stepper positions directly from current_position with
 * no kinematic translation. Used for homing axes and cartesian/core syncing.
 */
void sync_plan_position() {
  if (!planner.draining()) {
    planner.set_position_mm(current_position);
  }
}

void sync_plan_position_e(std::optional<uint8_t> e_axis_index) {
  if (!planner.draining()) {
    planner.set_e_position_mm(current_position.e, e_axis_index);
  }
}

void sync_e_position_to(float e) {
  xyze_pos_t target = current_position;
  target.e = e;
  set_current_position(target);
  
  sync_plan_position_e();
}

/**
 * Set the current_position for an axis based on
 * the stepper positions, removing any leveling that
 * may have been applied.
 *
 * To prevent small shifts in axis position always call
 * sync_plan_position after updating axes with this.
 *
 * To keep hosts in sync, always call report_current_position
 * after updating the current_position.
 */
void set_current_from_steppers_for_axis(const AxisEnum axis) {
  // Cartesian conversion result goes here:
  MachinePosXYZE machine_position;
  planner.get_axis_position_mm(machine_position);

  const xyze_pos_t native_position = to_native_pos(machine_position);
  
  if (axis == ALL_AXES_ENUM)
    current_position = native_position;
  else
    current_position[axis] = native_position[axis];
}


/**
 * Set the current_position for all axes based on
 * the stepper positions.
 */
void set_current_from_steppers() {
  set_current_from_steppers_for_axis(ALL_AXES_ENUM);
}

/**
 * Plans a line movement to the current_position from the last point
 * in the planner's buffer.
 * Suitable for homing, does not apply UBL.
 */
void line_to_current_position(const feedRate_t &fr_mm_s/*=feedrate_mm_s*/) {
  planner.buffer_line(current_position, fr_mm_s, PhysicalToolIndex::currently_selected());
}

void line_to_machine_pos(const MachinePosXYZE &target, feedRate_t fr_mm_s, const MoveHints &hints) {
    planner.buffer_line(target, fr_mm_s, PhysicalToolIndex::currently_selected(), PlannerHints { .move = hints });
    set_current_position(to_native_pos(target));
}

void line_to_machine_pos(const MachinePosXYZ &target, feedRate_t fr_mm_s, const MoveHints &hints) {
    auto target_xyze = current_machine_position();
    target_xyze.set(target);
    return line_to_machine_pos(target_xyze, fr_mm_s, hints);
}

void prepare_internal_move_to_destination(const feedRate_t &fr_mm_s/*=0.0f*/, const PrepareMoveHints & hints) {
  PrepareMoveHints hints_ = hints;
  hints_.move.ignore_e_factor = true;
  hints_.scale_feedrate = false;
  prepare_move_to(destination, fr_mm_s ?: feedrate_mm_s, hints_);
}

/**
 * Performs a blocking fast parking move to (X, Y, Z) and sets the current_position.
 * Parking (Z-Manhattan): Moves XY and Z independently. Raises Z before or lowers Z after XY motion.
 */
void do_blocking_move_to(const float rx, const float ry, const float rz, const feedRate_t &fr_mm_s/*=0.0*/, Segmented segmented) {
  const feedRate_t z_feedrate = fr_mm_s ?: homing_feedrate(Z_AXIS),
                  xy_feedrate = fr_mm_s ?: feedRate_t(XY_PROBE_FEEDRATE_MM_S);

  plan_park_move_to(rx, ry, rz, xy_feedrate, z_feedrate, segmented);
  planner.synchronize();
}

/// Z-Manhattan fast move
void plan_park_move_to(const float rx, const float ry, const float rz, const feedRate_t &fr_xy, const feedRate_t &fr_z, Segmented segmented) {
  // If Z needs to raise, do it before moving XY
  if (current_position.z < rz) {
    destination = current_position;
    destination.z = rz;
    prepare_internal_move_to_destination(fr_z, { .do_segment = segmented == Segmented::yes });
  }

  destination = current_position;
  destination.set(rx, ry);
  prepare_internal_move_to_destination(fr_xy, { .do_segment = segmented == Segmented::yes });

  // If Z needs to lower, do it after moving XY
  if (current_position.z > rz) {
    destination = current_position;
    destination.z = rz;
    prepare_internal_move_to_destination(fr_z, { .do_segment = segmented == Segmented::yes });
  }
}

void do_blocking_move_to(const xy_pos_t &raw, const feedRate_t &fr_mm_s/*=0.0f*/) {
  do_blocking_move_to(raw.x, raw.y, current_position.z, fr_mm_s);
}
void do_blocking_move_to(const xyz_pos_t &raw, const feedRate_t &fr_mm_s/*=0.0f*/) {
  do_blocking_move_to(raw.x, raw.y, raw.z, fr_mm_s);
}
void do_blocking_move_to(const xyze_pos_t &raw, const feedRate_t &fr_mm_s/*=0.0f*/) {
  do_blocking_move_to(raw.x, raw.y, raw.z, fr_mm_s);
}

void do_blocking_move_to_x(const float &rx, const feedRate_t &fr_mm_s/*=0.0*/) {
  do_blocking_move_to(rx, current_position.y, current_position.z, fr_mm_s);
}
void do_blocking_move_to_y(const float &ry, const feedRate_t &fr_mm_s/*=0.0*/) {
  do_blocking_move_to(current_position.x, ry, current_position.z, fr_mm_s);
}
void do_blocking_move_to_z(const float &rz, const feedRate_t &fr_mm_s/*=0.0*/, Segmented segmented) {
  do_blocking_move_to(current_position.x, current_position.y, rz, fr_mm_s, segmented);
}

void do_blocking_move_to_xy(const float &rx, const float &ry, const feedRate_t &fr_mm_s/*=0.0*/) {
  do_blocking_move_to(rx, ry, current_position.z, fr_mm_s);
}
void do_blocking_move_to_xy(const xy_pos_t &raw, const feedRate_t &fr_mm_s/*=0.0f*/) {
  do_blocking_move_to_xy(raw.x, raw.y, fr_mm_s);
}

#if HAS_Z_AXIS
  uint8_t do_z_clearance(const float zclear, const bool lower_allowed/*=false*/) {
    float zdest = zclear;
    if (!lower_allowed) NOLESS(zdest, current_position.z);
    NOMORE(zdest, Z_MAX_POS);

    if (axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
      // axis position is known: perform a regular move
      do_blocking_move_to_z(zdest);
      
    } else if (zdest != current_position.z) {
      // axis position is unknown: perform a homing move to detect the endstop

      const auto distance = zdest - current_position.z;

      // Move as a homing move to stop if we reach endstop
      // Use HOMING_FEEDRATE_INVERTED_Z - the default homing feedrate is for loadcell probing
      const auto trigger_state = do_homing_move(Z_AXIS, distance, HOMING_FEEDRATE_INVERTED_Z);

      if (planner.draining()) {
        return 0;
      }

      return trigger_state;
    }

    return 0;
  }
#endif

//
// Prepare to do endstop or probe moves with custom feedrates.
//  - Save / restore current feedrate and multiplier
//
static float saved_feedrate_mm_s;
static int16_t saved_feedrate_percentage;
void remember_feedrate_and_scaling() {
  saved_feedrate_mm_s = feedrate_mm_s;
  saved_feedrate_percentage = feedrate_percentage;
}
void remember_feedrate_scaling_off() {
  remember_feedrate_and_scaling();
  feedrate_percentage = 100;
}
void restore_feedrate_and_scaling() {
  feedrate_mm_s = saved_feedrate_mm_s;
  feedrate_percentage = saved_feedrate_percentage;
}

#if HAS_SOFTWARE_ENDSTOPS

  bool soft_endstops_enabled = true;

  // Software Endstops are based on the configured limits.
  axis_limits_t soft_endstop = {
    { X_MIN_POS, Y_MIN_POS, Z_MIN_POS },
    { X_MAX_POS, Y_MAX_POS, Z_MAX_POS }
  };

  /**
   * Software endstops can be used to monitor the open end of
   * an axis that has a hardware endstop on the other end. Or
   * they can prevent axes from moving past endstops and grinding.
   *
   * To keep doing their job as the coordinate system changes,
   * the software endstop positions must be refreshed to remain
   * at the same positions relative to the machine.
   */
  void update_software_endstops(const AxisEnum axis
    #if HAS_HOTEND_OFFSET
      , const uint8_t old_tool_index/*=0*/, const uint8_t new_tool_index/*=0*/
    #endif
  ) {

    #if HAS_HOTEND_OFFSET && !HAS_TOOLCHANGER()
      // #error dead code found by automatic analyses (see BFW-5461)

      // Software endstops are relative to the tool 0 workspace, so
      // the movement limits must be shifted by the tool offset to
      // retain the same physical limit when other tools are selected.
      if (old_tool_index != new_tool_index) {
        const float offs = hotend_offset[new_tool_index][axis] - hotend_offset[old_tool_index][axis];
        soft_endstop.min[axis] += offs;
        soft_endstop.max[axis] += offs;
      }
      else {
        const float offs = hotend_offset[active_extruder][axis];
        soft_endstop.min[axis] = base_min_pos(axis) + offs;
        soft_endstop.max[axis] = base_max_pos(axis) + offs;
      }

    #else

      soft_endstop.min[axis] = base_min_pos(axis);
      soft_endstop.max[axis] = base_max_pos(axis);

    #endif

  if (DEBUGGING(LEVELING))
    SERIAL_ECHOLNPAIR("Axis ", axis_codes[axis], " min:", soft_endstop.min[axis], " max:", soft_endstop.max[axis]);
}

  /**
   * Constrain the given coordinates to the software endstops.
   */
  void apply_motion_limits(xyz_pos_t &target) {

    if (!soft_endstops_enabled) return;

    if(!axes_need_homing(_BV(X_AXIS))) {
      #if !HAS_SOFTWARE_ENDSTOPS || ENABLED(MIN_SOFTWARE_ENDSTOP_X)
        NOLESS(target.x, soft_endstop.min.x);
      #endif

      #if !HAS_SOFTWARE_ENDSTOPS || ENABLED(MAX_SOFTWARE_ENDSTOP_X)
        NOMORE(target.x, soft_endstop.max.x);
    #endif
    }
    
    if(!axes_need_homing(_BV(Y_AXIS))) {
      #if !HAS_SOFTWARE_ENDSTOPS || ENABLED(MIN_SOFTWARE_ENDSTOP_Y)
        NOLESS(target.y, soft_endstop.min.y);
      #endif

      #if !HAS_SOFTWARE_ENDSTOPS || ENABLED(MAX_SOFTWARE_ENDSTOP_Y)
        NOMORE(target.y, soft_endstop.max.y);
      #endif
    }

    if(!axes_need_homing(_BV(Z_AXIS))) {
      #if !HAS_SOFTWARE_ENDSTOPS || ENABLED(MIN_SOFTWARE_ENDSTOP_Z)
        NOLESS(target.z, soft_endstop.min.z);
      #endif
      #if !HAS_SOFTWARE_ENDSTOPS || ENABLED(MAX_SOFTWARE_ENDSTOP_Z)
        NOMORE(target.z, soft_endstop.max.z);
      #endif
    }
  }

#endif // HAS_SOFTWARE_ENDSTOPS

uint8_t axes_need_homing(uint8_t axis_bits/*=0x07*/, AxisHomeLevel required_level) {
  uint8_t result = 0;
  for(uint8_t i = 0; i < axes_home_level.size(); i++) {
    const uint8_t bit = (1 << i);
    if((axis_bits & bit) && (axes_home_level[i] < required_level)) {
      result |= bit;
    }
  }
  return result;
}

bool axis_unhomed_error(uint8_t axis_bits/*=0x07*/, AxisHomeLevel required_level) {
  if ((axis_bits = axes_need_homing(axis_bits, required_level))) {
    PGM_P home_first = GET_TEXT(MSG_HOME_FIRST);
    char msg[strlen_P(home_first)+1];
    sprintf_P(msg, home_first,
      TEST(axis_bits, X_AXIS) ? "X" : "",
      TEST(axis_bits, Y_AXIS) ? "Y" : "",
      TEST(axis_bits, Z_AXIS) ? "Z" : ""
    );
    SERIAL_ECHO_START();
    SERIAL_ECHOLN(msg);
    #if ENABLED(EXTENSIBLE_UI)
      ui.set_status(msg);
    #endif
    return true;
  }
  return false;
}

/**
 * Homing bump feedrate (mm/s)
 */
feedRate_t get_homing_bump_feedrate(const AxisEnum axis) {
  float hbd = homing_bump_divisor[axis];
  if (hbd < 0.5f) {
    hbd = 10;
    SERIAL_ECHO_MSG("Warning: Homing Bump Divisor < 0.5");
  }
  return homing_feedrate(axis) / float(hbd);
}

  #if ENABLED(SENSORLESS_HOMING)
    /**
     * Set sensorless homing if the axis has it, accounting for Core Kinematics.
     */
    sensorless_t start_sensorless_homing_per_axis(const AxisEnum axis) {
      planner.synchronize();
      
      sensorless_t stealth_states { false };

      switch (axis) {
        default: break;
        #if X_SENSORLESS
          case X_AXIS:
            #if HAS_CRASH_DETECTION()
              crash_s.start_sensorless_homing_per_axis(axis);
            #endif

            stealth_states.x = enable_crash_detection(X_AXIS);
            #if ANY(CORE_IS_XY, MARKFORGED_XY, MARKFORGED_YX) && Y_SENSORLESS
              stealth_states.y = enable_crash_detection(Y_AXIS);
            #elif CORE_IS_XZ && Z_SENSORLESS
              // #error dead code found by automatic analyses (see BFW-5461)
              stealth_states.z = enable_crash_detection(Z_AXIS);
            #endif
            break;
        #endif
        #if Y_SENSORLESS
          case Y_AXIS:
            #if HAS_CRASH_DETECTION()
              crash_s.start_sensorless_homing_per_axis(axis);
            #endif

            stealth_states.y = enable_crash_detection(Y_AXIS);
            #if ANY(CORE_IS_XY, MARKFORGED_XY, MARKFORGED_YX) && X_SENSORLESS
              stealth_states.x = enable_crash_detection(X_AXIS);
            #elif CORE_IS_YZ && Z_SENSORLESS
              // #error dead code found by automatic analyses (see BFW-5461)
              stealth_states.z = enable_crash_detection(Z_AXIS);
            #endif
            break;
        #endif
        #if Z_SENSORLESS
          case Z_AXIS:
            stealth_states.z = enable_crash_detection(Z_AXIS);
            TERN_(Z2_SENSORLESS, stealth_states.z2 = enable_crash_detection(Z2_AXIS));
            TERN_(Z3_SENSORLESS, stealth_states.z3 = enable_crash_detection(Z3_AXIS));
            #if CORE_IS_XZ && X_SENSORLESS
              // #error dead code found by automatic analyses (see BFW-5461)
              stealth_states.x = enable_crash_detection(X_AXIS);
            #elif CORE_IS_YZ && Y_SENSORLESS
              // #error dead code found by automatic analyses (see BFW-5461)
              stealth_states.y = enable_crash_detection(Y_AXIS);
            #endif
            break;
        #endif
        #if I_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case I_AXIS: stealth_states.i = enable_crash_detection(I_AXIS); break;
        #endif
        #if J_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case J_AXIS: stealth_states.j = enable_crash_detection(J_AXIS); break;
        #endif
        #if K_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case K_AXIS: stealth_states.k = enable_crash_detection(K_AXIS); break;
        #endif
        #if U_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case U_AXIS: stealth_states.u = enable_crash_detection(U_AXIS); break;
        #endif
        #if V_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case V_AXIS: stealth_states.v = enable_crash_detection(V_AXIS); break;
        #endif
        #if W_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case W_AXIS: stealth_states.w = enable_crash_detection(W_AXIS); break;
        #endif
      }

      #if ENABLED(IMPROVE_HOMING_RELIABILITY) && HOMING_SG_GUARD_DURATION > 0
        // #error dead code found by automatic analyses (see BFW-5461)
        sg_guard_period = millis() + default_sg_guard_duration;
      #endif

      return stealth_states;
    }

    void end_sensorless_homing_per_axis(const AxisEnum axis, sensorless_t enable_stealth) {
      switch (axis) {
        default: break;
        #if X_SENSORLESS
          case X_AXIS:
            #if HAS_CRASH_DETECTION()
              crash_s.end_sensorless_homing_per_axis(axis, enable_stealth.x);
            #else
              // #error dead code found by automatic analyses (see BFW-5461)
              disable_crash_detection(X_AXIS, enable_stealth.x);
              #if ANY(CORE_IS_XY, MARKFORGED_XY, MARKFORGED_YX) && Y_SENSORLESS
                // #error dead code found by automatic analyses (see BFW-5461)
                disable_crash_detection(Y_AXIS, enable_stealth.y);
              #elif CORE_IS_XZ && Z_SENSORLESS
                // #error dead code found by automatic analyses (see BFW-5461)
                disable_crash_detection(Z_AXIS, enable_stealth.z);
              #endif
            #endif
          break;
        #endif
        #if Y_SENSORLESS
          case Y_AXIS:
            #if HAS_CRASH_DETECTION()
              crash_s.end_sensorless_homing_per_axis(axis, enable_stealth.y);
            #else
              // #error dead code found by automatic analyses (see BFW-5461)
              disable_crash_detection(Y_AXIS, enable_stealth.y);
              #if ANY(CORE_IS_XY, MARKFORGED_XY, MARKFORGED_YX) && X_SENSORLESS
                // #error dead code found by automatic analyses (see BFW-5461)
                disable_crash_detection(X_AXIS, enable_stealth.x);
              #elif CORE_IS_YZ && Z_SENSORLESS
                // #error dead code found by automatic analyses (see BFW-5461)
                disable_crash_detection(Z_AXIS, enable_stealth.z);
              #endif
            #endif
          break;
        #endif
        #if Z_SENSORLESS
          case Z_AXIS:
            disable_crash_detection(Z_AXIS, enable_stealth.z);
            TERN_(Z2_SENSORLESS, disable_crash_detection(Z2_AXIS, enable_stealth.z2));
            TERN_(Z3_SENSORLESS, disable_crash_detection(Z3_AXIS, enable_stealth.z3));
            #if CORE_IS_XZ && X_SENSORLESS
              // #error dead code found by automatic analyses (see BFW-5461)
              disable_crash_detection(X_AXIS, enable_stealth.x);
            #elif CORE_IS_YZ && Y_SENSORLESS
              // #error dead code found by automatic analyses (see BFW-5461)
              disable_crash_detection(Y_AXIS, enable_stealth.y);
            #endif
            break;
        #endif
        #if I_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case I_AXIS: disable_crash_detection(I_AXIS, enable_stealth.i); break;
        #endif
        #if J_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case J_AXIS: disable_crash_detection(J_AXIS, enable_stealth.j); break;
        #endif
        #if K_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case K_AXIS: disable_crash_detection(K_AXIS, enable_stealth.k); break;
        #endif
        #if U_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case U_AXIS: disable_crash_detection(U_AXIS, enable_stealth.u); break;
        #endif
        #if V_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case V_AXIS: disable_crash_detection(V_AXIS, enable_stealth.v); break;
        #endif
        #if W_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          case W_AXIS: disable_crash_detection(W_AXIS, enable_stealth.w); break;
        #endif
      }
    }

  #endif // SENSORLESS_HOMING

uint8_t do_homing_move_axis_rel(const AxisEnum axis, const float distance, const feedRate_t fr_mm_s) {
  debug_assert(fr_mm_s != 0.f);

  // If you're seeing either of these BSODs,
  // you're probably calling do_homing_move_axis_rel instead of do_homing_move
  
  if(axis == X_AXIS || axis == Y_AXIS) {
    #if HAS_PHASE_STEPPING()
      // Sensorless homing does not work with phstep enabled
      if(phase_stepping::axis_states[axis].enabled) {
        bsod("Homing w/ phstep");
      }
    #endif
    #if ENABLED(SENSORLESS_HOMING)
      if(!axis_crash_detection_enabled.test(axis)) {
        bsod("Homing w/o crash detection");
      }
    #endif

    // Funny thing, endstops can be disabled when homing Z at this moment
    // I wish I had the nerves to try to fix this...
    if(!endstops.is_enabled() && !planner.draining()) {
      bsod("Homing w/o endstops");
    }
  }

  // avoid trashing the position when aborted
  planner.synchronize();
  if (planner.draining())
    return 0;

  // Make sure steppers position is properly synced with current_position
  sync_plan_position();

  // Clear prior hit state
  endstops.validate_homing_move();

  // Execute the move.
  // If endstops would be hit, the move will get interrupted.
  {
    MachinePosXYZE destination = planner.get_machine_position_mm();
    destination[axis] += distance;

    // Important: do not apply motion limits on this move
    planner.buffer_segment(destination, fr_mm_s, PhysicalToolIndex::currently_selected());
    planner.synchronize();
  }

  const auto hit_state = endstops.trigger_state();
  endstops.validate_homing_move();
  
  // Update the actual current position from the motors
  // Do this in both cases, if we hit something or not, to unify the code.
  // If the homing move was set up properly, there should not be any skips, so we should be able to get the correct position
  set_current_from_steppers_for_axis(axis);
  sync_plan_position();

  return hit_state;
}

/**
 * Home an individual linear axis
 * @param homing_z_with_probe false to use sensorless homing instead of probe
 * @return endstop trigger state at the end of the move
 */
uint8_t do_homing_move(const AxisEnum axis, const float distance, const feedRate_t fr_mm_s, [[maybe_unused]] bool can_move_back_before_homing, [[maybe_unused]] bool homing_z_with_probe) {
  planner.synchronize();

  TemporaryGlobalEndstopsState _es(true);
  phase_stepping::EnsureSuitableForHoming phstep_disabler;

  #if HAS_CEILING_CLEARANCE()
    // The homing move is doing all sorts of voodoo with the positions and was triggering false ceiling clearance events
    buddy::CeilingClearanceCheckDisabler ccd;
  #endif

    #if ENABLED(SENSORLESS_HOMING)
      sensorless_t stealth_states;
    #endif

  #if HOMING_Z_WITH_PROBE && QUIET_PROBING
    // #error dead code found by automatic analyses (see BFW-5461)
    if (axis == Z_AXIS && homing_z_with_probe) {
      probing_pause(true);
    }
  #endif

  #if HOMING_Z_WITH_PROBE
    [[maybe_unused]] bool moving_probe_toward_bed = false;
    if (axis == Z_AXIS && homing_z_with_probe)
      moving_probe_toward_bed = (home_dir(axis) > 0) == (distance > 0);
  #endif

      // Disable stealthChop if used. Enable diag1 pin on driver.
      #if ENABLED(SENSORLESS_HOMING)
        bool enable_sensorless_homing =
        #if HOMING_Z_WITH_PROBE && !Z_SENSORLESS
          // #error dead code found by automatic analyses (see BFW-5461)
          !moving_probe_toward_bed || !homing_z_with_probe
        #else
          true
        #endif
          ;
        if (enable_sensorless_homing) {
          stealth_states = start_sensorless_homing_per_axis(axis);
          #if SENSORLESS_STALLGUARD_DELAY
            // #error dead code found by automatic analyses (see BFW-5461)
            safe_delay(SENSORLESS_STALLGUARD_DELAY); // Short delay needed to settle
          #endif
	}
      #endif

  const feedRate_t real_fr_mm_s = fr_mm_s ?: homing_feedrate(axis);

  uint8_t trigger_state = 0;

  #if ENABLED(MOVE_BACK_BEFORE_HOMING)
    if (can_move_back_before_homing && ((axis == X_AXIS) || (axis == Y_AXIS))) {
      float dist = (distance > 0) ? -MOVE_BACK_BEFORE_HOMING_DISTANCE : MOVE_BACK_BEFORE_HOMING_DISTANCE;
      trigger_state |= do_homing_move_axis_rel(axis, dist, real_fr_mm_s);
  }
  #endif

  #if ENABLED(NOZZLE_LOAD_CELL) && HOMING_Z_WITH_PROBE
    // NOTE: These RAII guards must outlive the homing move below; do not scope them inside an if block.

    // HighPrecision needs to be enabled with some time margin to prime the filters.
    // If it hasn't been already we're being called in single-probe mode, enable it temporarily.
    bool enableHighPrecision = !loadcell.IsHighPrecisionEnabled() && moving_probe_toward_bed;
    if (enableHighPrecision) SERIAL_ECHO_MSG("probe: enabling high-precision for single-probe mode");
    auto loadcellPrecisionEnabler = Loadcell::HighPrecisionEnabler(loadcell, enableHighPrecision);
    auto H = loadcell.CreateLoadAboveErrEnforcer(moving_probe_toward_bed);
    // Arm only for descents toward the bed (tare + Z-probe); X/Y homing stays unarmed.
    auto safetyArmer = Loadcell::ProbeSafetyArmer(loadcell, moving_probe_toward_bed);
    if (moving_probe_toward_bed) {
      safe_delay(Z_FIRST_PROBE_DELAY); // dampen the system before the tare
      loadcell.WaitBarrier(); // Sync samples before tare
      loadcell.Tare(Loadcell::TareMode::Continuous);
      endstops.enable_z_probe();
    }
  #endif

  trigger_state |= do_homing_move_axis_rel(axis, distance, real_fr_mm_s);

  #if ENABLED(NOZZLE_LOAD_CELL) && HOMING_Z_WITH_PROBE
    if (moving_probe_toward_bed) {
      endstops.enable_z_probe(false);
    }
  #endif

  #if HOMING_Z_WITH_PROBE && QUIET_PROBING
    // #error dead code found by automatic analyses (see BFW-5461)
    if (axis == Z_AXIS && homing_z_with_probe) {
      probing_pause(false);
    }
  #endif

      // Re-enable stealthChop if used. Disable diag1 pin on driver.
      #if ENABLED(SENSORLESS_HOMING)
	if (enable_sensorless_homing) {
	  end_sensorless_homing_per_axis(axis, stealth_states);
          #if SENSORLESS_STALLGUARD_DELAY
            // #error dead code found by automatic analyses (see BFW-5461)
            safe_delay(SENSORLESS_STALLGUARD_DELAY); // Short delay needed to settle
          #endif
	}
      #endif

  return trigger_state;
}

void prepare_move_to(xyze_pos_t target, feedRate_t fr_mm_s, PrepareMoveHints hints) {
  static_assert(sizeof(PrepareMoveHints) <= 4, "Change the parameter to a reference");

  if(hints.apply_motion_limits) {
    apply_motion_limits(target);
  }

  if (!position_is_reachable(target.xy())) {
      return;
  }

  if(hints.move.extrusion_safety_checks) {
    #if EITHER(PREVENT_COLD_EXTRUSION, PREVENT_LENGTHY_EXTRUDE)
      if (!DEBUGGING(DRYRUN)) {
        if (target.e != current_position.e) {
          #if ENABLED(PREVENT_COLD_EXTRUSION)
            const auto tool = PhysicalToolIndex::currently_selected_opt();
            if (!tool.has_value() || thermalManager.tooColdToExtrude(tool.value())) {
              current_position.e = target.e; // Behave as if the move really took place, but ignore E part
              SERIAL_ECHO_MSG(MSG_ERR_COLD_EXTRUDE_STOP);
            }
          #endif // PREVENT_COLD_EXTRUSION
          #if ENABLED(PREVENT_LENGTHY_EXTRUDE)
            const std::optional<VirtualToolIndex> vt = VirtualToolIndex::currently_selected_opt();
            const float e_fac = vt.has_value() ? planner.e_factor[*vt] : 1.0f;
            const float e_delta = ABS(target.e - current_position.e) * e_fac;
            if (e_delta > (EXTRUDE_MAXLENGTH)) {
              current_position.e = target.e; // Behave as if the move really took place, but ignore E part
              SERIAL_ECHO_MSG(MSG_ERR_LONG_EXTRUDE_STOP);
            }
          #endif // PREVENT_LENGTHY_EXTRUDE
        }
      }
    #endif // PREVENT_COLD_EXTRUSION || PREVENT_LENGTHY_EXTRUDE
  }

  if(hints.scale_feedrate) {
    fr_mm_s = MMS_SCALED(fr_mm_s);
  }

  const xyze_float_t full_diff = target - current_position;
  [[maybe_unused]] const float xy_distance = xy_float_t{{{full_diff.x, full_diff.y}}}.magnitude();

  using SegmentCount = uint16_t;
  SegmentCount segment_count = 1;

  if (hints.do_segment) {
    // Segment the move enough for MBL
    #if ENABLED(AUTO_BED_LEVELING_UBL)
    if(planner.leveling_active && planner.leveling_active_at_z(target.z)) {
      segment_count = std::max(segment_count, static_cast<SegmentCount>(std::round(xy_distance / LEVELED_SEGMENT_LENGTH)));
    }
    #endif

    // Segment the moves to be able to do emergency stop quickly
    #if HAS_EMERGENCY_STOP()
    {
      // Using xy_distance here is quite approximate, but good enough for our purposes here
      const float duration = (xy_distance / fr_mm_s);

      segment_count = std::max(segment_count, static_cast<SegmentCount>(std::abs(full_diff.z) / buddy::EmergencyStop::max_segment_z_mm));
      segment_count = std::max(segment_count, static_cast<SegmentCount>(duration / buddy::EmergencyStop::max_segment_time_s));
    }
    #endif
  }

  xyze_pos_t segment_pos = current_position;
  const xyze_pos_t segment_diff = full_diff / segment_count;

  const PlannerHints planner_hints {
    .move = hints.move,
  };

  const auto buffer_move = [&](const xyze_pos_t &target) {
    planner.buffer_segment(to_machine_pos(target), fr_mm_s, PhysicalToolIndex::currently_selected(), planner_hints);
    set_current_position(target);
  };

  while (--segment_count) {
    segment_pos += segment_diff;
    buffer_move(segment_pos);
  }
  buffer_move(target);
}

/**
 * Set an axis' current position to its home position (after homing).
 *
 * For Core and Cartesian robots this applies one-to-one when an
 * individual axis has been homed.
 *
 *
 * Callers must sync the planner position after calling this!
 *
 * @param homing_z_with_probe false when sensorless homing was used instead of probe
 */
void set_axis_is_at_home(const AxisEnum axis, AxisHomeLevel level, [[maybe_unused]] bool homing_z_with_probe) {
  // ensure we're not within an aborted move: caller needs to check!
  debug_assert(!planner.draining());

  axes_home_level[axis] = level; 

  auto new_machine_position = current_machine_position();

  new_machine_position[axis] = base_home_pos(axis)
      #if HAS_PRECISE_HOMING()
        - calibrated_home_offset(axis)
      #endif
    ;

  /**
   * Z Probe Z Homing? Account for the probe's Z offset.
   */
  #if HAS_BED_PROBE && Z_HOME_DIR < 0
    if (axis == Z_AXIS) {
      #if HOMING_Z_WITH_PROBE
        if (homing_z_with_probe) {
          new_machine_position.z -= probe_offset.z;
          #if HAS_HOTEND_OFFSET
          new_machine_position.z -= hotend_currently_applied_offset.z;
          #endif
        }
      #endif /*HOMING_Z_WITH_PROBE*/
    }
  #endif

    set_current_position(to_native_pos(new_machine_position));
}

/**
 * Set an axis' to be unhomed.
 */
void set_axis_is_not_at_home(const AxisEnum axis) {
  axes_home_level[axis] = AxisHomeLevel::not_homed;
}

// those metrics are intentionally not static, as it is expected that they might be referenced
// from outside this file for early registration
METRIC_DEF(metric_home_diff, "home_diff", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED);

/**
 * @brief Call this when homing fails, it will try to recover.
 * After calling this, homing needs to end right away with fail return.
 * @param fallback_error called when homing cannot be recovered
 * @param crash_was_active true if crash recovery was active, this is used if crash_recovery was temporarily disabled
 * @param recover_z true if failed during Z homing and should rehome Z
 */
void homing_failed(stdext::inplace_function<void()> fallback_error, [[maybe_unused]] bool crash_was_active, bool recover_z) {
  #if HAS_CRASH_DETECTION()
    const bool is_active = crash_s.is_active();
    if ((is_active || crash_was_active) // Allow if crash recovery was temporarily disabled
      && (crash_s.get_state() == Crash_s::PRINTING)) {
      if (!is_active && crash_was_active) {
        crash_s.activate(); // Reactivate temporarily disabled crash recovery
      }
      if (crash_s.is_toolchange_in_progress()) {
        crash_s.set_state(Crash_s::TRIGGERED_TOOLCRASH);
      } else {
        crash_s.set_state(Crash_s::TRIGGERED_HOMEFAIL);
        if (recover_z) {
          crash_s.set_homefail_z();
        }
      }
      return;
    }

    if ((crash_s.get_state() == Crash_s::TRIGGERED_ISR)       // ISR crash happened, it will replay homing
      || (crash_s.get_state() == Crash_s::TRIGGERED_AC_FAULT) // Power panic, end quickly and don't do anything
      || (crash_s.get_state() == Crash_s::TRIGGERED_HOMEFAIL) // Rehoming is already in progress
      || (crash_s.get_state() == Crash_s::TRIGGERED_TOOLCRASH)
      || (crash_s.get_state() == Crash_s::RECOVERY) // Recovery in progress, it will know that homing didn't succeed from return
      || (crash_s.get_state() == Crash_s::REPEAT_WAIT)) {
      return; // Ignore
    }
  #endif

  fallback_error();
}

/**
 * Home an individual "raw axis" to its endstop.
 * This applies to XYZ on Cartesian and Core robots.
 *
 * At the end of the procedure the axis is marked as
 * homed and the current position of that axis is updated.
 * Kinematic robots should wait till all axes are homed
 * before updating the current position.
 *
 * @param axis Axist to home
 * @param fr_mm_s Homing feed rate in millimeters per second
 * @param invert_home_dir
 *  @arg @c false Default homing direction
 *  @arg @c true Home to opposite end of axis than default.
 *               Warning - axis is considered homed and in known position.
 *               @todo Current position is wrong in case of invert_home_dir true after this call.
 * @param enable_wavetable pointer to reenable wavetable during backoff move
 * @param can_calibrate allows/avoids re-calibration if homing is not successful
 * @param homing_z_with_probe default true, set to false to home without using probe (useful to calibrate Z on XL)
 * @return true on success
 */
bool homeaxis(const AxisEnum axis, const feedRate_t fr_mm_s, bool invert_home_dir,
  void (*enable_wavetable)(AxisEnum), [[maybe_unused]] bool can_calibrate, bool homing_z_with_probe, bool throw_homing_failed) {

  // clear the axis state while running
  axes_home_level[axis] = AxisHomeLevel::not_homed;

  #if HAS_CRASH_DETECTION()
    crash_s.not_for_replay();
    Crash_Temporary_Deactivate ctd;
    const bool orig_crash [[maybe_unused]] = ctd.get_orig_state();
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    constexpr bool orig_crash [[maybe_unused]] = false;
  #endif

  #define _CAN_HOME(A) \
    (axis == _AXIS(A) && ((A##_MIN_PIN > -1 && A##_HOME_DIR < 0) || (A##_MAX_PIN > -1 && A##_HOME_DIR > 0)))
  #define CAN_HOME_X _CAN_HOME(X)
  #define CAN_HOME_Y _CAN_HOME(Y)
  #define CAN_HOME_Z _CAN_HOME(Z)
  if (!CAN_HOME_X && !CAN_HOME_Y && !CAN_HOME_Z) return true;

  const int axis_home_dir = (
      invert_home_dir ? (-home_dir(axis)) : home_dir(axis)
  );

  #if ENABLED(NOZZLE_LOAD_CELL) && HOMING_Z_WITH_PROBE
    // Enable loadcell high precision across the entire axis homing to prime the noise filters
    auto loadcellPrecisionEnabler = Loadcell::HighPrecisionEnabler(loadcell, axis == Z_AXIS);
    if (axis == Z_AXIS && !loadcell_wait_streaming()) {
      return false;
    }
  #endif

  float (*min_diff)(uint8_t) = invert_home_dir ? axis_home_invert_min_diff : axis_home_min_diff;
  float (*max_diff)(uint8_t) = invert_home_dir ? axis_home_invert_max_diff : axis_home_max_diff;

  float probe_offset;
  for(size_t attempt = 0;;) {
    #if HAS_PRECISE_HOMING()
      if ((axis == X_AXIS || axis == Y_AXIS) && !invert_home_dir) {
        probe_offset = home_axis_precise(axis, axis_home_dir, can_calibrate, fr_mm_s);
        attempt = HOMING_MAX_ATTEMPTS; // call home_axis_precise() just once
      }
      else
    #endif
      {
        if (attempt > 0 && axis == Z_AXIS) {
          // Z has no move back and after the first attempt we might be left too close on the
          // build plate (for example, with a loadcell we're _on_ the plate). Move back now before
          // we attempt to probe again so that we can zero the sensor again.
          float bump = axis_home_dir * (
            #if HOMING_Z_WITH_PROBE
              (axis == Z_AXIS && homing_z_with_probe && (Z_HOME_BUMP_MM)) ? _MAX(Z_CLEARANCE_BETWEEN_PROBES, Z_HOME_BUMP_MM) :
            #endif
            home_bump_mm(axis)
          );
          current_position[axis] -= bump;
          line_to_current_position(fr_mm_s ? fr_mm_s : homing_feedrate(Z_AXIS));
          planner.synchronize();
        }

        probe_offset = homeaxis_single_run(axis, axis_home_dir, fr_mm_s, invert_home_dir, homing_z_with_probe, attempt) * static_cast<float>(axis_home_dir);
      }
    if (planner.draining()) {
      // move intentionally aborted, do not retry/kill
      return true;
    }

    // check if the offset is acceptable
    bool in_range = min_diff(axis) <= probe_offset && probe_offset <= max_diff(axis);
    metric_record_custom(&metric_home_diff, ",ax=%u,ok=%u v=%.3f,n=%u", (unsigned)axis, (unsigned)in_range, probe_offset, (unsigned)attempt);
    if (in_range) break; // OK offset in range

    // check whether we should try again
    if (++attempt >= HOMING_MAX_ATTEMPTS) {
      // not OK run out attempts
      set_axis_is_not_at_home(axis);
      
      if (throw_homing_failed) {
        static constexpr std::array error_codes {
          ErrCode::ERR_ELECTRO_HOMING_ERROR_X,
          ErrCode::ERR_ELECTRO_HOMING_ERROR_Y,
          ErrCode::ERR_ELECTRO_HOMING_ERROR_Z
        };

        homing_failed([code = error_codes[std::min(static_cast<size_t>(axis), error_codes.size() - 1)]]() { fatal_error(code); }, orig_crash, axis == Z_AXIS);
      }

      return false;
    }

    if((axis == X_AXIS || axis == Y_AXIS) && !invert_home_dir){
      //print only for normal homing, messages from precise homing are taken care inside precise homing
      ui.status_printf_P(0,"%c axis homing failed, retrying", axis_codes[axis]);
    }
  }
  #ifdef HOMING_BACKOFF_POST_MM
    constexpr xyz_float_t endstop_backoff = HOMING_BACKOFF_POST_MM;
    const float backoff_mm = endstop_backoff[
        axis
    ];
    if (backoff_mm) {
      if (enable_wavetable != NULL)
        enable_wavetable(axis);

      current_position[axis] -= ABS(backoff_mm) * axis_home_dir;
      line_to_current_position(
        #if HOMING_Z_WITH_PROBE
          (axis == Z_AXIS && homing_z_with_probe) ? MMM_TO_MMS(Z_PROBE_SPEED_FAST) :
        #endif
        homing_feedrate(axis)
      );
      planner.synchronize();

      SERIAL_ECHO_START();
      SERIAL_ECHOLNPAIR_F("Backoff ipos:", stepper.position_from_startup(axis));
    }
  #endif

  return true;
}

/**
 * home axis and
 * return distance between fast and slow probe
 * @param homing_z_with_probe default true, set to false to home without using probe (useful to calibrate Z on XL)
 */
float homeaxis_single_run(const HomeAxisSingleRunArgs &args) {
  const AxisEnum axis = args.axis;
  const int axis_home_dir = args.axis_home_dir;
  [[maybe_unused]] const bool homing_z_with_probe = args.homing_z_with_probe;

  // Homing Z towards the bed? Deploy the Z probe or endstop.
  #if HOMING_Z_WITH_PROBE
    if (axis == Z_AXIS && homing_z_with_probe && DEPLOY_PROBE()) {
      return NAN;
    }
  #endif

  const auto initial_throw_count = gcode_exceptions().throw_count();

  // Set flags for X, Y, Z motor locking
  #if ENABLED(Z_TRIPLE_ENDSTOPS)
    // #error dead code found by automatic analyses (see BFW-5461)
    switch (axis) {
      case Z_AXIS:
      stepper.set_separate_multi_axis(true);
      default: break;
    }
  #endif

  // Fast move towards endstop until triggered

  const feedRate_t real_fr_mm_s = args.fr_mm_s ?: homing_feedrate(axis);

  #if ENABLED(MOVE_BACK_BEFORE_HOMING)
    #ifndef MOVE_BACK_BEFORE_HOMING_DISTANCE_FIRST
      #define MOVE_BACK_BEFORE_HOMING_DISTANCE_FIRST MOVE_BACK_BEFORE_HOMING_DISTANCE
    #endif
    if ((axis == X_AXIS) || (axis == Y_AXIS)) {
      const float move_back_distance = args.attempt ? MOVE_BACK_BEFORE_HOMING_DISTANCE : MOVE_BACK_BEFORE_HOMING_DISTANCE_FIRST;
      do_homing_move(axis, axis_home_dir * -move_back_distance, real_fr_mm_s);
    }
  #endif // ENABLED(MOVE_BACK_BEFORE_HOMING)

  do_homing_move(axis, 1.5f * max_length(axis) * axis_home_dir, real_fr_mm_s, false, homing_z_with_probe);

  // When homing Z with probe respect probe clearance
  float bump = axis_home_dir * (
    #if HOMING_Z_WITH_PROBE
      (axis == Z_AXIS && homing_z_with_probe && (Z_HOME_BUMP_MM)) ? _MAX(Z_CLEARANCE_BETWEEN_PROBES, Z_HOME_BUMP_MM) :
    #endif
    home_bump_mm(axis)
  );

  // If a second homing move is configured...
  const uint8_t bump_count = (bump == 0) ? 0 : (axis == Z_AXIS) ? 2 : 1;
  const int steps_before_bump = stepper.position_from_startup(axis);
  int steps_after_bump[2];

  for(uint8_t i = 0; i < bump_count; i++) {

    // Move away from the endstop by the axis HOME_BUMP_MM
    // We CANNOT use a stallguarded move here - it somehow manages to screw up results of the homing sensitivity calibration on cartesian printers
    // BFW-8396
    current_position[axis] -= bump;
    planner.buffer_segment(current_position, real_fr_mm_s, PhysicalToolIndex::currently_selected());
    planner.synchronize();

    // Slow move towards endstop until triggered

    // Early abort if a quick stop was issued
    if (planner.draining() || gcode_exceptions().throw_count() != initial_throw_count)
      return NAN;

    feedRate_t bump_feedrate;

    #if HOMING_Z_WITH_PROBE
    if (axis == Z_AXIS) {
      if (axis_home_dir < 0) {
#if HAS_DWARF()
        // Note: for XL (and possibly anything else with dwarf, but we don't
        // really have anything like that) has a "remote" loadcell and needed
        // very fine tuning. That tuning works only on that default feedrate,
        // not on the slow one (on the slow one, the touch is not actually
        // detected).
        bump_feedrate = real_fr_mm_s;
#else
        bump_feedrate = MMM_TO_MMS(Z_PROBE_SPEED_SLOW);
#endif
      } else {
        // moving away from the bed
        bump_feedrate = HOMING_FEEDRATE_INVERTED_Z;
      }
    } else
    #endif //HOMING_Z_WITH_PROBE
    {
      bump_feedrate = args.fr_mm_s ?: get_homing_bump_feedrate(axis);
    }

    do_homing_move(axis, 2 * bump, bump_feedrate, false, homing_z_with_probe);

    steps_after_bump[i] = stepper.position_from_startup(axis);
  }

  #if ENABLED(Z_TRIPLE_ENDSTOPS)
    // #error dead code found by automatic analyses (see BFW-5461)
    const bool pos_dir = axis_home_dir > 0;
      if (axis == Z_AXIS) {
        // we push the function pointers for the stepper lock function into an array
        void (*lock[3]) (bool)= {&stepper.set_z_lock, &stepper.set_z2_lock, &stepper.set_z3_lock};
        float adj[3] = {0, endstops.z2_endstop_adj, endstops.z3_endstop_adj};

        void (*tempLock) (bool);
        float tempAdj;

        // manual bubble sort by adjust value
        if (adj[1] < adj[0]) {
          tempLock = lock[0], tempAdj = adj[0];
          lock[0] = lock[1], adj[0] = adj[1];
          lock[1] = tempLock, adj[1] = tempAdj;
        }
        if (adj[2] < adj[1]) {
          tempLock = lock[1], tempAdj = adj[1];
          lock[1] = lock[2], adj[1] = adj[2];
          lock[2] = tempLock, adj[2] = tempAdj;
        }
        if (adj[1] < adj[0]) {
          tempLock = lock[0], tempAdj = adj[0];
          lock[0] = lock[1], adj[0] = adj[1];
          lock[1] = tempLock, adj[1] = tempAdj;
        }

        if (pos_dir) {
          // normalize adj to smallest value and do the first move
          (*lock[0])(true);
          do_homing_move(axis, adj[1] - adj[0], 0, false, homing_z_with_probe);
          // lock the second stepper for the final correction
          (*lock[1])(true);
          do_homing_move(axis, adj[2] - adj[1], 0, false, homing_z_with_probe);
        }
        else {
          (*lock[2])(true);
          do_homing_move(axis, adj[1] - adj[2], 0, false, homing_z_with_probe);
          (*lock[1])(true);
          do_homing_move(axis, adj[0] - adj[1], 0, false, homing_z_with_probe);
        }

        stepper.set_z_lock(false);
        stepper.set_z2_lock(false);
        stepper.set_z3_lock(false);
      }

    // Reset flags for X, Y, Z motor locking
    switch (axis) {
        case Z_AXIS:
      stepper.set_separate_multi_axis(false);
      default: break;
    }
  #endif

    // Check if any of the moves were aborted and avoid setting any state
    if (planner.draining() || gcode_exceptions().throw_count() != initial_throw_count)
      return NAN;

  if (!args.invert_home_dir) {
    bool is_homed_precisely = false;
    if(axis == Z_AXIS) {
      // Z is homed precisely only if we used probe (so banging against the ceiling is not considered precise homing)
      is_homed_precisely = (!HOMING_Z_WITH_PROBE || homing_z_with_probe);

    } else {
      // If precise homing is enabled, there will be a precise refinement done in a separate function
      is_homed_precisely = (!HAS_PRECISE_HOMING() && !HAS_PRECISE_HOMING_COREXY());
    }

    set_axis_is_at_home(axis, is_homed_precisely ? AxisHomeLevel::full : AxisHomeLevel::imprecise, homing_z_with_probe);
  }
  sync_plan_position();

  destination[axis] = current_position[axis];

  // Put away the Z probe
  #if HOMING_Z_WITH_PROBE
    if (axis == Z_AXIS && homing_z_with_probe && STOW_PROBE()) {
      return NAN;
    }
  #endif

  switch(bump_count) {

  case 0:
    // We've only done one bump - no way to validate precision/repeatibility
    return 0;

  case 1:
    // One extra bump - compare original homing pos vs after bump
    return (steps_after_bump[0] - steps_before_bump) * planner.mm_per_step[axis];

  case 2:
    // Doing two bumps (presumably slower than original homing) - compare precision between the bumps
    return (steps_after_bump[1] - steps_after_bump[0]) * planner.mm_per_step[axis];

  default:
    // Should never happen
    std::abort();

  }
} // homeaxis()

#if HAS_WORKSPACE_OFFSET
  void update_workspace_offset(const AxisEnum axis) {
    workspace_offset[axis] = home_offset[axis] + position_shift[axis];
  }
#endif

#if HAS_M206_COMMAND
  /**
   * Change the home offset for an axis.
   * Also refreshes the workspace offset.
   */
  void set_home_offset(const AxisEnum axis, const float v) {
    home_offset[axis] = v;
    update_workspace_offset(axis);
  }
#endif // HAS_M206_COMMAND
