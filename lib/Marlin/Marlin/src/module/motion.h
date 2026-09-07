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

#include <inplace_function.hpp>
#include <array>
#include <optional>
#include <span>
#include <atomic>

#if HAS_BED_PROBE
  #include "probe.h"
#endif
#include <option/has_toolchanger.h>
#include <option/has_wastebin.h>
#include <option/has_nozzle_thermal_compensation.h>
#if HAS_NOZZLE_THERMAL_COMPENSATION()
  #include <feature/nozzle_thermal_compensation/nozzle_thermal_compensation.hpp>
#endif
#include <tool_index.hpp>
#include <utils/storage/strong_index_array.hpp>

// Axis homed and known-position states
static constexpr uint8_t xyz_bits = _BV(X_AXIS) | _BV(Y_AXIS) | _BV(Z_AXIS);

struct MoveHints {
  /// The move is a printing move and should possibly count into max printed Z
  bool is_printing_move : 1 = false;

  /// Whether extrusion safety checks (PREVENT_COLD_EXTRUSION, PREVENT_LENGTHY_EXTRUDE) should be applied
  bool extrusion_safety_checks : 1 = true;

  /// If set, ignores the e_factor
  bool ignore_e_factor: 1 = false;

  /// The gears are not engaged in filament gear, executing extruder service (switching INDX tools)
  /// This move should avoid filament tracking, auto retract and tool presence check
  bool is_service_extruder_move : 1 = false;
};

/** Holds flags related to configuration and segment generation
 */
struct PrepareMoveHints {
  /// Apply feedrate scaling
  bool scale_feedrate : 1 = true;

  /// Segment the move to be able to append correct leveling values
  bool do_segment : 1 = true;

  /// Whether motion limits should be applied (not allowing moves outside of MIN/MAX coordinates)
  bool apply_motion_limits : 1 = true;

  MoveHints move = {};

};

bool is_xy_in_print_region(const xy_pos_t &xy);

enum class AxisHomeLevel : uint8_t {
  /// The axis it not homed at all, we could be anywhere
  not_homed,

  /// The axis is homed imprecisely (say +- 1mm). Good enough for some operations, not good enough for printing
  imprecise,

  /// The axis is homed as precisely as the printer allows
  full
};

struct AxesHomeLevel : public std::array<AxisHomeLevel, 3> {

public:
  // Inherit parent constructors and assign operators
  using array::array;
  using array::operator=;

  AxesHomeLevel(const array &data) : array(data) {}

  static constexpr array no_axes_homed{AxisHomeLevel::not_homed, AxisHomeLevel::not_homed, AxisHomeLevel::not_homed};

  /// \returns whether a single axis is homed to the required level
  constexpr bool is_homed(AxisEnum axis, AxisHomeLevel required_level) const {
    return at(std::to_underlying(axis)) >= required_level;
  }

  /// \returns whether all axes in the list are homed to the required level
  constexpr inline bool is_homed(std::span<const AxisEnum> axes, AxisHomeLevel required_level) const {
    for(auto axis : axes) {
      if(!is_homed(axis, required_level)) {
        return false;
      }
    }
    return true;
  }

  /// \returns whether all axes in the list are homed to the required level
  constexpr inline bool is_homed(std::initializer_list<AxisEnum> axes, AxisHomeLevel required_level) const {
    return is_homed(std::span(axes), required_level);
  }

  /// \returns whether all axes are homed to a required level
  constexpr bool is_homed(AxisHomeLevel required_level) const {
    return is_homed({X_AXIS, Y_AXIS, Z_AXIS}, required_level);
  }

};

/// To what degree are the individual axes homed
extern AxesHomeLevel axes_home_level;

inline bool all_axes_homed(AxisHomeLevel required_level = AxisHomeLevel::imprecise) { return axes_home_level.is_homed(required_level); }
inline bool all_axes_known(AxisHomeLevel required_level = AxisHomeLevel::imprecise) { return axes_home_level.is_homed(required_level); }

