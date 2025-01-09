/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * planner.h
 *
 * Buffer movement commands and manage the acceleration profile plan
 *
 * Derived from Grbl
 * Copyright (c) 2009-2011 Simen Svale Skogsrud
 */

#include "../Marlin.h"

#include "motion.h"
#include "../gcode/queue.h"
#include "../feature/precise_stepping/precise_stepping.hpp"
#include "../feature/phase_stepping/phase_stepping.hpp"

// Value by which steps are multiplied to increase the precision of the Planner.
constexpr const int PLANNER_STEPS_MULTIPLIER = 4;

#if ENABLED(FWRETRACT)
  #include "../feature/fwretract.h"
#endif

// from stepper/trinamic.h , avoiding include loop
extern uint16_t stepper_microsteps(const AxisEnum axis, uint16_t new_microsteps);
extern uint16_t stepper_mscnt(const AxisEnum axis);

// Feedrate for manual moves
#ifdef MANUAL_FEEDRATE
  constexpr xyze_feedrate_t manual_feedrate_mm_m = MANUAL_FEEDRATE;
#endif

/**
 * Planner block flags as boolean bit fields
 */
enum BlockFlagBit : uint8_t {
  // Recalculate trapezoids on entry junction. For optimization.
  BLOCK_BIT_RECALCULATE,

  // Nominal speed always reached.
  // i.e., The segment is long enough, so the nominal speed is reachable if accelerating
  // from a safe speed (in consideration of jerking from zero speed).
  BLOCK_BIT_NOMINAL_LENGTH,

  // The block is segment 2+ of a longer move
  BLOCK_BIT_CONTINUED,

  // Sync the stepper counts from the block
  BLOCK_BIT_SYNC_POSITION,

  // Indicate that PreciseStepping already processed the block, but it still has to be inside
  // the block queue until the last step event from this block will be processed in
  // PreciseStepping::process_one_step_event_from_queue().
  BLOCK_BIT_PROCESSED
};

/**
 * Planner block flags as boolean bit fields
 */
typedef struct {
  union {
    uint8_t bits;

    struct {
      bool recalculate:1;
      bool nominal_length:1;
      bool continued:1;
      bool sync_position:1;
      bool raw_block:1;
    };
  };

  void clear() volatile { bits = 0; }
  void apply(const uint8_t f) volatile { bits |= f; }
  void apply(const BlockFlagBit b) volatile { SBI(bits, b); }
  void reset(const BlockFlagBit b) volatile { bits = _BV(b); }
  void set_nominal(const bool n) volatile { recalculate = true; if (n) nominal_length = true; }

} block_flags_t;

/**
 * struct block_t
 *
 * A single entry in the planner buffer.
 * Tracks linear movement over multiple axes.
 *
 * The "nominal" values are as-specified by G-code, and
 * may never actually be reached due to acceleration limits.
 */
typedef struct PlannerBlock {

  volatile block_flags_t flag;              // Block flags (modified by planner)
  volatile bool busy;                       // Busy flag (modified by stepper)

  bool is_sync() { return flag.sync_position; }
  bool is_move() { return !is_sync(); }

  #if EXTRUDERS > 1
    uint8_t extruder;                       // The extruder to move (if E move)
  #else
    static constexpr uint8_t extruder = 0;
  #endif

  uint8_t direction_bits;                   // The direction bit set for this block (refers to *_DIRECTION_BIT in config.h)

  #if FAN_COUNT > 0
    uint8_t fan_speed[FAN_COUNT];
  #endif

  // Fields used by the motion planner to manage acceleration
  float nominal_speed,                      // The nominal speed for this block in (mm/sec)
        entry_speed_sqr,                    // Entry speed at previous-current junction in (mm/sec)^2
        max_entry_speed_sqr,                // Maximum allowable junction entry speed in (mm/sec)^2
        millimeters,                        // The total travel of this block in mm
        acceleration;                       // acceleration mm/sec^2

  union {
    abce_ulong_t msteps;                    // Mini-step count along each axis
    abce_long_t sync_step_position;         // Absolute step counts to set when this sync block is executed
  };
  uint32_t mstep_event_count;               // The number of mini-step events required to complete this block

  #if ENABLED(S_CURVE_ACCELERATION)
    uint32_t cruise_rate,                   // The actual cruise rate to use, between end of the acceleration phase and start of deceleration phase
             acceleration_time,             // Acceleration time and deceleration time in STEP timer counts
             deceleration_time,
             acceleration_time_inverse,     // Inverse of acceleration and deceleration periods, expressed as integer. Scale depends on CPU being used
             deceleration_time_inverse;

    uint32_t nominal_rate,                  // The nominal step rate for this block in step_events/sec
             initial_rate,                  // The jerk-adjusted step rate at start of block
             final_rate,                    // The minimal rate at exit
             acceleration_msteps_per_s2;    // acceleration mini-steps/sec^2
  #endif

  float initial_speed,                      // The jerk-adjusted spped at start of block
        final_speed;                        // The minimal speed at exit of block

  void reset() { memset((char*)this, 0, sizeof(*this)); }

} block_t;

#define BLOCK_MOD(n) ((n)&(BLOCK_BUFFER_SIZE-1))

