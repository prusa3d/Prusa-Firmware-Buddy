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

/**
 * motion.h
 *
 * High-level motion commands to feed the planner
 * Some of these methods may migrate to the planner class.
 */

#include "../inc/MarlinConfig.h"

#if HAS_BED_PROBE
  #include "probe.h"
#endif

#if IS_SCARA
  #include "scara.h"
#endif

// Axis homed and known-position states
extern uint8_t axis_homed, axis_known_position;
constexpr uint8_t xyz_bits = _BV(X_AXIS) | _BV(Y_AXIS) | _BV(Z_AXIS);
FORCE_INLINE bool all_axes_homed() { return (axis_homed & xyz_bits) == xyz_bits; }
FORCE_INLINE bool all_axes_known() { return (axis_known_position & xyz_bits) == xyz_bits; }
FORCE_INLINE void set_all_unhomed() { axis_homed = 0; }
FORCE_INLINE void set_all_unknown() { axis_known_position = 0; }

FORCE_INLINE bool homing_needed() {
  return !(
    #if ENABLED(HOME_AFTER_DEACTIVATE)
      all_axes_known()
    #else
      all_axes_homed()
    #endif
  );
}

// Error margin to work around float imprecision
constexpr float slop = 0.0001;

extern bool relative_mode;

extern xyze_pos_t current_position,  // High-level current tool position
                  destination;       // Destination for a move

// Scratch space for a cartesian result
extern xyz_pos_t cartes;

// Until kinematics.cpp is created, declare this here
#if IS_KINEMATIC
  extern abc_pos_t delta;
#endif

#if HAS_ABL_NOT_UBL
  extern float xy_probe_feedrate_mm_s;
  #define XY_PROBE_FEEDRATE_MM_S xy_probe_feedrate_mm_s
#elif defined(XY_PROBE_SPEED_INITIAL)
  #define XY_PROBE_FEEDRATE_MM_S MMM_TO_MMS(XY_PROBE_SPEED_INITIAL)
#else
  #define XY_PROBE_FEEDRATE_MM_S PLANNER_XY_FEEDRATE()
#endif

#if ENABLED(Z_SAFE_HOMING)
  constexpr xy_float_t safe_homing_xy = { Z_SAFE_HOMING_X_POINT, Z_SAFE_HOMING_Y_POINT };
#endif

/**
 * Feed rates are often configured with mm/m
 * but the planner and stepper like mm/s units.
 */
extern const feedRate_t homing_feedrate_mm_s[XYZ];
FORCE_INLINE feedRate_t homing_feedrate(const AxisEnum a) { return pgm_read_float(&homing_feedrate_mm_s[a]); }
feedRate_t get_homing_bump_feedrate(const AxisEnum axis);

extern feedRate_t feedrate_mm_s;

/**
 * Feedrate scaling is applied to all G0/G1, G2/G3, and G5 moves
 */
extern int16_t feedrate_percentage;
#define MMS_SCALED(V) ((V) * 0.01f * feedrate_percentage)

// The active extruder (tool). Set with T<extruder> command.
#if EXTRUDERS > 1
  extern uint8_t active_extruder;
#else
  constexpr uint8_t active_extruder = 0;
#endif

FORCE_INLINE float pgm_read_any(const float *p) { return pgm_read_float(p); }
FORCE_INLINE signed char pgm_read_any(const signed char *p) { return pgm_read_byte(p); }