inline void set_all_unhomed() { axes_home_level = AxesHomeLevel::no_axes_homed; }

// Error margin to work around float imprecision
constexpr float slop = 0.0001f;

extern bool relative_mode;

/**
 * Cartesian Current Position
 *   SHOULD be target position of the last queued move in NATIVE coordinates (after hotend offset, before MBL - NativePosTag)
 *   However it is up to the discretion of every caller right now, so it's rather wobbly. Not good.
 *
 *   Use current_machine_position if you want to get current machine position.
 *   Use planner.get_machine_position_mm if you want to get true machine position
 *
 *   Used by 'line_to_current_position' to do a move after changing it.
 *   Used by 'sync_plan_position' to update 'planner.position'.
 *
 * TODO Migrate all writes to set_current_position and make read-only
 */
extern xyze_pos_t current_position;

void set_current_position(const xyze_pos_t &native);

/// SHOULD be target position of the last queued move in MACHINE coordinates (after MBL)
/// SHOULD be ALMOST equal to planner.get_machine_position_mm.
/// There is an exception when a move shorter than MIN_MSTEPS_PER_SEGMENT gets planned.
/// In that case the planner actually does nothing, but current_machine_position changes
/// Also there might be small imprecisions due to to_machine_pos and to_native_pos reversibility
MachinePosXYZE current_machine_position();

// TODO: Get rid of this completely.
[[deprecated("EVIL. DO NOT USE. Use prepare_move_to or any other function that does not rely on static variables.")]]
extern xyze_pos_t destination;       // Destination for a move

#if defined(XY_PROBE_SPEED_INITIAL)
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

extern float homing_bump_divisor[];

/**
 * Feedrate scaling is applied to all G0/G1, G2/G3, and G5 moves
 */
extern int16_t feedrate_percentage;
#define MMS_SCALED(V) ((V) * 0.01f * feedrate_percentage)

// The active extruder (tool). Set with T<extruder> command.
#if EXTRUDERS > 1
  extern std::atomic<uint8_t> active_extruder;
#else
  constexpr std::atomic<uint8_t> active_extruder = 0;
#endif

/**
 * Gets hotend index associated with a given extruder index.
 */
 inline uint8_t hotend_from_extruder([[maybe_unused]] const uint8_t e) {
  #if HOTENDS > 1
    return e;
  #else
    return 0;
  #endif
}

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
  // we keep old array size instead of PhysicalToolIndex::count because of weak indexing (see definition of PhysicalToolIndex::count)
  extern StrongIndexArray<xyz_pos_t, HOTENDS, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> hotend_offset;
  extern xyz_pos_t hotend_currently_applied_offset; // Difference to position without hotend offset. Used for tool park/pickup
  void reset_hotend_offset(PhysicalToolIndex tool_index);
  void reset_hotend_offsets();
#elif HOTENDS
  // we keep old array size instead of PhysicalToolIndex::count because of weak indexing (see definition of PhysicalToolIndex::count)
  constexpr StrongIndexArray<xyz_pos_t, HOTENDS, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> hotend_offset {};
#else
  // #error dead code found by automatic analyses (see BFW-5461)
  constexpr StrongIndexArray<xyz_pos_t, 1, PhysicalToolIndex, PhysicalToolIndex::to_raw_static, strong_index_array::AllowWeakIndexing::yes> hotend_offset {};
#endif

typedef struct { xyz_pos_t min, max; } axis_limits_t;
#if HAS_SOFTWARE_ENDSTOPS
  extern bool soft_endstops_enabled;
  extern axis_limits_t soft_endstop;
  void apply_motion_limits(xyz_pos_t &target);
  inline void apply_motion_limits(xyze_pos_t &target) {
    xyz_pos_t xyz = target.xyz();
    apply_motion_limits(xyz);
    target.set(xyz);
  }
  void update_software_endstops(const AxisEnum axis
    #if HAS_HOTEND_OFFSET
      , const uint8_t old_tool_index=0, const uint8_t new_tool_index=0
    #endif
  );
  #define SET_SOFT_ENDSTOP_LOOSE(loose) NOOP