typedef struct {
   uint32_t max_acceleration_mm_per_s2[XYZE_N], // (mm/s^2) M201 XYZE
            min_segment_time_us;                // (µs) M205 B
      float axis_steps_per_mm[XYZE_N];          // (steps) M92 XYZE - Steps per millimeter
      float axis_msteps_per_mm[XYZE_N];         // (mini-steps) Steps per millimeter multiplied PLANNER_STEPS_MULTIPLIER to increase the Planner resolution.
 feedRate_t max_feedrate_mm_s[XYZE_N];          // (mm/s) M203 XYZE - Max speeds
      float acceleration,                       // (mm/s^2) M204 S - Normal acceleration. DEFAULT ACCELERATION for all printing moves.
            retract_acceleration,               // (mm/s^2) M204 R - Retract acceleration. Filament pull-back and push-forward while standing still in the other axes
            travel_acceleration;                // (mm/s^2) M204 T - Travel acceleration. DEFAULT ACCELERATION for all NON printing moves.
 feedRate_t min_feedrate_mm_s,                  // (mm/s) M205 S - Minimum linear feedrate
            min_travel_feedrate_mm_s;           // (mm/s) M205 T - Minimum travel feedrate

  #if HAS_CLASSIC_JERK
    #if HAS_LINEAR_E_JERK
      xyz_pos_t max_jerk;              // (mm/s^2) M205 XYZ - The largest speed change requiring no acceleration.
    #else
      xyze_pos_t max_jerk;             // (mm/s^2) M205 XYZE - The largest speed change requiring no acceleration.
    #endif
  #endif
} planner_settings_t;

/// Subclass to enforce that people are using user settings when applying settings
struct user_planner_settings_t : public planner_settings_t {};

// Structure for saving/loading movement parameters
typedef struct {
   uint32_t max_acceleration_mm_per_s2[XYZE_N], // (mm/s^2) M201 XYZE
            min_segment_time_us;                // (µs) M205 B
 feedRate_t max_feedrate_mm_s[XYZE_N];          // (mm/s) M203 XYZE - Max speeds
      float acceleration,                       // (mm/s^2) M204 S - Normal acceleration. DEFAULT ACCELERATION for all printing moves.
            retract_acceleration,               // (mm/s^2) M204 R - Retract acceleration. Filament pull-back and push-forward while standing still in the other axes
            travel_acceleration;                // (mm/s^2) M204 T - Travel acceleration. DEFAULT ACCELERATION for all NON printing moves.
 feedRate_t min_feedrate_mm_s,                  // (mm/s) M205 S - Minimum linear feedrate
            min_travel_feedrate_mm_s;           // (mm/s) M205 T - Minimum travel feedrate

  #if DISABLED(CLASSIC_JERK)
    float junction_deviation_mm;                // (mm) M205 J
  #endif
  #if HAS_CLASSIC_JERK
    #if HAS_LINEAR_E_JERK
      xyz_pos_t max_jerk;                       // (mm/s^2) M205 XYZ - The largest speed change requiring no acceleration.
    #else
      xyze_pos_t max_jerk;                      // (mm/s^2) M205 XYZE - The largest speed change requiring no acceleration.
    #endif
  #endif
} motion_parameters_t;

#if DISABLED(SKEW_CORRECTION)
  #define XY_SKEW_FACTOR 0
  #define XZ_SKEW_FACTOR 0
  #define YZ_SKEW_FACTOR 0
#endif

typedef struct {
  #if ENABLED(SKEW_CORRECTION_GCODE)
    float xy;
    #if ENABLED(SKEW_CORRECTION_FOR_Z)
      float xz, yz;
    #else
      const float xz = XZ_SKEW_FACTOR, yz = YZ_SKEW_FACTOR;
    #endif
  #else
    const float xy = XY_SKEW_FACTOR,
                xz = XZ_SKEW_FACTOR, yz = YZ_SKEW_FACTOR;
  #endif
} skew_factor_t;

#if ENABLED(ARC_SUPPORT)
  #define HINTS_CURVE_RADIUS
// @hejllukas: Disabled because it contains a significant issue causing that the entry speed is calculated incorrectly.
// #define HINTS_SAFE_EXIT_SPEED
#endif

struct PlannerHints {
  float millimeters = 0.0;            // Move Length, if known, else 0.
  #if ENABLED(HINTS_CURVE_RADIUS)
    float curve_radius = 0.0;         // Radius of curvature of the motion path - to calculate cornering speed
  #else
    static constexpr float curve_radius = 0.0;
  #endif
  #if ENABLED(HINTS_SAFE_EXIT_SPEED)
    float safe_exit_speed_sqr = 0.0;  // Square of the speed considered "safe" at the end of the segment
                                      // i.e., at or below the exit speed of the segment that the planner
                                      // would calculate if it knew the as-yet-unbuffered path
  #endif
  bool raw_block = false;             // Enqueue block without further modifications
};

class Planner {
  public:

    /**
     * The move buffer, calculated in stepper steps
     *
     * block_buffer is a ring buffer...
     *
     *             head,tail : indexes for write,read
     *            head==tail : the buffer is empty
     *            head!=tail : blocks are in the buffer
     *   head==(tail-1)%size : the buffer is full
     *
     *  Writer of head is Planner::buffer_segment().
     *  Reader of tail is Stepper::isr(). Always consider tail busy / read-only
     */
    static block_t block_buffer[BLOCK_BUFFER_SIZE];
    static volatile uint8_t block_buffer_head,        // Index of the next block to be pushed
                            block_buffer_nonbusy,     // Index of the first non busy block
                            block_buffer_planned,     // Index of the optimally planned block
                            block_buffer_tail;        // Index of the busy block, if any
    static uint8_t delay_before_delivering;           // This counter delays delivery of blocks when queue becomes empty to allow the opportunity of merging blocks


    #if ENABLED(DISTINCT_E_FACTORS)
      static uint8_t last_extruder;                 // Respond to extruder change
    #endif

    #if EXTRUDERS
      static int16_t flow_percentage[EXTRUDERS];    // Extrusion factor for each extruder
      static float e_factor[EXTRUDERS];             // The flow percentage and volumetric multiplier combine to scale E movement
    #endif

    #if DISABLED(NO_VOLUMETRICS)
      static float filament_size[EXTRUDERS],          // diameter of filament (in millimeters), typically around 1.75 or 2.85, 0 disables the volumetric calculations for the extruder
                   volumetric_area_nominal,           // Nominal cross-sectional area
                   volumetric_multiplier[EXTRUDERS];  // Reciprocal of cross-sectional area of filament (in mm^2). Pre-calculated to reduce computation in the planner
                                                      // May be auto-adjusted by a filament width sensor
    #endif

    /// Reference to working_settings - settings with applied limits
    static const planner_settings_t &settings;

    /// Reference to user_settings - settings before limits are applied
    static const user_planner_settings_t &user_settings;

    /// Sets new settings for the planner.
    /// Writes the settings raw to user_settings, and with limits applied to settings/working_settings
    /// !!! Always base your settings on user_settings, not on settings
    static void apply_settings(const user_planner_settings_t &settings);
    
    static void set_stealth_mode(bool set);

    static uint32_t max_acceleration_msteps_per_s2[XYZE_N]; // (mini-steps/s^2) Derived from mm_per_s2
    static float mm_per_step[XYZE_N];                       // Millimeters per step
    static float mm_per_half_step[XYZE_N];                  // Millimeters per half step
    static float mm_per_mstep[XYZE_N];                      // Millimeters per mini-step

    #if DISABLED(CLASSIC_JERK)
      static float junction_deviation_mm;       // (mm) M205 J
    #endif

    #if HAS_LEVELING
      static bool leveling_active;          // Flag that bed leveling is enabled
      #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
        static float z_fade_height, inverse_z_fade_height;
      #endif
    #else
      static constexpr bool leveling_active = false;
    #endif

    static xyze_pos_t position_float;

    xyze_long_t get_position_msteps() const { return position; };

    static skew_factor_t skew_factor;

    #if ENABLED(SD_ABORT_ON_ENDSTOP_HIT)
      static bool abort_on_endstop_hit;
    #endif

    /**
     * The current position of the tool in absolute mini-steps
     * Recalculated if any axis_steps_per_mm or axis_msteps_per_mm are changed by gcode
     */
    static xyze_long_t position;

    /// Increased with every quick stop
    /// Useful for tracking whether a quick stop occured during a procedure
    static uint32_t quick_stop_count;

  private:
    /**
     * Speed of previous path line segment
     */
    static xyze_float_t previous_speed;

    /**
     * Nominal speed of previous path line segment (mm/s)^2
     */
    static float previous_nominal_speed;

    /**
     * Limit where 64bit math is necessary for acceleration calculation
     */
    static uint32_t cutoff_long;

    #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
      static float last_fade_z;
    #endif

    #if ENABLED(DISABLE_INACTIVE_EXTRUDER)
      /**
       * Counters to manage disabling inactive extruders
       */
      static uint8_t g_uc_extruder_last_move[EXTRUDERS];
    #endif // DISABLE_INACTIVE_EXTRUDER

    #ifdef XY_FREQUENCY_LIMIT
      // Used for the frequency limit
      #define MAX_FREQ_TIME_US (uint32_t)(1000000.0 / XY_FREQUENCY_LIMIT)
      // Old direction bits. Used for speed calculations
      static unsigned char old_direction_bits;
      // Segment times (in µs). Used for speed calculations
      static xy_ulong_t axis_segment_time_us[3];
    #endif

    // A flag to drop queuing of blocks and abort any pending move
    static bool draining_buffer;

    // A flag to indicate that that buffer is being emptied intentionally
    static bool emptying_buffer;

  public:

    /**
     * Instance Methods
     */

    void init();

    /**
     * Static (class) Methods
     */

    // Recalculate steps/s^2 accelerations based on mm/s^2 settings
    static void refresh_acceleration_rates();

    /**
     * Recalculate 'position', 'mm_per_step', 'mm_per_half_step' and 'mm_per_mstep'.
     * Must be called whenever settings.axis_steps_per_mm or settings.axis_msteps_per_mm changes!
     */
    static void refresh_positioning();

    #if ENABLED(DISTINCT_E_FACTORS)
      static void refresh_e_positioning(const uint8_t extruder=active_extruder);
    #endif

    static void set_max_acceleration(const uint8_t axis, float targetValue);
    static void set_max_feedrate(const uint8_t axis, float targetValue);
    static void set_max_jerk(const AxisEnum axis, float targetValue);