#define XYZ_DEFS(T, NAME, OPT) \
  extern const XYZval<T> NAME##_P; \
  FORCE_INLINE T NAME(AxisEnum axis) { return pgm_read_any(&NAME##_P[axis]); }

XYZ_DEFS(float, base_min_pos,   MIN_POS);
XYZ_DEFS(float, base_max_pos,   MAX_POS);
XYZ_DEFS(float, base_home_pos,  HOME_POS);
XYZ_DEFS(float, max_length,     MAX_LENGTH);
XYZ_DEFS(float, home_bump_mm,   HOME_BUMP_MM);
XYZ_DEFS(signed char, home_dir, HOME_DIR);

#if HAS_WORKSPACE_OFFSET
  void update_workspace_offset(const AxisEnum axis);
#else
  #define update_workspace_offset(x) NOOP
#endif

#if HAS_HOTEND_OFFSET
  extern xyz_pos_t hotend_offset[HOTENDS];
  void reset_hotend_offsets();
#elif HOTENDS
  constexpr xyz_pos_t hotend_offset[HOTENDS] = { { 0 } };
#else
  constexpr xyz_pos_t hotend_offset[1] = { { 0 } };
#endif

#if HAS_SOFTWARE_ENDSTOPS

  typedef struct {
    bool _enabled, _loose;
    bool enabled() { return _enabled && !_loose; }

    xyz_pos_t min, max;
    void get_manual_axis_limits(const AxisEnum axis, float &amin, float &amax) {
      amin = -100000; amax = 100000; // "No limits"
      #if HAS_SOFTWARE_ENDSTOPS
        if (enabled()) switch (axis) {
          case X_AXIS:
            TERN_(MIN_SOFTWARE_ENDSTOP_X, amin = min.x);
            TERN_(MAX_SOFTWARE_ENDSTOP_X, amax = max.x);
            break;
          #if HAS_Y_AXIS
            case Y_AXIS:
              TERN_(MIN_SOFTWARE_ENDSTOP_Y, amin = min.y);
              TERN_(MAX_SOFTWARE_ENDSTOP_Y, amax = max.y);
              break;
          #endif
          #if HAS_Z_AXIS
            case Z_AXIS:
              TERN_(MIN_SOFTWARE_ENDSTOP_Z, amin = min.z);
              TERN_(MAX_SOFTWARE_ENDSTOP_Z, amax = max.z);
              break;
          #endif
          #if HAS_I_AXIS
            case I_AXIS:
              TERN_(MIN_SOFTWARE_ENDSTOP_I, amin = min.i);
              TERN_(MIN_SOFTWARE_ENDSTOP_I, amax = max.i);
              break;
          #endif
          #if HAS_J_AXIS
            case J_AXIS:
              TERN_(MIN_SOFTWARE_ENDSTOP_J, amin = min.j);
              TERN_(MIN_SOFTWARE_ENDSTOP_J, amax = max.j);
              break;
          #endif
          #if HAS_K_AXIS
            case K_AXIS:
              TERN_(MIN_SOFTWARE_ENDSTOP_K, amin = min.k);
              TERN_(MIN_SOFTWARE_ENDSTOP_K, amax = max.k);
              break;
          #endif
          #if HAS_U_AXIS
            case U_AXIS:
              TERN_(MIN_SOFTWARE_ENDSTOP_U, amin = min.u);
              TERN_(MIN_SOFTWARE_ENDSTOP_U, amax = max.u);
              break;
          #endif
          #if HAS_V_AXIS
            case V_AXIS:
              TERN_(MIN_SOFTWARE_ENDSTOP_V, amin = min.v);
              TERN_(MIN_SOFTWARE_ENDSTOP_V, amax = max.v);
              break;
          #endif
          #if HAS_W_AXIS
            case W_AXIS:
              TERN_(MIN_SOFTWARE_ENDSTOP_W, amin = min.w);
              TERN_(MIN_SOFTWARE_ENDSTOP_W, amax = max.w);
              break;
          #endif
          default: break;
        }
      #endif
    }
  } soft_endstops_t;

  extern soft_endstops_t soft_endstop;
  void apply_motion_limits(xyz_pos_t &target);
  void update_software_endstops(const AxisEnum axis
    #if HAS_HOTEND_OFFSET
      , const uint8_t old_tool_index=0, const uint8_t new_tool_index=0
    #endif
  );
  #define SET_SOFT_ENDSTOP_LOOSE(loose) (soft_endstop._loose = loose)

  static bool& soft_endstops_enabled = soft_endstop._enabled; // stub

#else // !HAS_SOFTWARE_ENDSTOPS

  typedef struct {
    bool enabled() { return false; }
    void get_manual_axis_limits(const AxisEnum axis, float &amin, float &amax) {
      // No limits
      amin = current_position[axis] - 1000;
      amax = current_position[axis] + 1000;
    }
  } soft_endstops_t;
  extern soft_endstops_t soft_endstop;
  #define apply_motion_limits(V)        NOOP
  #define update_software_endstops(...) NOOP
  #define SET_SOFT_ENDSTOP_LOOSE(V)     NOOP

#endif // !HAS_SOFTWARE_ENDSTOPS

void report_current_position();

void get_cartesian_from_steppers();
void set_current_from_steppers_for_axis(const AxisEnum axis);
void set_current_from_steppers();

/**
 * sync_plan_position
 *
 * Set the planner/stepper positions directly from current_position with
 * no kinematic translation. Used for homing axes and cartesian/core syncing.
 */
void sync_plan_position();
void sync_plan_position_e();

/**
 * Move the planner to the current position from wherever it last moved
 * (or from wherever it has been told it is located).
 */
void line_to_current_position(const feedRate_t &fr_mm_s=feedrate_mm_s);

/// Plans (non-blocking) linear move to relative distance.
/// It uses prepare_move_to_destination() for the planning which
/// is suitable with UBL.
void plan_move_by(const feedRate_t fr, const float dx, const float dy = 0, const float dz = 0, const float de = 0);

void prepare_move_to_destination();

void _internal_move_to_destination(const feedRate_t &fr_mm_s=0.0f
  #if IS_KINEMATIC
    , const bool is_fast=false
  #endif
);

inline void prepare_internal_move_to_destination(const feedRate_t &fr_mm_s=0.0f) {
  _internal_move_to_destination(fr_mm_s);
}

#if IS_KINEMATIC
  void prepare_fast_move_to_destination(const feedRate_t &scaled_fr_mm_s=MMS_SCALED(feedrate_mm_s));

  inline void prepare_internal_fast_move_to_destination(const feedRate_t &fr_mm_s=0.0f) {
    _internal_move_to_destination(fr_mm_s, true);
  }
#endif

/// Plans (non-blocking) Z-Manhattan fast (non-linear) move to the specified location
/// Feedrate is in mm/s
/// Z-Manhattan: moves XY and Z independently. Raises before or lowers after XY motion.
/// Suitable for Z probing because it does not apply motion limits
/// Uses logical coordinates
void plan_park_move_to(const float rx, const float ry, const float rz, const feedRate_t &fr_xy, const feedRate_t &fr_z);

static inline void plan_park_move_to_xyz(const xyz_pos_t &xyz, const feedRate_t &fr_xy, const feedRate_t &fr_z) {
  plan_park_move_to(xyz.x, xyz.y, xyz.z, fr_xy, fr_z);
}

/**
 * Blocking movement and shorthand functions
 */

/**
 * Performs a blocking fast parking move to (X, Y, Z) and sets the current_position.
 * Parking (Z-Manhattan): Moves XY and Z independently. Raises Z before or lowers Z after XY motion.
 */
void do_blocking_move_to(const float rx, const float ry, const float rz, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to(const xy_pos_t &raw, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to(const xyz_pos_t &raw, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to(const xyze_pos_t &raw, const feedRate_t &fr_mm_s=0.0f);

void do_blocking_move_to_x(const float &rx, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to_y(const float &ry, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to_z(const float &rz, const feedRate_t &fr_mm_s=0.0f);

void do_blocking_move_to_xy(const float &rx, const float &ry, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to_xy(const xy_pos_t &raw, const feedRate_t &fr_mm_s=0.0f);
FORCE_INLINE void do_blocking_move_to_xy(const xyz_pos_t &raw, const feedRate_t &fr_mm_s=0.0f)  { do_blocking_move_to_xy(xy_pos_t(raw), fr_mm_s); }
FORCE_INLINE void do_blocking_move_to_xy(const xyze_pos_t &raw, const feedRate_t &fr_mm_s=0.0f) { do_blocking_move_to_xy(xy_pos_t(raw), fr_mm_s); }

void do_blocking_move_to_xy_z(const xy_pos_t &raw, const float &z, const feedRate_t &fr_mm_s=0.0f);
FORCE_INLINE void do_blocking_move_to_xy_z(const xyz_pos_t &raw, const float &z, const feedRate_t &fr_mm_s=0.0f)  { do_blocking_move_to_xy_z(xy_pos_t(raw), z, fr_mm_s); }
FORCE_INLINE void do_blocking_move_to_xy_z(const xyze_pos_t &raw, const float &z, const feedRate_t &fr_mm_s=0.0f) { do_blocking_move_to_xy_z(xy_pos_t(raw), z, fr_mm_s); }

void remember_feedrate_and_scaling();
void remember_feedrate_scaling_off();
void restore_feedrate_and_scaling();

#if HAS_Z_AXIS
  void do_z_clearance(const_float_t zclear, const bool lower_allowed=false);
#else
  inline void do_z_clearance(float, bool=false) {}
#endif

//
// Homing
//

uint8_t axes_need_homing(uint8_t axis_bits=0x07);
bool axis_unhomed_error(uint8_t axis_bits=0x07);

static inline bool axes_should_home(uint8_t axis_bits=0x07) { return axes_need_homing(axis_bits); }
static inline bool homing_needed_error(uint8_t axis_bits=0x07) { return axis_unhomed_error(axis_bits); }

#if ENABLED(NO_MOTION_BEFORE_HOMING)
  #define MOTION_CONDITIONS (IsRunning() && !axis_unhomed_error())
#else
  #define MOTION_CONDITIONS IsRunning()
#endif

void set_axis_is_at_home(const AxisEnum axis);

void set_axis_is_not_at_home(const AxisEnum axis);

#if ENABLED(PRECISE_HOMING)
  void homeaxis(const AxisEnum axis, const feedRate_t fr_mm_s=0.0, bool invert_home_dir = false, bool can_calibrate = true);
#else
  void homeaxis(const AxisEnum axis, const feedRate_t fr_mm_s=0.0, bool invert_home_dir = false);
#endif

void do_homing_move(const AxisEnum axis, const float distance, const feedRate_t fr_mm_s=0.0
  #if ENABLED(MOVE_BACK_BEFORE_HOMING)
    , bool can_move_back_before_homing = false
  #endif
);

/**
 * Workspace offsets
 */
#if HAS_HOME_OFFSET || HAS_POSITION_SHIFT
  #if HAS_HOME_OFFSET
    extern xyz_pos_t home_offset;
  #endif
  #if HAS_POSITION_SHIFT
    extern xyz_pos_t position_shift;
  #endif
  #if HAS_HOME_OFFSET && HAS_POSITION_SHIFT
    extern xyz_pos_t workspace_offset;
    #define _WS workspace_offset
  #elif HAS_HOME_OFFSET
    #define _WS home_offset
  #else
    #define _WS position_shift
  #endif
  #define NATIVE_TO_LOGICAL(POS, AXIS) ((POS) + _WS[AXIS])
  #define LOGICAL_TO_NATIVE(POS, AXIS) ((POS) - _WS[AXIS])
  FORCE_INLINE void toLogical(xy_pos_t &raw)   { raw += _WS; }
  FORCE_INLINE void toLogical(xyz_pos_t &raw)  { raw += _WS; }
  FORCE_INLINE void toLogical(xyze_pos_t &raw) { raw += _WS; }
  FORCE_INLINE void toNative(xy_pos_t &raw)    { raw -= _WS; }
  FORCE_INLINE void toNative(xyz_pos_t &raw)   { raw -= _WS; }
  FORCE_INLINE void toNative(xyze_pos_t &raw)  { raw -= _WS; }
#else
  #define NATIVE_TO_LOGICAL(POS, AXIS) (POS)
  #define LOGICAL_TO_NATIVE(POS, AXIS) (POS)
  FORCE_INLINE void toLogical(xy_pos_t&)   {}
  FORCE_INLINE void toLogical(xyz_pos_t&)  {}
  FORCE_INLINE void toLogical(xyze_pos_t&) {}
  FORCE_INLINE void toNative(xy_pos_t&)    {}
  FORCE_INLINE void toNative(xyz_pos_t&)   {}
  FORCE_INLINE void toNative(xyze_pos_t&)  {}
#endif
#define LOGICAL_X_POSITION(POS) NATIVE_TO_LOGICAL(POS, X_AXIS)
#define LOGICAL_Y_POSITION(POS) NATIVE_TO_LOGICAL(POS, Y_AXIS)
#define LOGICAL_Z_POSITION(POS) NATIVE_TO_LOGICAL(POS, Z_AXIS)
#define RAW_X_POSITION(POS)     LOGICAL_TO_NATIVE(POS, X_AXIS)
#define RAW_Y_POSITION(POS)     LOGICAL_TO_NATIVE(POS, Y_AXIS)
#define RAW_Z_POSITION(POS)     LOGICAL_TO_NATIVE(POS, Z_AXIS)

/**
 * position_is_reachable family of functions
 */

#if IS_KINEMATIC // (DELTA or SCARA)
  #if HAS_SCARA_OFFSET
    extern abc_pos_t scara_home_offset; // A and B angular offsets, Z mm offset
  #endif

  // Return true if the given point is within the printable area
  inline bool position_is_reachable(const float &rx, const float &ry, const float inset=0) {
    #if ENABLED(DELTA)
      return HYPOT2(rx, ry) <= sq(DELTA_PRINTABLE_RADIUS - inset);
    #elif IS_SCARA
      const float R2 = HYPOT2(rx - SCARA_OFFSET_X, ry - SCARA_OFFSET_Y);
      return (
        R2 <= sq(L1 + L2) - inset
        #if MIDDLE_DEAD_ZONE_R > 0
          && R2 >= sq(float(MIDDLE_DEAD_ZONE_R))
        #endif
      );
    #endif
  }

  inline bool position_is_reachable(const xy_pos_t &pos, const float inset=0) {
    return position_is_reachable(pos.x, pos.y, inset);
  }

  #if HAS_BED_PROBE
    // Return true if the both nozzle and the probe can reach the given point.
    // Note: This won't work on SCARA since the probe offset rotates with the arm.
    inline bool position_is_reachable_by_probe(const float &rx, const float &ry) {
      return position_is_reachable(rx - probe_offset.x, ry - probe_offset.y)
             && position_is_reachable(rx, ry, ABS(MIN_PROBE_EDGE));
    }
  #endif

#else // CARTESIAN

  // Return true if the given position is within the machine bounds.
  inline bool position_is_reachable(const float &rx, const float &ry) {
    if (!WITHIN(ry, Y_MIN_POS - slop, Y_MAX_POS + slop)) return false;
    #if ENABLED(DUAL_X_CARRIAGE)
      if (active_extruder)
        return WITHIN(rx, X2_MIN_POS - slop, X2_MAX_POS + slop);
      else
        return WITHIN(rx, X1_MIN_POS - slop, X1_MAX_POS + slop);
    #else
      return WITHIN(rx, X_MIN_POS - slop, X_MAX_POS + slop);
    #endif
  }
  inline bool position_is_reachable(const xy_pos_t &pos) { return position_is_reachable(pos.x, pos.y); }

  #if HAS_BED_PROBE
    /**
     * Return whether the given position is within the bed, and whether the nozzle
     * can reach the position required to put the probe at the given position.
     *
     * Example: For a probe offset of -10,+10, then for the probe to reach 0,0 the
     *          nozzle must be be able to reach +10,-10.
     */
    inline bool position_is_reachable_by_probe(const float &rx, const float &ry) {
      return position_is_reachable(rx - probe_offset.x, ry - probe_offset.y)
          && WITHIN(rx, probe_min_x() - slop, probe_max_x() + slop)
          && WITHIN(ry, probe_min_y() - slop, probe_max_y() + slop);
    }
  #endif

#endif // CARTESIAN

#if !HAS_BED_PROBE
  FORCE_INLINE bool position_is_reachable_by_probe(const float &rx, const float &ry) { return position_is_reachable(rx, ry); }
#endif
FORCE_INLINE bool position_is_reachable_by_probe(const xy_int_t &pos) { return position_is_reachable_by_probe(pos.x, pos.y); }
FORCE_INLINE bool position_is_reachable_by_probe(const xy_pos_t &pos) { return position_is_reachable_by_probe(pos.x, pos.y); }

/**
 * Duplication mode
 */
#if HAS_DUPLICATION_MODE
  extern bool extruder_duplication_enabled,       // Used in Dual X mode 2
              mirrored_duplication_mode;          // Used in Dual X mode 3
  #if ENABLED(MULTI_NOZZLE_DUPLICATION)
    extern uint8_t duplication_e_mask;
  #endif
#endif

/**
 * Dual X Carriage
 */
#if ENABLED(DUAL_X_CARRIAGE)

  enum DualXMode : char {
    DXC_FULL_CONTROL_MODE,
    DXC_AUTO_PARK_MODE,
    DXC_DUPLICATION_MODE,
    DXC_MIRRORED_MODE
  };

  extern DualXMode dual_x_carriage_mode;
  extern float inactive_extruder_x_pos,           // Used in mode 0 & 1
               duplicate_extruder_x_offset;       // Used in mode 2 & 3
  extern xyz_pos_t raised_parked_position;        // Used in mode 1
  extern bool active_extruder_parked;             // Used in mode 1, 2 & 3
  extern millis_t delayed_move_time;              // Used in mode 1
  extern int16_t duplicate_extruder_temp_offset;  // Used in mode 2 & 3

  FORCE_INLINE bool dxc_is_duplicating() { return dual_x_carriage_mode >= DXC_DUPLICATION_MODE; }

  float x_home_pos(const int extruder);

  FORCE_INLINE int x_home_dir(const uint8_t extruder) { return extruder ? X2_HOME_DIR : X_HOME_DIR; }

#elif ENABLED(MULTI_NOZZLE_DUPLICATION)

  enum DualXMode : char {
    DXC_DUPLICATION_MODE = 2
  };

#else

  #define TOOL_X_HOME_DIR(T) X_HOME_DIR

#endif

#if HAS_M206_COMMAND
  void set_home_offset(const AxisEnum axis, const float v);
#endif

#if USE_SENSORLESS
  struct sensorless_t;
  sensorless_t start_sensorless_homing_per_axis(const AxisEnum axis);
  void end_sensorless_homing_per_axis(const AxisEnum axis, sensorless_t enable_stealth);
#endif

#if ENABLED(PRECISE_HOMING)
  
  /**
  * \returns offset of current position to calibrated safe home position.
  * This includes HOME_GAP. Works for X and Y axes only.
  */
  float calibrated_home_offset(const AxisEnum axis);

  /**
   * Provides precise homing for X and Y axes
   * \param axis axis to be homed (cartesian printers only)
   * \param axis_home_dir direction where the home of the axis is
   * \param can_calibrate allows re-calibration if homing is not successful
   * calibration should be disabled for crash recovery, power loss recovery etc.
   * \return probe offset
   */ 
  float home_axis_precise(AxisEnum axis, int axis_home_dir, bool can_calibrate = true);

#endif // ENABLED(PRECISE_HOMING)