#else // !HAS_SOFTWARE_ENDSTOPS

  constexpr bool soft_endstops_enabled = false;
  //constexpr axis_limits_t soft_endstop = {
  //  { X_MIN_POS, Y_MIN_POS, Z_MIN_POS },
  //  { X_MAX_POS, Y_MAX_POS, Z_MAX_POS } };
  #define apply_motion_limits(V)    NOOP
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
/// @param e_axis_index Explicit E stepper index — use for service moves (e.g. INDX lever)
///                      that must sync E regardless of tool state.
void sync_plan_position_e(std::optional<uint8_t> e_axis_index = std::nullopt);

/**
 * Move the planner to the current position from wherever it last moved
 * (or from wherever it has been told it is located).
 */
[[deprecated("EVIL! DOES NOT APPLY MBL, EVEN THOUGH IT WORKS WITH CURRENT_POSITION! Use line_to_machine_pos instead")]]
void line_to_current_position(const feedRate_t &fr_mm_s=feedrate_mm_s);

/// Sets @param e as the current extruder position on all layers
void sync_e_position_to(float e);

/// Plans a move to the specified machine coordinates
/// Updates current_position to match the machine position
void line_to_machine_pos(const MachinePosXYZE &target, feedRate_t fr_mm_s, const MoveHints &hints = {});

void line_to_machine_pos(const MachinePosXYZ &target, feedRate_t fr_mm_s, const MoveHints &hints = {});

enum class Segmented {
    yes,
    no,
};

void prepare_internal_move_to_destination(const feedRate_t &fr_mm_s=0.0f, const PrepareMoveHints &hints = {});

/// Plans (non-blocking) Z-Manhattan fast (non-linear) move to the specified location
/// Feedrate is in mm/s
/// Z-Manhattan: moves XY and Z independently. Raises before or lowers after XY motion.
/// Suitable for Z probing because it does not apply motion limits
/// Uses logical coordinates
void plan_park_move_to(const float rx, const float ry, const float rz, const feedRate_t &fr_xy, const feedRate_t &fr_z, Segmented segmented);

static inline void plan_park_move_to_xyz(const xyz_pos_t &xyz, const feedRate_t &fr_xy, const feedRate_t &fr_z, Segmented segmented) {
  plan_park_move_to(xyz.x, xyz.y, xyz.z, fr_xy, fr_z, segmented);
}

/**
 * Blocking movement and shorthand functions
 */

/**
 * Performs a blocking fast parking move to (X, Y, Z) and sets the current_position.
 * Parking (Z-Manhattan): Moves XY and Z independently. Raises Z before or lowers Z after XY motion.
 */