    // TERMINOLOGY (derived from TMC2130):
    // nstep ... minimal configurable microstep, always = 1/256 full step
    // ustep ... currently configured microstep, e.g. 1/8 full step when stepper_microsteps() = 8
    // qstep ... 4 full steps, i.e. = 1024 nstep
    // note: stepper_mscnt() returns nstep position inside current qstep (0..1023)
    FORCE_INLINE static uint32_t nsteps_per_qstep(const AxisEnum axis) { (void)axis; return 1024; }
    FORCE_INLINE static uint32_t nsteps_per_ustep(const AxisEnum axis) { return (nsteps_per_qstep(axis) / (uint32_t)(stepper_microsteps(axis, 0) * 4)); }
    FORCE_INLINE static uint32_t usteps_per_qstep(const AxisEnum axis) { return (nsteps_per_qstep(axis) / nsteps_per_ustep(axis)); }
    FORCE_INLINE static float mm_per_qsteps(const AxisEnum axis, uint32_t qsteps) { return ((float)(usteps_per_qstep(axis) * qsteps)) * mm_per_step[axis]; }
    FORCE_INLINE static float qsteps_per_mm(const AxisEnum axis) { return (settings.axis_steps_per_mm[axis] / (float)usteps_per_qstep(axis)); }
    FORCE_INLINE static float distance_to_stepper_zero(const AxisEnum axis, bool inverted_dir) { return mm_per_qsteps(axis, inverted_dir ? nsteps_per_qstep(axis) - stepper_mscnt(axis) : stepper_mscnt(axis)) / (float)nsteps_per_qstep(axis); }

    #if EXTRUDERS
      FORCE_INLINE static void refresh_e_factor(const uint8_t e) {
        e_factor[e] = (flow_percentage[e] * 0.01f
          #if DISABLED(NO_VOLUMETRICS)
            * volumetric_multiplier[e]
          #endif
        );
      }
    #endif

    // Manage fans, paste pressure, etc.
    static void check_axes_activity();

    // Update multipliers based on new diameter measurements
    static void calculate_volumetric_multipliers();

    #if ENABLED(FILAMENT_WIDTH_SENSOR)
      void apply_filament_width_sensor(const int8_t encoded_ratio);

      static inline float volumetric_percent(const bool vol) {
        return 100.0f * (vol
            ? volumetric_area_nominal / volumetric_multiplier[FILAMENT_SENSOR_EXTRUDER_NUM]
            : volumetric_multiplier[FILAMENT_SENSOR_EXTRUDER_NUM]
        );
      }
    #endif

    #if DISABLED(NO_VOLUMETRICS)

      FORCE_INLINE static void set_filament_size(const uint8_t e, const_float_t v) {
        filament_size[e] = v;
        // make sure all extruders have some sane value for the filament size
        for (uint8_t i = 0; i < COUNT(filament_size); i++)
          if (!filament_size[i]) filament_size[i] = DEFAULT_NOMINAL_FILAMENT_DIA;
      }

    #endif

    #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)

      /**
       * Get the Z leveling fade factor based on the given Z height,
       * re-calculating only when needed.
       *
       *  Returns 1.0 if planner.z_fade_height is 0.0.
       *  Returns 0.0 if Z is past the specified 'Fade Height'.
       */
      static inline float fade_scaling_factor_for_z(const_float_t rz) {
        static float z_fade_factor = 1;
        if (!z_fade_height) return 1;
        if (rz >= z_fade_height) return 0;
        if (last_fade_z != rz) {
          last_fade_z = rz;
          z_fade_factor = 1 - rz * inverse_z_fade_height;
        }
        return z_fade_factor;
      }

      FORCE_INLINE static void force_fade_recalc() { last_fade_z = -999.999f; }

      FORCE_INLINE static void set_z_fade_height(const_float_t zfh) {
        z_fade_height = zfh > 0 ? zfh : 0;
        inverse_z_fade_height = RECIPROCAL(z_fade_height);
        force_fade_recalc();
      }

      FORCE_INLINE static bool leveling_active_at_z(const_float_t rz) {
        return !z_fade_height || rz < z_fade_height;
      }

    #else

      FORCE_INLINE static float fade_scaling_factor_for_z(const_float_t) { return 1; }

      FORCE_INLINE static bool leveling_active_at_z(const_float_t) { return true; }

    #endif

    #if ENABLED(SKEW_CORRECTION)

      FORCE_INLINE static void skew(float &cx, float &cy, const_float_t cz) {
        if (WITHIN(cx, X_MIN_POS + 1, X_MAX_POS) && WITHIN(cy, Y_MIN_POS + 1, Y_MAX_POS)) {
          const float sx = cx - cy * skew_factor.xy - cz * (skew_factor.xz - (skew_factor.xy * skew_factor.yz)),
                      sy = cy - cz * skew_factor.yz;
          if (WITHIN(sx, X_MIN_POS, X_MAX_POS) && WITHIN(sy, Y_MIN_POS, Y_MAX_POS)) {
            cx = sx; cy = sy;
          }
        }
      }
      FORCE_INLINE static void skew(xyz_pos_t &raw) { skew(raw.x, raw.y, raw.z); }

      FORCE_INLINE static void unskew(float &cx, float &cy, const_float_t cz) {
        if (WITHIN(cx, X_MIN_POS, X_MAX_POS) && WITHIN(cy, Y_MIN_POS, Y_MAX_POS)) {
          const float sx = cx + cy * skew_factor.xy + cz * skew_factor.xz,
                      sy = cy + cz * skew_factor.yz;
          if (WITHIN(sx, X_MIN_POS, X_MAX_POS) && WITHIN(sy, Y_MIN_POS, Y_MAX_POS)) {
            cx = sx; cy = sy;
          }
        }
      }
      FORCE_INLINE static void unskew(xyz_pos_t &raw) { unskew(raw.x, raw.y, raw.z); }

    #endif // SKEW_CORRECTION

    #if HAS_LEVELING
      /**
       * Apply leveling to transform a cartesian position
       * as it will be given to the planner and steppers.
       */
      static void apply_leveling(xyz_pos_t &raw);
      static void unapply_leveling(xyz_pos_t &raw);
      FORCE_INLINE static void force_unapply_leveling(xyz_pos_t &raw) {
        leveling_active = true;
        unapply_leveling(raw);
        leveling_active = false;
      }
    #endif

    #if ENABLED(FWRETRACT)
      static void apply_retract(float &rz, float &e);
      FORCE_INLINE static void apply_retract(xyze_pos_t &raw) { apply_retract(raw.z, raw.e); }
      static void unapply_retract(float &rz, float &e);
      FORCE_INLINE static void unapply_retract(xyze_pos_t &raw) { unapply_retract(raw.z, raw.e); }
    #endif

    #if HAS_POSITION_MODIFIERS
      FORCE_INLINE static void apply_modifiers(xyze_pos_t &pos
        #if HAS_LEVELING
          , bool leveling =
          #if PLANNER_LEVELING
            true
          #else
            false
          #endif
        #endif
      ) {
        #if ENABLED(SKEW_CORRECTION)
          skew(pos);
        #endif
        #if HAS_LEVELING
          if (leveling) apply_leveling(pos);
        #endif
        #if ENABLED(FWRETRACT)
          apply_retract(pos);
        #endif
      }

      FORCE_INLINE static void unapply_modifiers(xyze_pos_t &pos
        #if HAS_LEVELING
          , bool leveling =
          #if PLANNER_LEVELING
            true
          #else
            false
          #endif
        #endif
      ) {
        #if ENABLED(FWRETRACT)
          unapply_retract(pos);
        #endif
        #if HAS_LEVELING
          if (leveling) unapply_leveling(pos);
        #endif
        #if ENABLED(SKEW_CORRECTION)
          unskew(pos);
        #endif
      }
    #endif // HAS_POSITION_MODIFIERS

    // Number of moves currently in the planner including the busy block, if any
    FORCE_INLINE static uint8_t movesplanned() { return BLOCK_MOD(block_buffer_head - block_buffer_tail); }

    // Number of blocks already processed by PreciseStepping and now there are just waiting to be discarded.
    FORCE_INLINE static uint8_t movesplanned_processed() { return movesplanned() - nonbusy_movesplanned(); }

    // Number of nonbusy moves currently in the planner
    FORCE_INLINE static uint8_t nonbusy_movesplanned() { return BLOCK_MOD(block_buffer_head - block_buffer_nonbusy); }

    // Remove all blocks from the buffer
    FORCE_INLINE static void clear_block_buffer() { block_buffer_nonbusy = block_buffer_planned = block_buffer_head = block_buffer_tail = 0; }

    // Check if movement queue is full
    FORCE_INLINE static bool is_full() { return block_buffer_tail == next_block_index(block_buffer_head); }

    // Set emptying buffer
    FORCE_INLINE static void set_emptying_buffer(bool b) { emptying_buffer = b; }

    // Get count of movement slots free
    FORCE_INLINE static uint8_t moves_free() { return BLOCK_BUFFER_SIZE - 1 - movesplanned(); }

    /**
     * Planner::get_next_free_block
     *
     * - Get the next head indices (passed by reference)
     * - Wait for the number of spaces to open up in the planner
     * - Return the first head block, or nullptr if the queue is being drained
     */
    FORCE_INLINE static block_t* get_next_free_block(uint8_t &next_buffer_head, const uint8_t count=1) {

      // Wait until there are enough slots free or if aborting
      while (moves_free() < count && !draining_buffer) { idle(true); }
      if (draining_buffer || PreciseStepping::stopping())
        return nullptr;

      // Return the first available block
      next_buffer_head = next_block_index(block_buffer_head);
      return &block_buffer[block_buffer_head];
    }

    /**
     * Planner::_buffer_msteps_raw
     *
     * Add a new linear movement to the buffer (in terms of steps) without implicit kinematic
     * translation, compensation or queuing restrictions.
     *
     *  target      - target position in mini-steps units
     *  fr_mm_s     - (target) speed of the move
     *  extruder    - target extruder
     *  hints       - parameters to aid planner calculations
     *
     * Returns true if movement was buffered, false otherwise
     */
    static bool _buffer_msteps_raw(const xyze_long_t &target, const xyze_pos_t &target_float
      , feedRate_t fr_mm_s, const uint8_t extruder, const PlannerHints &hints=PlannerHints()
    );