void do_blocking_move_to(const float rx, const float ry, const float rz, const feedRate_t &fr_mm_s=0.0f, Segmented segmented = Segmented::no);
void do_blocking_move_to(const xy_pos_t &raw, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to(const xyz_pos_t &raw, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to(const xyze_pos_t &raw, const feedRate_t &fr_mm_s=0.0f);

void do_blocking_move_to_x(const float &rx, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to_y(const float &ry, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to_z(const float &rz, const feedRate_t &fr_mm_s=0.0f, Segmented segmented = Segmented::no);

void do_blocking_move_to_xy(const float &rx, const float &ry, const feedRate_t &fr_mm_s=0.0f);
void do_blocking_move_to_xy(const xy_pos_t &raw, const feedRate_t &fr_mm_s=0.0f);
FORCE_INLINE void do_blocking_move_to_xy(const xyz_pos_t &raw, const feedRate_t &fr_mm_s=0.0f)  { do_blocking_move_to_xy(raw.xy(), fr_mm_s); }
FORCE_INLINE void do_blocking_move_to_xy(const xyze_pos_t &raw, const feedRate_t &fr_mm_s=0.0f) { do_blocking_move_to_xy(raw.xy(), fr_mm_s); }

void remember_feedrate_and_scaling();
void remember_feedrate_scaling_off();
void restore_feedrate_and_scaling();

#if HAS_Z_AXIS
  uint8_t do_z_clearance(const float zclear, const bool lower_allowed=false);
#else
  // #error dead code found by automatic analyses (see BFW-5461)
  inline uint8_t do_z_clearance(float, bool=false) { return 0; }
#endif

//
// Homing
//

uint8_t axes_need_homing(uint8_t axis_bits=0x07, AxisHomeLevel required_level = AxisHomeLevel::imprecise);
bool axis_unhomed_error(uint8_t axis_bits=0x07, AxisHomeLevel required_level = AxisHomeLevel::imprecise);

static inline bool homing_needed_error(uint8_t axis_bits=0x07) { return axis_unhomed_error(axis_bits); }

void set_axis_is_at_home(const AxisEnum axis, AxisHomeLevel level, bool homing_z_with_probe = true);

void set_axis_is_not_at_home(const AxisEnum axis);

void homing_failed(stdext::inplace_function<void()> fallback_error, bool crash_was_active = false, bool recover_z = false);

// Home a single logical axis
[[nodiscard]] bool homeaxis(const AxisEnum axis, const feedRate_t fr_mm_s=0.0, bool invert_home_dir = false,
  void (*enable_wavetable)(AxisEnum) = NULL, bool can_calibrate = true, bool homing_z_with_probe = true, bool throw_homing_failed = true);

struct HomeAxisSingleRunArgs {
  AxisEnum axis;
  int axis_home_dir;
  feedRate_t fr_mm_s = 0;
  int attempt = 0;
  bool invert_home_dir : 1 = false;
  bool homing_z_with_probe : 1 = true;
};

// Perform a single homing probe on a logical axis
float homeaxis_single_run(const HomeAxisSingleRunArgs &args);

[[deprecated("Use the HomeAxisSingleRunArgs overload")]]
inline float homeaxis_single_run(const AxisEnum axis, const int axis_home_dir, const feedRate_t fr_mm_s = 0.0,
  bool invert_home_dir = false, bool homing_z_with_probe = true, const int attempt = 0) {
    return homeaxis_single_run(HomeAxisSingleRunArgs{
      .axis = axis,
      .axis_home_dir = axis_home_dir,
      .fr_mm_s = fr_mm_s,
      .attempt = attempt,
      .invert_home_dir = invert_home_dir,
      .homing_z_with_probe = homing_z_with_probe,
    });
}

/**
 * @brief Perform a blocking, relative move on the specified axis *without* position modifiers
 * @param axis Axis to move
 * @param distance Distance relative to current position
 * @param fr_mm_s Move feedrate
 * @warning Does not set up the printer for the homing! (endstops enable, stallguards enable, ...) Use do_homing_move instead.
 * @returns endstops.trigger_state()
 */
[[nodiscard]] uint8_t do_homing_move_axis_rel(const AxisEnum axis, const float distance, const feedRate_t fr_mm_s);

/// Perform a single homing move on a logical axis
uint8_t do_homing_move(const AxisEnum axis, const float distance, const feedRate_t fr_mm_s=0.0, bool can_move_back_before_homing = false, bool homing_z_with_probe = true);

/// Prepares the move to the target. Can apply segmentation based on MBL and other mechanisms requirements.
void prepare_move_to(xyze_pos_t target, feedRate_t fr_mm_s, PrepareMoveHints hints);

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
    // #error dead code found by automatic analyses (see BFW-5461)
    #define _WS home_offset
  #else
    // #error dead code found by automatic analyses (see BFW-5461)
    #define _WS position_shift
  #endif
#endif

inline xyz_pos_t native_logical_offset() {
  xyz_pos_t result { 0, 0, 0 };
#if HAS_HOME_OFFSET || HAS_POSITION_SHIFT
  result += _WS;
#endif
#if HAS_TOOLCHANGER()
  result += hotend_currently_applied_offset;
#endif
#if HAS_NOZZLE_THERMAL_COMPENSATION()
  // Subtracted so that toNative() below *raises* the machine Z of a g-code target as the nozzle
  // grows past the length the probe results were normalized to, keeping the tip where it was
  // asked for. Only g-code positions cross this transform, which is what keeps the term out of
  // firmware-internal ones - park heights and dock coordinates are carriage positions.
  result.z -= buddy::nozzle_thermal_compensation::current_elongation_vs_reference_mm();
#endif
  return result;
}

template<size_t s>
[[nodiscard]] auto toLogical(const Vector<float, NativePosTag, s> &v) {
  return (v + native_logical_offset()).template to_tag<LogicalPosTag>();
}

template<size_t s>
[[nodiscard]] auto toNative(const Vector<float, LogicalPosTag, s> &v) {
  return v.template to_tag<NativePosTag>() - native_logical_offset();
}

/// Transforms the native position to machine position (applies MBL)
MachinePosXYZ to_machine_pos(const xyz_pos_t &pos);

/// Transform a machine position to a native position (unapplies MBL)
xyz_pos_t to_native_pos(const MachinePosXYZ &pos);

inline MachinePosXYZE to_machine_pos(const xyze_pos_t &pos) {
    MachinePosXYZE result = pos;
    result.set(to_machine_pos(pos.xyz())); // Only change the XYZ coordinates
    return result;
}

inline xyze_pos_t to_native_pos(const MachinePosXYZE &pos) {
    xyze_pos_t result = pos;
    result.set(to_native_pos(pos.xyz())); // Only change the XYZ coordinates
    return result;
}

#undef _WS

/**
 * position_is_reachable family of functions
 */

// Return true if the given position is within the machine bounds.
inline bool position_is_reachable(const float &rx, const float &ry) {
  if (!WITHIN(ry, Y_MIN_POS - slop, Y_MAX_POS + slop)) return false;
  return WITHIN(rx, X_MIN_POS - slop, X_MAX_POS + slop);
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
    return position_is_reachable(rx - probe_offset.x - TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.x), ry - probe_offset.y - TERN0(HAS_HOTEND_OFFSET, hotend_currently_applied_offset.y))
        && WITHIN(rx, probe_min_x() - slop, probe_max_x() + slop)
        && WITHIN(ry, probe_min_y() - slop, probe_max_y() + slop);
  }
#endif

#if !HAS_BED_PROBE
  FORCE_INLINE bool position_is_reachable_by_probe(const float &rx, const float &ry) { return position_is_reachable(rx, ry); }
#endif
FORCE_INLINE bool position_is_reachable_by_probe(const xy_int_t &pos) { return position_is_reachable_by_probe(pos.x, pos.y); }
FORCE_INLINE bool position_is_reachable_by_probe(const xy_pos_t &pos) { return position_is_reachable_by_probe(pos.x, pos.y); }

#if HAS_M206_COMMAND
  void set_home_offset(const AxisEnum axis, const float v);
#endif

#if USE_SENSORLESS
  struct sensorless_t;
  sensorless_t start_sensorless_homing_per_axis(const AxisEnum axis);
  void end_sensorless_homing_per_axis(const AxisEnum axis, sensorless_t enable_stealth);
#endif

#if ENABLED(ARC_SUPPORT)
  /**
   * Plan an arc in 2 dimensions, with linear motion in the other axes.
   * The arc is traced with many small linear segments according to the configuration.
   */
  void plan_arc(
    const xyze_pos_t &cart,   // Destination position
    const xy_float_t &offset, // Center of rotation relative to current_position
    const bool clockwise,     // Clockwise?
    const uint8_t circles,    // Number of full circles to perform
    bool last_segment=false   // Mark last segment of the arc as terminating
  );
#endif