    /**
     * Planner::_buffer_msteps
     *
     * Add a new linear movement to the buffer (in terms of steps).
     *
     *  target      - target position in mini-steps units
     *  fr_mm_s     - (target) speed of the move
     *  extruder    - target extruder
     *  hints       - parameters to aid planner calculations
     *
     * Returns true if movement was buffered, false otherwise
     */
    static bool _buffer_msteps(const xyze_long_t &target, const xyze_pos_t &target_float
      , feedRate_t fr_mm_s, const uint8_t extruder, const PlannerHints &hints
    );

    static bool buffer_raw_block(const xyze_long_t &target, const xyze_pos_t &target_float,
        float acceleration, float nominal_speed, float entry_speed, float exit_speed, uint8_t extruder);

    /**
     * @brief Populate a block in preparation for insertion
     * @details Populate the fields of a new linear movement block
     *          that will be added to the queue and processed soon
     *          by the Stepper ISR.
     *
     * @param block         A block to populate
     * @param target        Target position in mini-steps units
     * @param target_float  Target position in native mm
     * @param fr_mm_s       (target) speed of the move
     * @param extruder      target extruder
     * @param hints         parameters to aid planner calculations
     *
     * @return  true if movement is acceptable, false otherwise
     */
    static bool _populate_block(block_t * const block,
        const xyze_long_t &target, const xyze_pos_t &target_float
      , feedRate_t fr_mm_s, const uint8_t extruder, const PlannerHints &hints
    );

    static bool populate_raw_block(block_t *const block, const abce_long_t &target,
        const xyze_pos_t &target_float, float acceleration, float nominal_speed,
        float entry_speed, float exit_speed, uint8_t extruder);

    /**
     * Planner::buffer_sync_block
     * Add a block to the buffer that just updates the position
     */
    static void buffer_sync_block();

    /**
     * Planner::buffer_segment
     *
     * Add a new linear movement to the buffer in axis units.
     *
     * Leveling and kinematics should be applied ahead of calling this.
     *
     *  a,b,c,e     - target positions in mm and/or degrees
     *  fr_mm_s     - (target) speed of the move
     *  extruder    - optional target extruder (otherwise active_extruder)
     *  hints       - optional parameters to aid planner calculations
     */
    static bool buffer_segment(const abce_pos_t &abce
      , const_feedRate_t fr_mm_s
      , const uint8_t extruder=active_extruder
      , const PlannerHints &hints=PlannerHints()
    );

    static bool buffer_raw_segment(const abce_pos_t &abce, float acceleration, float nominal_speed,
        float entry_speed, float exit_speed, uint8_t extruder);

  public:

    /**
     * Add a new linear movement to the buffer.
     * The target is cartesian.
     *
     *  cart         - target position in mm or degrees
     *  fr_mm_s      - (target) speed of the move (mm/s)
     *  extruder     - optional target extruder (otherwise active_extruder)
     *  hints        - optional parameters to aid planner calculations
     */
    static bool buffer_line(const xyze_pos_t &cart, const_feedRate_t fr_mm_s
      , const uint8_t extruder=active_extruder
      , const PlannerHints &hints=PlannerHints()
    );

    static bool buffer_raw_line(const xyze_pos_t &cart, float acceleration, float nominal_speed,
        float entry_speed, float exit_speed, uint8_t extruder);

    /**
     * Set the planner.position and individual stepper positions.
     * Used by G92, G28, G29, and other procedures.
     *
     * The supplied position is in the cartesian coordinate space and is
     * translated in to machine space as needed. Modifiers such as leveling
     * and skew are also applied.
     *
     * Multiplies by axis_steps_per_mm[] and does necessary conversion
     * for COREXY / COREXZ / COREYZ to set the corresponding stepper positions.
     *
     * Clears previous speed values.
     */
    static void set_position_mm(const xyze_pos_t &xyze);

    static void set_e_position_mm(const_float_t e);

    /// Resets machine position to values from stepper
    static void reset_position();

    /**
     * Set the planner.position and individual stepper positions.
     *
     * The supplied position is in machine space, and no additional
     * conversions are applied.
     */
    static void set_machine_position_mm(const abce_pos_t &abce);

    /**
     * Get an axis position according to stepper position(s)
     * For CORE machines:
     *   - apply translation from AB to XY
     *   - when querying for more axes, use the appropriate overload for less overhead!
     * WARNING:
     *   - it's an expensive function to call
     *   - can return incoherent values when axes are moving
     */
    static float get_axis_position_mm(const AxisEnum axis);
    static void get_axis_position_mm(ab_pos_t& pos);
    static void get_axis_position_mm(abc_pos_t& pos);
    static void get_axis_position_mm(abce_pos_t& pos);

    /**
     * Get planner's axis position in mm
     */
    static abce_pos_t get_machine_position_mm() { return position_float; }

    // Called to force a quick stop of the machine (for example, when
    // a Full Shutdown is required, or when endstops are hit).
    // Will implicitly call drain().
    static void quick_stop();

    // Return the draining status
    static bool draining() { return draining_buffer; }

    // Resume queuing after being held by drain()
    static void resume_queuing();

    // Called when an endstop is triggered. Causes the machine to stop immediately
    static void endstop_triggered(const AxisEnum axis);

    // Triggered position of an axis in mm (not core-savvy)
    static float triggered_position_mm(const AxisEnum axis);

    /**
     * Notes on synchronization primitives and behavior during cancellation
     *
     * When motion is stopped the two lower-level functions draining() and
     * processing() can be checked to determine motion the state. idle() needs
     * to be called until one or both return false to ensure that motion can be
     * scheduled again, depending on context.
     *
     * When canceling a move the backend is stopped first until all internal
     * queues are flushed. In this stage no stepping can take place until
     * processing() returns false in the Marlin thread. To advance the
     * cancellation state idle() needs to be called. position/current_position
     * is undetermined and will usually be at the location expected by the
     * nearest synchronize() call.
     *
     * Once movement can resume draining() will keep returning true until the
     * entire g-code instruction has finished processing. This requires an
     * entire Marlin loop() iteration to complete and any scheduled motion in
     * this stage is just silently accepted and discarded. idle() is not
     * forbidden but will not advance from this state, which is only cleared by
     * the server implementation. Errors should never be reported and/or kill
     * the print when draining() since the move can be re-scheduled.
     *
     * synchronize(): use within the implementation of a compound g-code
     *   sequence in Marlin itself. It waits for motion to complete and ensures
     *   that position/current_position is synchronized with the head movement
     *   when it finally returns. When a g-code sequence has been canceled,
     *   draining() is true and synchronized() returns early so that the command
     *   can continue passively without stopping or special handling.
     *   synchronize() also works as a cancellation barrier: when motion is
     *   interrupted, all queued commands up to the next synchronize() call can
     *   be discarded, but no further.
     *
     * emptying(): return true if the buffer is intentionally being emptied
     *   (for synchronization barriers or while aborting)
     *
     * busy(): is the underlying primitive behind synchronize() and will return
     *   true as long as pending motion hasn't completed or canceled. When a
     *   command has been canceled busy() is never true so that the command can
     *   continue passively without special handling (as the case for most
     *   g-code instructions). When checking for the planner position or state,
     *   draining() should be checked to determine if the command has been
     *   explicitly canceled.
     *
     * processing(): returns true if the backend is still processing the queues,
     *   including during cancellation. If a move has been canceled, all planner
     *   state should be assumed to be undetermined/unknown until processing()
     *   return false. During this state no motion can take place or be
     *   scheduled. The cancellation state is advanced by calling idle() from
     *   the main Marlin thread.
     */

    // Planner is busy processing moves. Not true when being drained!
    static bool busy();

    // Wait for busy(). Does not wait when draining!
    static void synchronize();

    // Indicate whether the buffer is being drained intentionally
    static bool emptying() { return emptying_buffer || draining_buffer; }

    // Wait for busy(), then disable all steppers
    static void finish_and_disable();

    // Periodic tick to handle cleaning timeouts
    // Called from the Temperature ISR at ~1kHz
    static void tick() {}

    /**
     * Does the buffer have any blocks queued?
     */
    FORCE_INLINE static bool has_blocks_queued() { return (block_buffer_head != block_buffer_tail); }

    /**
     * Does the buffer have any unprocessed blocks queued?
     */
    FORCE_INLINE static bool has_unprocessed_blocks_queued() { return (block_buffer_head != block_buffer_nonbusy); }

    /**
     * Return if some processing is still pending before all queues are flushed
     * 
     * !!! Consider using marlin_server::is_processing() instead, it's usually the right choice
     */
    FORCE_INLINE static bool processing() { return has_blocks_queued() || PreciseStepping::processing() || phase_stepping::processing(); }

    /**
     * Returns the current block that PreciseStepping already processed and that is waiting for discarding,
     * nullptr if the queue is empty or none of blocks haven't already been processed.
     */
    static block_t *get_current_processed_block() {
      if (has_blocks_queued()) {
        if (block_t *const block = &block_buffer[block_buffer_tail]; block->busy)
          return block;
      }
      return nullptr;
    }

    /**
     * Returns the current block that PreciseStepping didn't process, nullptr if the queue is empty or all
     * blocks have already been processed.
     * This also marks the block as busy.
     */
    static block_t* get_current_unprocessed_block() {
      // If there are any moves queued ...
      if (has_unprocessed_blocks_queued()) {
        block_t *const block = &block_buffer[block_buffer_nonbusy];
        assert(!block->busy);

        // Recalculation pending? Don't execute yet.
        if (block->flag.recalculate)
          return nullptr;

        // Return the block
        return block;
      }

      return nullptr;
    }

    /**
     * Returns the earliest move block in the queue, nullptr if the queue is empty or if no moves
     * are present. The returned block can be busy or unprocessed.
     */
    static block_t *get_first_move_block() {
      for (uint8_t current_block_idx = block_buffer_tail; current_block_idx != block_buffer_head; ++current_block_idx) {
        if (block_t *const current_block = &block_buffer[current_block_idx]; current_block->is_move())
          return current_block;
      }

      return nullptr;
    }

    // Check if the given block is busy or not
    static bool is_block_busy(const block_t* const block) { return block->busy; }

    /**
     * "Release" the current block so its slot can be reused.
     * Called when the current block is no longer needed.
     * Caller must ensure that there is something to discard.
     */
    FORCE_INLINE static void discard_current_block() {
      assert(has_blocks_queued());
      assert(block_buffer[block_buffer_tail].busy);
      block_buffer_tail = next_block_index(block_buffer_tail);
    }

    /**
     * Discard the current unprocessed block.
     * Caller must ensure that there is something to discard.
     */
    static void discard_current_unprocessed_block();

    #if ENABLED(AUTOTEMP)
      static float autotemp_min, autotemp_max, autotemp_factor;
      static bool autotemp_enabled;
      static void getHighESpeed();
      static void autotemp_M104_M109();
    #endif

    #if HAS_LINEAR_E_JERK
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wdouble-promotion"
      FORCE_INLINE static void recalculate_max_e_jerk() {
        #define GET_MAX_E_JERK(N) SQRT(SQRT(0.5) * junction_deviation_mm * (N) * RECIPROCAL(1.0 - SQRT(0.5)))
        #if ENABLED(DISTINCT_E_FACTORS)
          for (uint8_t i = 0; i < EXTRUDERS; i++)
            max_e_jerk[i] = GET_MAX_E_JERK(settings.max_acceleration_mm_per_s2[E_AXIS_N(i)]);
        #else
          max_e_jerk = GET_MAX_E_JERK(settings.max_acceleration_mm_per_s2[E_AXIS]);
        #endif
      }
      #pragma GCC diagnostic pop
    #endif

  private:
    /// Target planner settings, withOUT limits applied.
    static user_planner_settings_t user_settings_;

    /// Actual settings the planner is using, with limits applied
    static planner_settings_t working_settings_;

    static bool stealth_mode_;

  private:
    /**
     * Get the index of the next / previous block in the ring buffer
     */
    static constexpr uint8_t next_block_index(const uint8_t block_index) { return BLOCK_MOD(block_index + 1); }
    static constexpr uint8_t prev_block_index(const uint8_t block_index) { return BLOCK_MOD(block_index - 1); }

    /**
     * Calculate the maximum allowable speed squared at this point, in order
     * to reach 'target_velocity_sqr' using 'acceleration' within a given
     * 'distance'.
     */
    static float max_allowable_speed_sqr(const_float_t accel, const_float_t target_velocity_sqr, const_float_t distance) {
      return target_velocity_sqr - 2 * accel * distance;
    }

    #if ENABLED(S_CURVE_ACCELERATION)
      /**
       * Calculate the speed reached given initial speed, acceleration and distance
       */
      static float final_speed(const_float_t initial_velocity, const_float_t accel, const_float_t distance) {
        return SQRT(sq(initial_velocity) + 2 * accel * distance);
      }
    #endif

    static void calculate_trapezoid_for_block(block_t * const block, const_float_t entry_speed, const_float_t exit_speed);

    static void reverse_pass_kernel(block_t * const previous, block_t* const current, const block_t * const next OPTARG(HINTS_SAFE_EXIT_SPEED, const_float_t safe_exit_speed_sqr));
    static void forward_pass_kernel(block_t * const previous, block_t* const current, uint8_t prev_index);

    static void reverse_pass(TERN_(HINTS_SAFE_EXIT_SPEED, const_float_t safe_exit_speed_sqr));
    static void forward_pass();

    static void recalculate_trapezoids(TERN_(HINTS_SAFE_EXIT_SPEED, const_float_t safe_exit_speed_sqr));

    static void recalculate(TERN_(HINTS_SAFE_EXIT_SPEED, const_float_t safe_exit_speed_sqr));

    #if DISABLED(CLASSIC_JERK)

      FORCE_INLINE static void normalize_junction_vector(xyze_float_t &vector) {
        float magnitude_sq = 0;
        LOOP_XYZE(idx) if (vector[idx]) magnitude_sq += sq(vector[idx]);
        vector *= RSQRT(magnitude_sq);
      }

      FORCE_INLINE static float limit_value_by_axis_maximum(const_float_t max_value, xyze_float_t &unit_vec) {
        float limit_value = max_value;
        LOOP_XYZE(idx) if (unit_vec[idx]) // Avoid divide by zero
          NOMORE(limit_value, ABS(settings.max_acceleration_mm_per_s2[idx] / unit_vec[idx]));
        return limit_value;
      }

    #endif // !CLASSIC_JERK
};

/**
 * Class provides storage of speed and acceleration parameters of the motion.
 */
class Motion_Parameters {
  public:
    // save motion parameters
    void save();
    // save motion parameters and reset them
    void save_reset();
    // reset motion parameters
    static void reset();
    // load motion parameters back
    void load() const;

  private:
    motion_parameters_t mp;
};

/**
 * Class provides temporary storage of speed and acceleration parameters of the motion.
 * Constructor saves parameters and resets them, destructor retrieves them back.
 */
class Temporary_Reset_Motion_Parameters {
  public:
    Temporary_Reset_Motion_Parameters() {
      mp.save();
      mp.reset();
    }

    ~Temporary_Reset_Motion_Parameters() {
      mp.load();
    }
  private:
    Motion_Parameters mp;
};

#define PLANNER_XY_FEEDRATE() (_MIN(planner.settings.max_feedrate_mm_s[X_AXIS], planner.settings.max_feedrate_mm_s[Y_AXIS]))

extern Planner planner;
