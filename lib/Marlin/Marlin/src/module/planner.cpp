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

/**
 * planner.cpp
 *
 * Buffer movement commands and manage the acceleration profile plan
 *
 * Derived from Grbl
 * Copyright (c) 2009-2011 Simen Svale Skogsrud
 *
 * Ring buffer gleaned from wiring_serial library by David A. Mellis.
 *
 * Fast inverse function needed for Bézier interpolation for AVR
 * was designed, written and tested by Eduardo José Tagle, April 2018.
 *
 * Planner mathematics (Mathematica-style):
 *
 * Where: s == speed, a == acceleration, t == time, d == distance
 *
 * Basic definitions:
 *   Speed[s_, a_, t_] := s + (a*t)
 *   Travel[s_, a_, t_] := Integrate[Speed[s, a, t], t]
 *
 * Distance to reach a specific speed with a constant acceleration:
 *   Solve[{Speed[s, a, t] == m, Travel[s, a, t] == d}, d, t]
 *   d -> (m^2 - s^2) / (2 a)
 *
 * Speed after a given distance of travel with constant acceleration:
 *   Solve[{Speed[s, a, t] == m, Travel[s, a, t] == d}, m, t]
 *   m -> Sqrt[2 a d + s^2]
 *
 * DestinationSpeed[s_, a_, d_] := Sqrt[2 a d + s^2]
 *
 * When to start braking (di) to reach a specified destination speed (s2) after
 * acceleration from initial speed s1 without ever reaching a plateau:
 *   Solve[{DestinationSpeed[s1, a, di] == DestinationSpeed[s2, a, d - di]}, di]
 *   di -> (2 a d - s1^2 + s2^2)/(4 a)
 *
 * We note, as an optimization, that if we have already calculated an
 * acceleration distance d1 from s1 to m and a deceration distance d2
 * from m to s2 then
 *
 *   d1 -> (m^2 - s1^2) / (2 a)
 *   d2 -> (m^2 - s2^2) / (2 a)
 *   di -> (d + d1 - d2) / 2
 */

#include "planner.h"
#include "stepper.h"
#include "motion.h"
#include "temperature.h"
#include "../lcd/ultralcd.h"
#include "../core/language.h"
#include "../gcode/parser.h"

#include "../Marlin.h"

#if HAS_LEVELING
  #include "../feature/bedlevel/bedlevel.h"
#endif

#if ENABLED(FILAMENT_WIDTH_SENSOR)
  #include "../feature/filwidth.h"
#endif

#if ENABLED(AUTO_POWER_CONTROL)
  #include "../feature/power.h"
#endif

#if ENABLED(BACKLASH_COMPENSATION)
  #include "../feature/backlash.h"
#endif

#if ENABLED(CANCEL_OBJECTS)
  #include "../feature/cancel_object.h"
#endif

#if ENABLED(CRASH_RECOVERY)
  #include "../feature/prusa/crash_recovery.hpp"
#endif

#include <option/has_phase_stepping.h>
#if HAS_PHASE_STEPPING()
#include "../feature/phase_stepping/phase_stepping.hpp"
#endif // HAS_PHASE_STEPPING()

#include "configuration_store.h"

constexpr const int32_t MIN_MSTEPS_PER_SEGMENT = MIN_STEPS_PER_SEGMENT * PLANNER_STEPS_MULTIPLIER;

// Delay for delivery of first block to the stepper ISR, if the queue contains 2 or
// fewer movements. The delay is measured in milliseconds, and must be less than 250ms
#define BLOCK_DELAY_FOR_1ST_MOVE 100

Planner planner;

  // public:

/**
 * A ring buffer of moves described in steps
 */
block_t Planner::block_buffer[BLOCK_BUFFER_SIZE];
volatile uint8_t Planner::block_buffer_head,    // Index of the next block to be pushed
                 Planner::block_buffer_nonbusy, // Index of the first non-busy block
                 Planner::block_buffer_planned, // Index of the optimally planned block
                 Planner::block_buffer_tail;    // Index of the busy block, if any
uint8_t Planner::delay_before_delivering;       // This counter delays delivery of blocks when queue becomes empty to allow the opportunity of merging blocks

// A flag to drop queuing of blocks and abort any pending move
bool Planner::draining_buffer;

// A flag to indicate that that buffer is being emptied intentionally
bool Planner::emptying_buffer;

planner_settings_t Planner::working_settings_;
user_planner_settings_t Planner::user_settings_;

const planner_settings_t &Planner::settings = working_settings_;
const user_planner_settings_t &Planner::user_settings = user_settings_;

bool Planner::stealth_mode_ = false;

void Planner::apply_settings(const user_planner_settings_t &settings) {
  static constexpr planner_settings_t standard_limits = {
    .max_acceleration_mm_per_s2 = HWLIMIT_NORMAL_MAX_ACCELERATION,
    .max_feedrate_mm_s = HWLIMIT_NORMAL_MAX_FEEDRATE,
    .max_jerk = HWLIMIT_NORMAL_JERK,
  };
  static constexpr planner_settings_t stealth_limits = {
    .max_acceleration_mm_per_s2 = HWLIMIT_STEALTH_MAX_ACCELERATION,
    .max_feedrate_mm_s = HWLIMIT_STEALTH_MAX_FEEDRATE,
    .max_jerk = HWLIMIT_STEALTH_JERK,
  };
  const auto &limits = stealth_mode_ ? stealth_limits : standard_limits;

  user_settings_ = settings;
  working_settings_ = settings;

  const auto apply_limit = [&]<typename T>(T planner_settings_t::*member) {
    auto &value = working_settings_.*member;
    const auto &limit = limits.*member;

    if constexpr(std::is_array_v<T>) {
      for(size_t i = 0; i < std::size(value); i++) {
        value[i] = std::min(value[i], limit[i]);
      }
    } else if constexpr(std::is_array_v<T> || std::is_same_v<T, xyze_pos_t>) {
      for(size_t i = 0; i < std::size(value.pos); i++) {
        value[i] = std::min(value[i], limit[i]);
      }
    } else {
      value = std::min(value, limit);
    }
  };

  apply_limit(&planner_settings_t::max_feedrate_mm_s);
  apply_limit(&planner_settings_t::max_acceleration_mm_per_s2);
  apply_limit(&planner_settings_t::max_jerk);

  refresh_acceleration_rates();
}

void Planner::set_stealth_mode(bool set) {
  if(stealth_mode_ != set) {
    stealth_mode_ = set;
    apply_settings(user_settings);
  }
}

uint32_t Planner::max_acceleration_msteps_per_s2[XYZE_N]; // (mini-steps/s^2) Derived from mm_per_s2

float Planner::mm_per_step[XYZE_N];           // (mm) Millimeters per step
float Planner::mm_per_half_step[XYZE_N];      // (mm) Millimeters per half step
float Planner::mm_per_mstep[XYZE_N];          // (mm) Millimeters per mini-step

#if DISABLED(CLASSIC_JERK)
  float Planner::junction_deviation_mm;       // (mm) M205 J
#endif

#if ENABLED(DISTINCT_E_FACTORS)
  uint8_t Planner::last_extruder = 0;     // Respond to extruder change
#endif

#if EXTRUDERS
  int16_t Planner::flow_percentage[EXTRUDERS] = ARRAY_BY_EXTRUDERS1(100); // Extrusion factor for each extruder
  float Planner::e_factor[EXTRUDERS] = ARRAY_BY_EXTRUDERS1(1.0f); // The flow percentage and volumetric multiplier combine to scale E movement
#endif

#if DISABLED(NO_VOLUMETRICS)
  float Planner::filament_size[EXTRUDERS],          // diameter of filament (in millimeters), typically around 1.75 or 2.85, 0 disables the volumetric calculations for the extruder
        Planner::volumetric_area_nominal = CIRCLE_AREA(float(DEFAULT_NOMINAL_FILAMENT_DIA) * 0.5f), // Nominal cross-sectional area
        Planner::volumetric_multiplier[EXTRUDERS];  // Reciprocal of cross-sectional area of filament (in mm^2). Pre-calculated to reduce computation in the planner
#endif

#if HAS_LEVELING
  bool Planner::leveling_active = false; // Flag that auto bed leveling is enabled
  #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
    float Planner::z_fade_height,      // Initialized by settings.load()
          Planner::inverse_z_fade_height,
          Planner::last_fade_z;
  #endif
#else
  constexpr bool Planner::leveling_active;
#endif

skew_factor_t Planner::skew_factor; // Initialized by settings.load()

#if ENABLED(AUTOTEMP)
  float Planner::autotemp_max = 250,
        Planner::autotemp_min = 210,
        Planner::autotemp_factor = 0.1f;
  bool Planner::autotemp_enabled = false;
#endif

// private:

xyze_long_t Planner::position{0};

uint32_t Planner::cutoff_long;

xyze_float_t Planner::previous_speed;
float Planner::previous_nominal_speed;

#if ENABLED(DISABLE_INACTIVE_EXTRUDER)
  uint8_t Planner::g_uc_extruder_last_move[EXTRUDERS] = { 0 };
#endif

#ifdef XY_FREQUENCY_LIMIT
  // Old direction bits. Used for speed calculations
  unsigned char Planner::old_direction_bits = 0;
  // Segment times (in µs). Used for speed calculations
  xy_ulong_t Planner::axis_segment_time_us[3] = { { MAX_FREQ_TIME_US + 1, MAX_FREQ_TIME_US + 1 } };
#endif

xyze_pos_t Planner::position_float; // Needed for accurate maths. Steps cannot be used!

/**
 * Class and Instance Methods
 */

void Planner::init() {
  position.reset();
  position_float.reset();
  previous_speed.reset();
  previous_nominal_speed = 0;
  clear_block_buffer();
  delay_before_delivering = 0;
}

#if ENABLED(S_CURVE_ACCELERATION)
  // All other 32-bit MPUs can easily do inverse using hardware division,
  // so we don't need to reduce precision or to use assembly language at all.
  // This routine, for all other archs, returns 0x100000000 / d ~= 0xFFFFFFFF / d
  static FORCE_INLINE uint32_t get_period_inverse(const uint32_t d) {
    return d ? 0xFFFFFFFF / d : 0xFFFFFFFF;
  }

#define MINIMAL_STEP_RATE 120
#endif

/**
 * Calculate trapezoid parameters, multiplying the entry- and exit-speeds
 * by the provided factors.
 **
 * ############ VERY IMPORTANT ############
 * NOTE that the PRECONDITION to call this function is that the block is
 * NOT BUSY and it is marked as RECALCULATE. That WARRANTIES the Stepper ISR
 * is not and will not use the block while we modify it, so it is safe to
 * alter its values.
 */
void Planner::calculate_trapezoid_for_block(block_t * const block, const_float_t entry_speed, const_float_t exit_speed) {
  // Store new block parameters
  block->initial_speed = entry_speed;
  block->final_speed = exit_speed;

  #if ENABLED(S_CURVE_ACCELERATION)
    const float nomr = 1.0f / block->nominal_speed;
    const float entry_factor = entry_speed * nomr,
                exit_factor = exit_speed * nomr;

  uint32_t initial_rate = CEIL(block->nominal_rate * entry_factor),
           final_rate = CEIL(block->nominal_rate * exit_factor); // (steps per second)

  // TODO @hejllukas: Probably we don't need to limit the minimal step rate at all because the current stepper routine should handle it without overflow.
  // Limit minimal step rate (Otherwise the timer will overflow.)
  NOLESS(initial_rate, uint32_t(MINIMAL_STEP_RATE));
  NOLESS(final_rate, uint32_t(MINIMAL_STEP_RATE));

    // If we have some plateau time, the cruise rate will be the nominal rate
    uint32_t cruise_rate = block->nominal_rate;

  // Steps for acceleration, plateau and deceleration
  int32_t plateau_steps = block->mstep_event_count;
  uint32_t accelerate_steps = 0,
           decelerate_steps = 0;

  const int32_t accel = block->acceleration_steps_per_s2;
  float inverse_accel = 0.0f;
  if (accel != 0) {
    inverse_accel = 1.0f / accel;
    const float half_inverse_accel = 0.5f * inverse_accel,
                nominal_rate_sq = sq(float(block->nominal_rate)),
                // Steps required for acceleration, deceleration to/from nominal rate
                decelerate_steps_float = half_inverse_accel * (nominal_rate_sq - sq(float(final_rate)));
          float accelerate_steps_float = half_inverse_accel * (nominal_rate_sq - sq(float(initial_rate)));
    accelerate_steps = CEIL(accelerate_steps_float);
    decelerate_steps = FLOOR(decelerate_steps_float);

    // Steps between acceleration and deceleration, if any
    plateau_steps -= accelerate_steps + decelerate_steps;

    // Does accelerate_steps + decelerate_steps exceed mstep_event_count?
    // Then we can't possibly reach the nominal rate, there will be no cruising.
    // Calculate accel / braking time in order to reach the final_rate exactly
    // at the end of this block.
    if (plateau_steps < 0) {
      accelerate_steps_float = CEIL((block->mstep_event_count + accelerate_steps_float - decelerate_steps_float) * 0.5f);
      accelerate_steps = _MIN(uint32_t(_MAX(accelerate_steps_float, 0)), block->mstep_event_count);

        // We won't reach the cruising rate. Let's calculate the speed we will reach
        cruise_rate = final_speed(initial_rate, accel, accelerate_steps);
    }
  }

    const float rate_factor = inverse_accel * (STEPPER_TIMER_RATE);
    // Jerk controlled speed requires to express speed versus time, NOT steps
    uint32_t acceleration_time = rate_factor * float(cruise_rate - initial_rate),
             deceleration_time = rate_factor * float(cruise_rate - final_rate),
    // And to offload calculations from the ISR, we also calculate the inverse of those times here
             acceleration_time_inverse = get_period_inverse(acceleration_time),
             deceleration_time_inverse = get_period_inverse(deceleration_time);

  // Store new block parameters
  block->initial_rate = initial_rate;
    block->acceleration_time = acceleration_time;
    block->deceleration_time = deceleration_time;
    block->acceleration_time_inverse = acceleration_time_inverse;
    block->deceleration_time_inverse = deceleration_time_inverse;
    block->cruise_rate = cruise_rate;
  block->final_rate = final_rate;
  #endif
}

/**
 *                              PLANNER SPEED DEFINITION
 *                                     +--------+   <- current->nominal_speed
 *                                    /          \
 *         current->entry_speed ->   +            \
 *                                   |             + <- next->entry_speed (aka exit speed)
 *                                   +-------------+
 *                                       time -->
 *
 *  Recalculates the motion plan according to the following basic guidelines:
 *
 *    1. Go over every feasible block sequentially in reverse order and calculate the junction speeds
 *        (i.e. current->entry_speed) such that:
 *      a. No junction speed exceeds the pre-computed maximum junction speed limit or nominal speeds of
 *         neighboring blocks.
 *      b. A block entry speed cannot exceed one reverse-computed from its exit speed (next->entry_speed)
 *         with a maximum allowable deceleration over the block travel distance.
 *      c. The last (or newest appended) block is planned from a complete stop (an exit speed of zero).
 *    2. Go over every block in chronological (forward) order and dial down junction speed values if
 *      a. The exit speed exceeds the one forward-computed from its entry speed with the maximum allowable
 *         acceleration over the block travel distance.
 *
 *  When these stages are complete, the planner will have maximized the velocity profiles throughout the all
 *  of the planner blocks, where every block is operating at its maximum allowable acceleration limits. In
 *  other words, for all of the blocks in the planner, the plan is optimal and no further speed improvements
 *  are possible. If a new block is added to the buffer, the plan is recomputed according to the said
 *  guidelines for a new optimal plan.
 *
 *  To increase computational efficiency of these guidelines, a set of planner block pointers have been
 *  created to indicate stop-compute points for when the planner guidelines cannot logically make any further
 *  changes or improvements to the plan when in normal operation and new blocks are streamed and added to the
 *  planner buffer. For example, if a subset of sequential blocks in the planner have been planned and are
 *  bracketed by junction velocities at their maximums (or by the first planner block as well), no new block
 *  added to the planner buffer will alter the velocity profiles within them. So we no longer have to compute
 *  them. Or, if a set of sequential blocks from the first block in the planner (or a optimal stop-compute
 *  point) are all accelerating, they are all optimal and can not be altered by a new block added to the
 *  planner buffer, as this will only further increase the plan speed to chronological blocks until a maximum
 *  junction velocity is reached. However, if the operational conditions of the plan changes from infrequently
 *  used feed holds or feedrate overrides, the stop-compute pointers will be reset and the entire plan is
 *  recomputed as stated in the general guidelines.
 *
 *  Planner buffer index mapping:
 *  - block_buffer_tail: Points to the beginning of the planner buffer. First to be executed or being executed.
 *  - block_buffer_head: Points to the buffer block after the last block in the buffer. Used to indicate whether
 *      the buffer is full or empty. As described for standard ring buffers, this block is always empty.
 *  - block_buffer_planned: Points to the first buffer block after the last optimally planned block for normal
 *      streaming operating conditions. Use for planning optimizations by avoiding recomputing parts of the
 *      planner buffer that don't change with the addition of a new block, as describe above. In addition,
 *      this block can never be less than block_buffer_tail and will always be pushed forward and maintain
 *      this requirement when encountered by the Planner::release_current_block() routine during a cycle.
 *
 *  NOTE: Since the planner only computes on what's in the planner buffer, some motions with many short
 *        segments (e.g., complex curves) may seem to move slowly. This is because there simply isn't
 *        enough combined distance traveled in the entire buffer to accelerate up to the nominal speed and
 *        then decelerate to a complete stop at the end of the buffer, as stated by the guidelines. If this
 *        happens and becomes an annoyance, there are a few simple solutions:
 *
 *    - Maximize the machine acceleration. The planner will be able to compute higher velocity profiles
 *      within the same combined distance.
 *
 *    - Maximize line motion(s) distance per block to a desired tolerance. The more combined distance the
 *      planner has to use, the faster it can go.
 *
 *    - Maximize the planner buffer size. This also will increase the combined distance for the planner to
 *      compute over. It also increases the number of computations the planner has to perform to compute an
 *      optimal plan, so select carefully.
 *
 *    - Use G2/G3 arcs instead of many short segments. Arcs inform the planner of a safe exit speed at the
 *      end of the last segment, which alleviates this problem.
 */

// The kernel called by recalculate() when scanning the plan from last to first entry.
void Planner::reverse_pass_kernel(block_t * const previous, block_t * const current, const block_t * const next
  OPTARG(HINTS_SAFE_EXIT_SPEED, const_float_t safe_exit_speed_sqr)
) {
  // If entry speed is already at the maximum entry speed, and there was no change of speed
  // in the next block, there is no need to recheck. Block is cruising and there is no need to
  // compute anything for this block,
  // If not, block entry speed needs to be recalculated to ensure maximum possible planned speed.
  const float max_entry_speed_sqr = current->max_entry_speed_sqr;

  // Compute maximum entry speed decelerating over the current block from its exit speed.
  // If not at the maximum entry speed, or the previous block entry speed changed
  if (current->entry_speed_sqr != max_entry_speed_sqr || (next && next->flag.recalculate)) {

    // If nominal length true, max junction speed is guaranteed to be reached.
    // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
    // the current block and next block junction speeds are guaranteed to always be at their maximum
    // junction speeds in deceleration and acceleration, respectively. This is due to how the current
    // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
    // the reverse and forward planners, the corresponding block junction speed will always be at the
    // the maximum junction speed and may always be ignored for any speed reduction checks.

    const float next_entry_speed_sqr = next ? next->entry_speed_sqr : _MAX(TERN0(HINTS_SAFE_EXIT_SPEED, safe_exit_speed_sqr), sq(float(MINIMUM_PLANNER_SPEED))),
                new_entry_speed_sqr = current->flag.nominal_length
                  ? max_entry_speed_sqr
                  : _MIN(max_entry_speed_sqr, max_allowable_speed_sqr(-current->acceleration, next_entry_speed_sqr, current->millimeters));
    if (current->entry_speed_sqr != new_entry_speed_sqr) {

      // Before changing the entry speed of the current block we must ensure the previous block
      // can still be recomputed, so attempt to mark it as busy
      previous->flag.recalculate = true;

      if (is_block_busy(previous)) {
        // We lost the race with the ISR, clear the recalculate flag
        previous->flag.recalculate = false;
      }
      else {
        // We won the race, also mark the current block and update the entry speed
        current->flag.recalculate = true;
        current->entry_speed_sqr = new_entry_speed_sqr;
      }
    }
  }
}

/**
 * recalculate() needs to go over the current plan twice.
 * Once in reverse and once forward. This implements the reverse pass.
 */
void Planner::reverse_pass(TERN_(HINTS_SAFE_EXIT_SPEED, const_float_t safe_exit_speed_sqr)) {
  // Initialize block index to the last block in the planner buffer.
  uint8_t current_index = block_buffer_head,
          prev_index = prev_block_index(block_buffer_head);

  // Read the index of the last buffer planned block.
  // The ISR may change it so get a stable local copy.
  uint8_t planned_block_index = block_buffer_planned;

  // Reverse Pass: Coarsely maximize all possible deceleration curves back-planning from the last
  // block in buffer. Cease planning when the last optimal planned or tail pointer is reached.
  // NOTE: Forward pass will later refine and correct the reverse pass to create an optimal plan.
  const block_t *next = nullptr;
  block_t *current = nullptr;
  while(current_index != planned_block_index) {

    // Perform the reverse pass
    block_t *previous = &block_buffer[prev_index];

    // Only consider non sync blocks
    if (previous->is_move()) {
      // If there's no previous block or the previous block is not
      // BUSY (thus, modifiable) run the reverse_pass_kernel. Otherwise,
      // the previous block became BUSY, so assume the current block's
      // entry speed can't be altered (since that would also require
      // updating the exit speed of the previous block).
      if (previous && !is_block_busy(previous) && current)
        reverse_pass_kernel(previous, current, next OPTARG(HINTS_SAFE_EXIT_SPEED, safe_exit_speed_sqr));;
      next = current;
      current = previous;
      current_index = prev_index;
    }

    // If we just included the planned block the remainder is already optimal or
    // can't be modified past this point, break the loop early
    if (prev_index == planned_block_index)
      break;

    // Advance to the next
    prev_index = prev_block_index(prev_index);

    // The ISR could advance the block_buffer_planned while we were doing the reverse pass.
    // We must try to avoid using an already consumed block as the last one - So follow
    // changes to the pointer and make sure to limit the loop to the currently busy block
    while (planned_block_index != block_buffer_planned) {

      // If the planned block was pushed by the ISR, is it also guaranteed
      // to be busy and thus unmodifiable: abort immediately
      if (prev_index == planned_block_index) return;

      // Advance the pointer, following the busy block
      planned_block_index = next_block_index(planned_block_index);
    }
  }
}

// The kernel called by recalculate() when scanning the plan from first to last entry.
void Planner::forward_pass_kernel(block_t * const previous, block_t * const current, const uint8_t prev_index) {
  // If the previous block is an acceleration block, too short to complete the full speed
  // change, adjust the entry speed accordingly. Entry speeds have already been reset,
  // maximized, and reverse-planned. If nominal length is set, max junction speed is
  // guaranteed to be reached. No need to recheck.
  if (!previous->flag.nominal_length && previous->entry_speed_sqr < current->entry_speed_sqr) {

    // Compute the maximum allowable speed
    const float new_entry_speed_sqr = max_allowable_speed_sqr(-previous->acceleration, previous->entry_speed_sqr, previous->millimeters);

    // If true, current block is full-acceleration and we can move the planned pointer forward.
    if (new_entry_speed_sqr < current->entry_speed_sqr) {

      // Before changing the entry speed of the current block we must ensure the previous block
      // can still be recomputed, so attempt to mark for recalculation
      previous->flag.recalculate = true;

      if (is_block_busy(previous)) {
        // We lost the race with the ISR, clear the recalculate flag
        previous->flag.recalculate = false;
      }
      else {
        // We won the race, also mark the current block
        current->flag.recalculate = true;

        // Always <= max_entry_speed_sqr. Backward pass sets this.
        current->entry_speed_sqr = new_entry_speed_sqr; // Always <= max_entry_speed_sqr. Backward pass sets this.

        // Set optimal plan pointer.
        block_buffer_planned = prev_index;
      }
    }
  }

  // Any block set at its maximum entry speed also creates an optimal plan up to this
  // point in the buffer. When the plan is bracketed by either the beginning of the
  // buffer and a maximum entry speed or two maximum entry speeds, every block in between
  // cannot logically be further improved. Hence, we don't have to recompute them anymore.
  if (current->entry_speed_sqr == current->max_entry_speed_sqr)
    block_buffer_planned = prev_index;
}

/**
 * recalculate() needs to go over the current plan twice.
 * Once in reverse and once forward. This implements the forward pass.
 */
void Planner::forward_pass() {

  // Forward Pass: Forward plan the acceleration curve from the planned pointer onward.
  // Also scans for optimal plan breakpoints and appropriately updates the planned pointer.

  // Begin at buffer planned pointer. Note that block_buffer_planned can be modified
  //  by the stepper ISR,  so read it ONCE. It it guaranteed that block_buffer_planned
  //  will never lead head, so the loop is safe to execute. Also note that the forward
  //  pass will never modify the values at the tail.
  uint8_t block_index = block_buffer_planned;
  uint8_t prev_index;

  block_t *block;
  block_t *previous = nullptr;
  while (block_index != block_buffer_head) {

    // Perform the forward pass
    block = &block_buffer[block_index];

    // Only process movement blocks
    if (block->is_move()) {
      // If there's no previous block or the previous block is not
      // BUSY (thus, modifiable) run the forward_pass_kernel. Otherwise,
      // the previous block became BUSY, so assume the current block's
      // entry speed can't be altered (since that would also require
      // updating the exit speed of the previous block).
      if (previous && !is_block_busy(previous))
        forward_pass_kernel(previous, block, prev_index);
      previous = block;
      prev_index = block_index;
    }
    // Advance to the previous
    block_index = next_block_index(block_index);
  }
}

/**
 * Recalculate the trapezoid speed profiles for all blocks in the plan
 * according to the entry_factor for each junction. Must be called by
 * recalculate() after updating the blocks.
 */
void Planner::recalculate_trapezoids(TERN_(HINTS_SAFE_EXIT_SPEED, const_float_t safe_exit_speed_sqr)) {
  // The tail may be changed by the ISR so get a local copy.
  uint8_t block_index = block_buffer_nonbusy,
          head_block_index = block_buffer_head,
          tail_block_index = block_buffer_tail;

  // Move backwards to find the first busy/non-SYNC block so that we can initialize the
  // entry speed from a running move. If there's none then we must have come to a halt.
  while (tail_block_index != block_index) {
    block_index = prev_block_index(block_index);

    // Get the pointer to the block
    block_t *block = &block_buffer[block_index];

    if (is_block_busy(block) && block->is_move()) {
      // Found the first running move, we're done
      break;
    }
  }

  // Since there could be a sync block in the head of the queue, and the
  // next loop must not recalculate the head block (as it needs to be
  // specially handled), scan backwards to the first non-SYNC block.
  while (head_block_index != block_index) {

    // Go back (head always point to the first free block)
    const uint8_t prev_index = prev_block_index(head_block_index);

    // Get the pointer to the block
    block_t *prev = &block_buffer[prev_index];

    // It the block is a move, we're done with this loop
    if (prev->is_move()) break;

    // Examine the previous block. This and all following are SYNC blocks
    head_block_index = prev_index;
  }

  // Go from the tail (currently executed block) to the first block, without including it)
  block_t *block = nullptr, *next = nullptr;
  float current_entry_speed = 0.0f, next_entry_speed = 0.0f;
  while (block_index != head_block_index) {

    next = &block_buffer[block_index];

    // Only process movement blocks
    if (next->is_move()) {
      next_entry_speed = SQRT(next->entry_speed_sqr);

      if (block) {

        // Recalculate if current block entry or exit junction speed has changed.
        if (block->flag.recalculate) {

          // NOTE: Entry and exit factors always > 0 by all previous logic operations.
          calculate_trapezoid_for_block(block, current_entry_speed, next_entry_speed);

          // Reset current only to ensure next trapezoid is computed - The
          // stepper is free to use the block from now on.
          block->flag.recalculate = false;
        }
      }

      block = next;
      current_entry_speed = next_entry_speed;
    }

    block_index = next_block_index(block_index);
  }

  // Last/newest block in buffer. Always recalculated.
  if (block) {
    // Exit speed is set with MINIMUM_PLANNER_SPEED unless some code higher up knows better.
    next_entry_speed = _MAX(TERN0(HINTS_SAFE_EXIT_SPEED, SQRT(safe_exit_speed_sqr)), float(MINIMUM_PLANNER_SPEED));

    // Mark the next(last) block as RECALCULATE, to prevent the Stepper ISR running it.
    // As the last block is always recalculated here, there is a chance the block isn't
    // marked as RECALCULATE yet. That's the reason for the following line.
    block->flag.recalculate = true;

    // But there is an inherent race condition here, as the block maybe
    // became BUSY, just before it was marked as RECALCULATE, so check
    // if that is the case!
    if (!is_block_busy(block)) {
      // Block is not BUSY, we won the race against the Stepper ISR:
      calculate_trapezoid_for_block(block, current_entry_speed, next_entry_speed);
    }

    // Reset block to ensure its trapezoid is computed - The stepper is free to use
    // the block from now on.
    block->flag.recalculate = false;
  }
}

void Planner::recalculate(TERN_(HINTS_SAFE_EXIT_SPEED, const_float_t safe_exit_speed_sqr)) {
  // Initialize block index to the last block in the planner buffer.
  const uint8_t block_index = prev_block_index(block_buffer_head);
  // If there is just one block, no planning can be done. Avoid it!
  if (block_index != block_buffer_planned) {
    reverse_pass(TERN_(HINTS_SAFE_EXIT_SPEED, safe_exit_speed_sqr));
    forward_pass();
  }
  recalculate_trapezoids(TERN_(HINTS_SAFE_EXIT_SPEED, safe_exit_speed_sqr));
}

/**
 * Discard the current unprocessed block.
 * Caller must ensure that there is something to discard.
 */
void Planner::discard_current_unprocessed_block() {
  assert(has_unprocessed_blocks_queued());

  block_t * block = &block_buffer[block_buffer_nonbusy];
  assert(!block->busy);
  block->busy = true;

  if (block_buffer_nonbusy != block_buffer_planned)
    block_buffer_nonbusy = next_block_index(block_buffer_nonbusy);
  else {
    // push "planned" block as it became busy as well
    block_buffer_nonbusy = next_block_index(block_buffer_nonbusy);
    block_buffer_planned = block_buffer_nonbusy;
  }
}

#if ENABLED(AUTOTEMP)

  void Planner::getHighESpeed() {
    static float oldt = 0;

    if (!autotemp_enabled) return;
    if (thermalManager.degTargetHotend(0) + 2 < autotemp_min) return; // probably temperature set to zero.

    float high = 0.0;
    for (uint8_t b = block_buffer_tail; b != block_buffer_head; b = next_block_index(b)) {
      block_t* block = &block_buffer[b];
      if (block->msteps.x || block->msteps.y || block->msteps.z) {
        const float se = (float)block->msteps.e / block->mstep_event_count * block->nominal_speed; // mm/sec;
        NOLESS(high, se);
      }
    }

    float t = autotemp_min + high * autotemp_factor;
    LIMIT(t, autotemp_min, autotemp_max);
    if (t < oldt) t = t * (1 - float(AUTOTEMP_OLDWEIGHT)) + oldt * float(AUTOTEMP_OLDWEIGHT);
    oldt = t;
    thermalManager.setTargetHotend(t, 0);
  }

#endif // AUTOTEMP

/**
 * Maintain fans, paste extruder pressure,
 */
void Planner::check_axes_activity() {

  #if ANY(DISABLE_X, DISABLE_Y, DISABLE_Z, DISABLE_E)
    xyze_bool_t axis_active = { false };
  #endif

  #if FAN_COUNT > 0
    uint8_t tail_fan_speed[FAN_COUNT];
  #endif

  // In the current implementation of PreciseStepping, a sync position block can spend some time at the top of the block queue in contrast with the original Marlin.
  // So we have to ignore sync position blocks because they always have zero fan speeds.
  if (const block_t *block = get_first_move_block(); block != nullptr) {
    #if FAN_COUNT > 0
      FANS_LOOP(i)
        tail_fan_speed[i] = thermalManager.scaledFanSpeed(i, block->fan_speed[i]);
    #endif

    #if ANY(DISABLE_X, DISABLE_Y, DISABLE_Z, DISABLE_E)
      for (uint8_t b = block_buffer_tail; b != block_buffer_head; b = next_block_index(b)) {
        block_t *block = &block_buffer[b];
        LOOP_XYZE(i) if (block->msteps[i]) axis_active[i] = true;
      }
    #endif
  }
  else {

    #if FAN_COUNT > 0
      FANS_LOOP(i)
        tail_fan_speed[i] = thermalManager.scaledFanSpeed(i);
    #endif
  }

  //
  // Disable inactive axes
  //
  #if (ENABLED(XY_LINKED_ENABLE) && (ENABLED(DISABLE_X) || ENABLED(DISABLE_Y)))
    if (!axis_active.x && !axis_active.y) disable_XY();
  #else
    #if ENABLED(DISABLE_X)
      if (!axis_active.x) disable_X();
    #endif
    #if ENABLED(DISABLE_Y)
      if (!axis_active.y) disable_Y();
    #endif
  #endif
  #if ENABLED(DISABLE_Z)
    if (!axis_active.z) disable_Z();
  #endif
  #if ENABLED(DISABLE_E)
    if (!axis_active.e) disable_e_steppers();
  #endif

  //
  // Update Fan speeds
  //
  #if FAN_COUNT > 0

    #if FAN_KICKSTART_TIME > 0
      static millis_t fan_kick_end[FAN_COUNT] = { 0 };
      #define KICKSTART_FAN(f)                         \
        if (tail_fan_speed[f]) {                       \
          millis_t ms = millis();                      \
          if (fan_kick_end[f] == 0) {                  \
            fan_kick_end[f] = ms + FAN_KICKSTART_TIME; \
            tail_fan_speed[f] = 255;                   \
          } else if (PENDING(ms, fan_kick_end[f]))     \
            tail_fan_speed[f] = 255;                   \
        } else fan_kick_end[f] = 0
    #else
      #define KICKSTART_FAN(f) NOOP
    #endif

    #if FAN_MIN_PWM != 0 || FAN_MAX_PWM != 255
      #define CALC_FAN_SPEED(f) (tail_fan_speed[f] ? map(tail_fan_speed[f], 1, 255, FAN_MIN_PWM, FAN_MAX_PWM) : 0)
    #else
      #define CALC_FAN_SPEED(f) tail_fan_speed[f]
    #endif

    #if ENABLED(FAN_SOFT_PWM)
      #define _FAN_SET(F) thermalManager.soft_pwm_amount_fan[F] = CALC_FAN_SPEED(F);
    #elif ENABLED(FAST_PWM_FAN)
      #define _FAN_SET(F) set_pwm_duty(FAN##F##_PIN, CALC_FAN_SPEED(F));
    #else
      #define _FAN_SET(F) analogWrite(pin_t(FAN##F##_PIN), CALC_FAN_SPEED(F));
    #endif
    #define FAN_SET(F) do{ KICKSTART_FAN(F); _FAN_SET(F); }while(0)

    #if HAS_FAN0
      FAN_SET(0);
    #endif
    #if HAS_FAN1
      FAN_SET(1);
    #endif
    #if HAS_FAN2
      FAN_SET(2);
    #endif

  #endif // FAN_COUNT > 0

  #if ENABLED(AUTOTEMP)
    getHighESpeed();
  #endif
}

#if DISABLED(NO_VOLUMETRICS)

  /**
   * Get a volumetric multiplier from a filament diameter.
   * This is the reciprocal of the circular cross-section area.
   * Return 1.0 with volumetric off or a diameter of 0.0.
   */
  inline float calculate_volumetric_multiplier(const_float_t diameter) {
    return (parser.volumetric_enabled && diameter) ? 1.0f / CIRCLE_AREA(diameter * 0.5f) : 1;
  }

  /**
   * Convert the filament sizes into volumetric multipliers.
   * The multiplier converts a given E value into a length.
   */
  void Planner::calculate_volumetric_multipliers() {
    for (uint8_t i = 0; i < COUNT(filament_size); i++) {
      volumetric_multiplier[i] = calculate_volumetric_multiplier(filament_size[i]);
      refresh_e_factor(i);
    }
  }

#endif // !NO_VOLUMETRICS

#if ENABLED(FILAMENT_WIDTH_SENSOR)
  /**
   * Convert the ratio value given by the filament width sensor
   * into a volumetric multiplier. Conversion differs when using
   * linear extrusion vs volumetric extrusion.
   */
  void Planner::apply_filament_width_sensor(const int8_t encoded_ratio) {
    // Reconstitute the nominal/measured ratio
    const float nom_meas_ratio = 1 + 0.01f * encoded_ratio,
                ratio_2 = sq(nom_meas_ratio);

    volumetric_multiplier[FILAMENT_SENSOR_EXTRUDER_NUM] = parser.volumetric_enabled
      ? ratio_2 / CIRCLE_AREA(filwidth.nominal_mm * 0.5f) // Volumetric uses a true volumetric multiplier
      : ratio_2;                                          // Linear squares the ratio, which scales the volume

    refresh_e_factor(FILAMENT_SENSOR_EXTRUDER_NUM);
  }
#endif

#if HAS_LEVELING

  constexpr xy_pos_t level_fulcrum = {
    #if ENABLED(Z_SAFE_HOMING)
      Z_SAFE_HOMING_X_POINT, Z_SAFE_HOMING_Y_POINT
    #else
      X_HOME_POS, Y_HOME_POS
    #endif
  };

  /**
   * rx, ry, rz - Cartesian positions in mm
   *              Leveled XYZ on completion
   */
  void Planner::apply_leveling(xyz_pos_t &raw) {
    if (!leveling_active) return;

    #if HAS_MESH

      #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
        const float fade_scaling_factor = fade_scaling_factor_for_z(raw.z);
      #else
        constexpr float fade_scaling_factor = 1.0;
      #endif

      raw.z += (
        #if ENABLED(MESH_BED_LEVELING)
          mbl.get_z(raw
            #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
              , fade_scaling_factor
            #endif
          )
        #elif ENABLED(AUTO_BED_LEVELING_UBL)
          fade_scaling_factor ? fade_scaling_factor * ubl.get_z_correction(raw) : 0.0
        #elif ENABLED(AUTO_BED_LEVELING_BILINEAR)
          fade_scaling_factor ? fade_scaling_factor * bilinear_z_offset(raw) : 0.0
        #endif
      );

    #endif
  }

  void Planner::unapply_leveling(xyz_pos_t &raw) {

    if (leveling_active) {

      #if HAS_MESH

        #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
          const float fade_scaling_factor = fade_scaling_factor_for_z(raw.z);
        #else
          constexpr float fade_scaling_factor = 1.0;
        #endif

        raw.z -= (
          #if ENABLED(MESH_BED_LEVELING)
            mbl.get_z(raw
              #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
                , fade_scaling_factor
              #endif
            )
          #elif ENABLED(AUTO_BED_LEVELING_UBL)
            fade_scaling_factor ? fade_scaling_factor * ubl.get_z_correction(raw) : 0.0
          #elif ENABLED(AUTO_BED_LEVELING_BILINEAR)
            fade_scaling_factor ? fade_scaling_factor * bilinear_z_offset(raw) : 0.0
          #endif
        );

      #endif
    }

    #if ENABLED(SKEW_CORRECTION)
      unskew(raw);
    #endif
  }

#endif // HAS_LEVELING

#if ENABLED(FWRETRACT)
  /**
   * rz, e - Cartesian positions in mm
   */
  void Planner::apply_retract(float &rz, float &e) {
    rz += fwretract.current_hop;
    e -= fwretract.current_retract[active_extruder];
  }

  void Planner::unapply_retract(float &rz, float &e) {
    rz -= fwretract.current_hop;
    e += fwretract.current_retract[active_extruder];
  }

#endif

void Planner::quick_stop() {
  // Immediately stop motion: all pending moves will be discarded later
  PreciseStepping::quick_stop();

  // Start draining the planner (requires one full marlin loop to complete!)
  drain();

  // Restart the block delay for the first movement - As the queue was
  // forced to empty, there's no risk the ISR will touch this.
  delay_before_delivering = BLOCK_DELAY_FOR_1ST_MOVE;
}

void Planner::resume_queuing() {
  if (PreciseStepping::stopping()) {
    // If stop_pending hasn't been processed yet, do so now before new moves are processed
    PreciseStepping::loop();
    assert(!PreciseStepping::stopping());
  }
  draining_buffer = false;
}

// Called from ISR
void Planner::endstop_triggered(const AxisEnum axis) {
  #if ENABLED(CRASH_RECOVERY)
    if (crash_s.is_active() && crash_s.is_enabled() && (axis == X_AXIS || axis == Y_AXIS)) {
      // endstop triggered: save the current planner state
      crash_s.axis_hit_isr(axis);
      if (crash_s.is_toolchange_in_progress()) {
        if (crash_s.get_state() == Crash_s::PRINTING) {
          crash_s.set_state(Crash_s::TRIGGERED_TOOLCRASH);
        }
        return; // Do not abort movement if crash happens during toolchange, will just home after toolchange
      } else{
        // Ignore repeated ISR trigger
        // It can happen if the endstop pin is still high while Z also triggers
        if (crash_s.get_state() != Crash_s::TRIGGERED_ISR) {
          crash_s.set_state(Crash_s::TRIGGERED_ISR);
        }
      }
    }
  #endif

  // Record stepper position and discard the current block
  stepper.endstop_triggered(axis);
}

float Planner::triggered_position_mm(const AxisEnum axis) {
  return stepper.triggered_position(axis) * mm_per_step[axis];
}

void Planner::finish_and_disable() {
  synchronize();
  if (!draining_buffer) disable_all_steppers();
}


/**
 * Attempt to get a coherent snapshot of stepper positions across axes
 * NOTE: suspending _just_ the stepper ISR can result in priority inversion.
 *   Instead of disabling all interrupts (and still risk missing a deadline)
 *   just _try_ to get coherent values when the ISR is running!
 *
 * @param pos output axis positions (steps)
 * @param cnt number of axes to sample (2 <= cnt <= LOGICAL_AXES)
 */
static void sample_stepper_positions(int32_t* pos, const uint8_t cnt) {
  constexpr uint8_t max_retry = 3;
  int32_t buf[LOGICAL_AXES];

  // initial sample
  for (uint8_t i = 0; i != cnt; ++i)
    pos[i] = stepper.position((AxisEnum)i);

  if (!STEPPER_ISR_ENABLED())
    return;

  // check for coherency
  for (uint8_t retry = 0; retry != max_retry; ++retry) {
    // refresh buffer
    for (uint8_t i = 0; i != cnt; ++i)
      buf[i] = stepper.position((AxisEnum)i);

    // check and update the initial sample
    bool unchanged = true;
    for (uint8_t i = 0; i != cnt; ++i) {
      if (pos[i] != buf[i]) {
        pos[i] = buf[i];
        unchanged = false;
      }
    }
    if (unchanged)
      break;
  }
}

/**
 * Get axis position according to stepper position(s)
 * For CORE machines apply translation from AB to XY.
 *
 * @param pos output axis positions (mm)
 * @param cnt number of axes to sample (2 <= cnt <= LOGICAL_AXES)
 */
static void get_multi_axis_position_mm(float* pos, const uint8_t cnt) {
  int32_t axis_steps[LOGICAL_AXES];
  sample_stepper_positions(axis_steps, cnt);

  #if IS_CORE
    #if CORE_IS_XY
      int32_t a = axis_steps[A_AXIS];
      int32_t b = axis_steps[B_AXIS];
      axis_steps[X_AXIS] = (a + b) * 0.5f;
      axis_steps[Y_AXIS] = CORESIGN(a - b) * 0.5f;
    #else
      #error "unsupported core type"
    #endif
  #endif

  for(uint8_t i = 0; i != cnt; ++i)
    pos[i] = axis_steps[i] * Planner::mm_per_step[i];
}

void Planner::get_axis_position_mm(ab_pos_t& pos) {
  get_multi_axis_position_mm(pos, 2);
}

void Planner::get_axis_position_mm(abc_pos_t& pos) {
  get_multi_axis_position_mm(pos, NUM_AXES);
}

void Planner::get_axis_position_mm(abce_pos_t& pos) {
  get_multi_axis_position_mm(pos, LOGICAL_AXES);
}

/**
 * Get XY axis position according to stepper position(s)
 * For CORE machines apply translation from AB to XY.
 */
float Planner::get_axis_position_mm(const AxisEnum axis) {
  float axis_steps;
  #if IS_CORE
    #if CORE_IS_XY
      // Requesting one of the "core" axes?
      if (axis == A_AXIS || axis == B_AXIS) {
        ab_pos_t pos;
        get_axis_position_mm(pos);
        return pos[axis];
      }
      else
        axis_steps = stepper.position(axis);
    #else
      #error "unsupported core type"
    #endif
  #else
    axis_steps = stepper.position(axis);
  #endif
  return axis_steps * mm_per_step[axis];
}


bool Planner::busy() {
  return !draining_buffer && (
    processing()
      #if ENABLED(EXTERNAL_CLOSED_LOOP_CONTROLLER)
        || (READ(CLOSED_LOOP_ENABLE_PIN) && !READ(CLOSED_LOOP_MOVE_COMPLETE_PIN))
      #endif
    );
}

/**
 * Block until all buffered steps are executed / cleaned
 */
void Planner::synchronize() {
  bool emptying_buffer_orig = emptying();
  emptying_buffer = true;
  while (busy()) idle(true);
  emptying_buffer = emptying_buffer_orig;
#if HAS_PHASE_STEPPING()
  phase_stepping::check_state();
#endif // HAS_PHASE_STEPPING()
}

/**
 * Planner::_buffer_msteps_raw
 *
 * Add a new linear movement to the buffer (in terms of steps) without implicit kinematic
 * translation, compensation or queuing restrictions.
 *
 *  target        - target position in mini-steps units
 *  fr_mm_s       - (target) speed of the move
 *  extruder      - target extruder
 *  millimeters   - the length of the movement, if known
 *
 * Returns true if movement was properly queued, false otherwise
 */
bool Planner::_buffer_msteps_raw(const xyze_long_t &target, const xyze_pos_t &target_float
  , feedRate_t fr_mm_s, const uint8_t extruder, const PlannerHints &hints
) {

  // Wait for the next available block
  uint8_t next_buffer_head;
  block_t * const block = get_next_free_block(next_buffer_head);
  if (!block) return false;

  // Mark the block as raw
  PlannerHints block_hints = hints;
  block_hints.raw_block = true;

  // Fill the block with the specified movement
  if (!_populate_block(block, target, target_float, fr_mm_s, extruder, block_hints)) {
    // Movement was not queued, probably because it was too short.
    //  Simply accept that as movement queued and done
    return true;
  }

  // Move buffer head
  block_buffer_head = next_buffer_head;

  // Recalculate and optimize trapezoidal speed profiles
  recalculate(TERN_(HINTS_SAFE_EXIT_SPEED, hints.safe_exit_speed_sqr));

  // Movement successfully queued!
  return true;
}

/**
 * @brief Add a new linear movement to the planner queue (in terms of steps).
 *
 * @param target        Target position in mini-steps units
 * @param target_float  Target position in direct (mm, degrees) units.
 * @param fr_mm_s       (target) speed of the move
 * @param extruder      target extruder
 * @param hints         parameters to aid planner calculations
 *
 * Returns true if movement was properly queued, false otherwise
 */
bool Planner::_buffer_msteps(const xyze_long_t &target, const xyze_pos_t &target_float
  , feedRate_t fr_mm_s, const uint8_t extruder, const PlannerHints &hints
) {

  // Wait for the next available block
  uint8_t next_buffer_head;
  block_t * const block = get_next_free_block(next_buffer_head);
  if (!block) return false;

  // Fill the block with the specified movement
  if (!_populate_block(block, target, target_float, fr_mm_s, extruder, hints)) {
    // Movement was not queued, probably because it was too short.
    //  Simply accept that as movement queued and done
    return true;
  }

  // If this is the first added movement, reload the delay, otherwise, cancel it.
  if (block_buffer_head == block_buffer_tail) {
    // If it was the first queued block, restart the 1st block delivery delay, to
    // give the planner an opportunity to queue more movements and plan them
    // As there are no queued movements, the Stepper ISR will not touch this
    // variable, so there is no risk setting this here (but it MUST be done
    // before the following line!!)
    delay_before_delivering = BLOCK_DELAY_FOR_1ST_MOVE;
  }

  // Move buffer head
  block_buffer_head = next_buffer_head;

  // Recalculate and optimize trapezoidal speed profiles
  recalculate(TERN_(HINTS_SAFE_EXIT_SPEED, hints.safe_exit_speed_sqr));

  // Movement successfully queued!
  return true;
}

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
bool Planner::_populate_block(block_t * const block,
  const abce_long_t &target, const xyze_pos_t &target_float
  , feedRate_t fr_mm_s, const uint8_t extruder, const PlannerHints &hints
) {
  const int32_t da = target.a - position.a,
                db = target.b - position.b,
                dc = target.c - position.c;

  #if EXTRUDERS
    int32_t de = target.e - position.e;
  #else
    constexpr int32_t de = 0;
  #endif

  /* <-- add a slash to enable
    SERIAL_ECHOLNPAIR("  _populate_block FR:", fr_mm_s,
                      " A:", target.a, " (", da, " msteps)"
                      " B:", target.b, " (", db, " msteps)"
                      " C:", target.c, " (", dc, " msteps)"
                      #if EXTRUDERS
                        " E:", target.e, " (", de, " msteps)"
                      #endif
                    );
  //*/

  #if EITHER(PREVENT_COLD_EXTRUSION, PREVENT_LENGTHY_EXTRUDE)
    if (de) {
      #if ENABLED(PREVENT_COLD_EXTRUSION)
        if (thermalManager.tooColdToExtrude(extruder)) {
          position.e = target.e; // Behave as if the move really took place, but ignore E part
          position_float.e = target_float.e;
          de = 0; // no difference
          SERIAL_ECHO_MSG(MSG_ERR_COLD_EXTRUDE_STOP);
        }
      #endif // PREVENT_COLD_EXTRUSION
      #if ENABLED(PREVENT_LENGTHY_EXTRUDE)
        const float e_msteps = ABS(de * e_factor[extruder]);
        const float max_e_msteps = settings.axis_msteps_per_mm[E_AXIS_N(extruder)] * (EXTRUDE_MAXLENGTH);
        if (e_msteps > max_e_msteps) {
          constexpr bool ignore_e = true;
          if (ignore_e) {
            position.e = target.e; // Behave as if the move really took place, but ignore E part
            position_float.e = target_float.e;
            de = 0; // no difference
            SERIAL_ECHO_MSG(MSG_ERR_LONG_EXTRUDE_STOP);
          }
        }
      #endif // PREVENT_LENGTHY_EXTRUDE
    }
  #endif // PREVENT_COLD_EXTRUSION || PREVENT_LENGTHY_EXTRUDE

  // Compute direction bit-mask for this block
  uint8_t dm = 0;
  if (da < 0) SBI(dm, X_AXIS);
  if (db < 0) SBI(dm, Y_AXIS);
  if (dc < 0) SBI(dm, Z_AXIS);
  if (de < 0) SBI(dm, E_AXIS);

  #if EXTRUDERS
    const float e_msteps_float = de * e_factor[extruder];
    const uint32_t e_msteps = ABS(e_msteps_float) + 0.5f;
  #else
    constexpr uint32_t e_msteps = 0;
  #endif

  // Clear all flags, including the "busy" bit
  block->flag.clear();
  block->busy = false;

  // Set direction bits
  block->direction_bits = dm;

  // Number of mini-steps for each axis
  // default non-h-bot planning
  block->msteps.set(ABS(da), ABS(db), ABS(dc));

  /**
   * This part of the code calculates the total length of the movement.
   * For cartesian bots, the X_AXIS is the real X movement and same for Y_AXIS.
   * But for corexy bots, that is not true. The "X_AXIS" and "Y_AXIS" motors (that should be named to A_AXIS
   * and B_AXIS) cannot be used for X and Y length, because A=X+Y and B=X-Y.
   * So we need to create other 2 "AXIS", named X_HEAD and Y_HEAD, meaning the real displacement of the Head.
   * Having the real displacement of the head, we can calculate the total movement length and apply the desired speed.
   */
  abce_float_t delta_mm;
  delta_mm.a = da * mm_per_mstep[A_AXIS];
  delta_mm.b = db * mm_per_mstep[B_AXIS];
  delta_mm.c = dc * mm_per_mstep[C_AXIS];

  #if EXTRUDERS
    delta_mm.e = e_msteps_float * mm_per_mstep[E_AXIS_N(extruder)];
  #endif

  if (!hints.raw_block NUM_AXIS_GANG(
      && block->msteps.a < MIN_MSTEPS_PER_SEGMENT, && block->msteps.b < MIN_MSTEPS_PER_SEGMENT, && block->msteps.c < MIN_MSTEPS_PER_SEGMENT,
      && block->msteps.i < MIN_MSTEPS_PER_SEGMENT, && block->msteps.j < MIN_MSTEPS_PER_SEGMENT, && block->msteps.k < MIN_MSTEPS_PER_SEGMENT,
      && block->msteps.u < MIN_MSTEPS_PER_SEGMENT, && block->msteps.v < MIN_MSTEPS_PER_SEGMENT, && block->msteps.w < MIN_MSTEPS_PER_SEGMENT
    )
  ) {
    block->millimeters = TERN0(HAS_EXTRUDERS, ABS(delta_mm.e));
  }
  else {
    if (hints.millimeters)
      block->millimeters = hints.millimeters;
    else
      block->millimeters = SQRT(sq(delta_mm.x) + sq(delta_mm.y) + sq(delta_mm.z));

    /**
     * At this point at least one of the axes has more mini-steps than
     * MIN_MSTEPS_PER_SEGMENT, ensuring the segment won't get dropped as
     * zero-length. It's important to not apply corrections
     * to blocks that would get dropped!
     *
     * A correction function is permitted to add steps to an axis, it
     * should *never* remove steps!
     */
    #if ENABLED(BACKLASH_COMPENSATION)
      if (!hints.raw_block) {
        backlash.add_correction_msteps(da, db, dc, dm, block);
      }
    #endif
  }

  #if EXTRUDERS
    block->msteps.e = e_msteps;
  #endif

  block->mstep_event_count = _MAX(block->msteps.a, block->msteps.b, block->msteps.c, e_msteps);

  if (!hints.raw_block) {
    // Bail if this is a regular short block
    if (block->mstep_event_count < MIN_MSTEPS_PER_SEGMENT)
      return false;
  } else if (!block->mstep_event_count) {
    // Bail if this is a zero-length block
    return false;
  }

  #if FAN_COUNT > 0
    FANS_LOOP(i) block->fan_speed[i] = thermalManager.fan_speed[i];
  #endif

  #if EXTRUDERS > 1
    block->extruder = extruder;
  #endif

  #if ENABLED(AUTO_POWER_CONTROL)
    if (block->msteps.x || block->msteps.y || block->msteps.z)
      powerManager.power_on();
  #endif

  // Enable active axes
  #if CORE_IS_XY
    if (block->msteps.a || block->msteps.b) {
      enable_XY();
    }
    #if DISABLED(Z_LATE_ENABLE)
      if (block->msteps.z) enable_Z();
    #endif
  #elif CORE_IS_XZ
    if (block->msteps.a || block->msteps.c) {
      enable_X();
      enable_Z();
    }
    if (block->msteps.y) enable_Y();
  #elif CORE_IS_YZ
    if (block->msteps.b || block->msteps.c) {
      enable_Y();
      enable_Z();
    }
    if (block->msteps.x) enable_X();
  #else
    #if ENABLED(XY_LINKED_ENABLE)
      if (block->msteps.x || block->msteps.y) enable_XY();
    #else
      if (block->msteps.x) enable_X();
      if (block->msteps.y) enable_Y();
    #endif
    #if DISABLED(Z_LATE_ENABLE)
      if (block->msteps.z) enable_Z();
    #endif
  #endif

  // Enable extruder(s)
  #if EXTRUDERS
    if (e_msteps) {
      #if ENABLED(AUTO_POWER_CONTROL)
        powerManager.power_on();
      #endif

      #if ENABLED(DISABLE_INACTIVE_EXTRUDER) // Enable only the selected extruder

        #define DISABLE_IDLE_E(N) if (!g_uc_extruder_last_move[N]) disable_E##N();

        for (uint8_t i = 0; i < EXTRUDERS; i++)
          if (g_uc_extruder_last_move[i] > 0) g_uc_extruder_last_move[i]--;

        switch (extruder) {
          case 0:
            #if EXTRUDERS > 1
              DISABLE_IDLE_E(1);
              #if EXTRUDERS > 2
                DISABLE_IDLE_E(2);
                #if EXTRUDERS > 3
                  DISABLE_IDLE_E(3);
                  #if EXTRUDERS > 4
                    DISABLE_IDLE_E(4);
                    #if EXTRUDERS > 5
                      DISABLE_IDLE_E(5);
                    #endif // EXTRUDERS > 5
                  #endif // EXTRUDERS > 4
                #endif // EXTRUDERS > 3
              #endif // EXTRUDERS > 2
            #endif // EXTRUDERS > 1
            enable_E0();
            g_uc_extruder_last_move[0] = (BLOCK_BUFFER_SIZE) * 2;
            #if HAS_DUPLICATION_MODE
              if (extruder_duplication_enabled) {
                enable_E1();
                g_uc_extruder_last_move[1] = (BLOCK_BUFFER_SIZE) * 2;
              }
            #endif
          break;
          #if EXTRUDERS > 1
            case 1:
              DISABLE_IDLE_E(0);
              #if EXTRUDERS > 2
                DISABLE_IDLE_E(2);
                #if EXTRUDERS > 3
                  DISABLE_IDLE_E(3);
                  #if EXTRUDERS > 4
                    DISABLE_IDLE_E(4);
                    #if EXTRUDERS > 5
                      DISABLE_IDLE_E(5);
                    #endif // EXTRUDERS > 5
                  #endif // EXTRUDERS > 4
                #endif // EXTRUDERS > 3
              #endif // EXTRUDERS > 2
              enable_E1();
              g_uc_extruder_last_move[1] = (BLOCK_BUFFER_SIZE) * 2;
            break;
            #if EXTRUDERS > 2
              case 2:
                DISABLE_IDLE_E(0);
                DISABLE_IDLE_E(1);
                #if EXTRUDERS > 3
                  DISABLE_IDLE_E(3);
                  #if EXTRUDERS > 4
                    DISABLE_IDLE_E(4);
                    #if EXTRUDERS > 5
                      DISABLE_IDLE_E(5);
                    #endif
                  #endif
                #endif
                enable_E2();
                g_uc_extruder_last_move[2] = (BLOCK_BUFFER_SIZE) * 2;
              break;
              #if EXTRUDERS > 3
                case 3:
                  DISABLE_IDLE_E(0);
                  DISABLE_IDLE_E(1);
                  DISABLE_IDLE_E(2);
                  #if EXTRUDERS > 4
                    DISABLE_IDLE_E(4);
                    #if EXTRUDERS > 5
                      DISABLE_IDLE_E(5);
                    #endif
                  #endif
                  enable_E3();
                  g_uc_extruder_last_move[3] = (BLOCK_BUFFER_SIZE) * 2;
                break;
                #if EXTRUDERS > 4
                  case 4:
                    DISABLE_IDLE_E(0);
                    DISABLE_IDLE_E(1);
                    DISABLE_IDLE_E(2);
                    DISABLE_IDLE_E(3);
                    #if EXTRUDERS > 5
                      DISABLE_IDLE_E(5);
                    #endif
                    enable_E4();
                    g_uc_extruder_last_move[4] = (BLOCK_BUFFER_SIZE) * 2;
                  break;
                  #if EXTRUDERS > 5
                    case 5:
                      DISABLE_IDLE_E(0);
                      DISABLE_IDLE_E(1);
                      DISABLE_IDLE_E(2);
                      DISABLE_IDLE_E(3);
                      DISABLE_IDLE_E(4);
                      enable_E5();
                      g_uc_extruder_last_move[5] = (BLOCK_BUFFER_SIZE) * 2;
                    break;
                  #endif // EXTRUDERS > 5
                #endif // EXTRUDERS > 4
              #endif // EXTRUDERS > 3
            #endif // EXTRUDERS > 2
          #endif // EXTRUDERS > 1
        }
      #else
        enable_E0();
        enable_E1();
        enable_E2();
        enable_E3();
        enable_E4();
        enable_E5();
      #endif
    }
  #endif // EXTRUDERS

  if (e_msteps)
    NOLESS(fr_mm_s, settings.min_feedrate_mm_s);
  else
    NOLESS(fr_mm_s, settings.min_travel_feedrate_mm_s);

  const float inverse_millimeters = 1.0f / block->millimeters;  // Inverse millimeters to remove multiple divides

  // Calculate inverse time for this move. No divide by zero due to previous checks.
  // Example: At 120mm/s a 60mm move takes 0.5s. So this will give 2.0.
  float inverse_secs = fr_mm_s * inverse_millimeters;

  // Get the number of non busy movements in queue (non busy means that they can be altered)
  const uint8_t moves_queued = nonbusy_movesplanned();

  // Slow down when the buffer starts to empty, rather than wait at the corner for a buffer refill
  #if EITHER(SLOWDOWN, ULTRA_LCD) || defined(XY_FREQUENCY_LIMIT)
    // Segment time in microseconds
    uint32_t segment_time_us = LROUND(1000000.0f / inverse_secs);
  #endif

  #if ENABLED(SLOWDOWN)
    #ifndef SLOWDOWN_DIVISOR
      #define SLOWDOWN_DIVISOR 2
    #endif
    // Take into account also blocks that are just waiting to be discarded because even those blocks
    // can't be modified, those blocks still aren't processed by PreciseStepping::process_one_step_event_from_queue().
    const uint8_t total_blocks_queued = movesplanned();

    // Do not slowdown when implicitly stopping and/or when the queue still contains at least one command
    if (!emptying() && queue.length <= 3 && WITHIN(total_blocks_queued, 2, (BLOCK_BUFFER_SIZE) / (SLOWDOWN_DIVISOR) - 1)) {
      const int32_t time_diff = settings.min_segment_time_us - segment_time_us;
      if (time_diff > 0) {
        // Buffer is draining so add extra time. The amount of time added increases if the buffer is still emptied more.
        const uint32_t nst = segment_time_us + LROUND(2 * time_diff / total_blocks_queued);
        inverse_secs = 1000000.0f / nst;
        #if defined(XY_FREQUENCY_LIMIT) || HAS_SPI_LCD
          segment_time_us = nst;
        #endif
      }
    }
  #endif

  block->nominal_speed = block->millimeters * inverse_secs;           // (mm/sec) Always > 0
#if ENABLED(S_CURVE_ACCELERATION)
  block->nominal_rate = CEIL(block->mstep_event_count * inverse_secs); // (mini-step/sec) Always > 0
#endif

  #if ENABLED(FILAMENT_WIDTH_SENSOR)
    if (extruder == FILAMENT_SENSOR_EXTRUDER_NUM)   // Only for extruder with filament sensor
      filwidth.advance_e(delta_mm.e);
  #endif

  // Calculate and limit speed in mm/sec for each axis
  xyze_float_t current_speed;
  float speed_factor = 1.0f; // factor <1 decreases speed

  #ifdef COREXY_CONVERT_LIMITS
    const float speed_mm_x = std::abs(current_speed[X_AXIS] = delta_mm[X_AXIS] * inverse_secs);
    const float speed_mm_y = std::abs(current_speed[Y_AXIS] = delta_mm[Y_AXIS] * inverse_secs);
    const feedRate_t highest_strain = (speed_mm_x + speed_mm_y) * 0.5f;
    
    const float max_feedrate_mm_s = settings.max_feedrate_mm_s[X_AXIS];
    if(highest_strain > max_feedrate_mm_s) {
      NOMORE(speed_factor, max_feedrate_mm_s / highest_strain);
    }
  #else
    LOOP_XY(i) {
      const float delta_mm_i = delta_mm[i];
      const feedRate_t cs = ABS(current_speed[i] = delta_mm_i * inverse_secs);
      if (cs > settings.max_feedrate_mm_s[i]) NOMORE(speed_factor, settings.max_feedrate_mm_s[i] / cs);
    }
  #endif

  LOOP_S_LE_N(i, Z_AXIS, E_AXIS) {
    const float delta_mm_i = delta_mm[i];
    const feedRate_t cs = ABS(current_speed[i] = delta_mm_i * inverse_secs);
    if (cs > settings.max_feedrate_mm_s[i]) NOMORE(speed_factor, settings.max_feedrate_mm_s[i] / cs);
    #if ENABLED(DISTINCT_E_FACTORS)
      if (i == E_AXIS) i += extruder;
    #endif
  }

  // Max segment time in µs.
  #ifdef XY_FREQUENCY_LIMIT

    // Check and limit the xy direction change frequency
    const unsigned char direction_change = block->direction_bits ^ old_direction_bits;
    old_direction_bits = block->direction_bits;
    segment_time_us = LROUND((float)segment_time_us / speed_factor);

    uint32_t xs0 = axis_segment_time_us[0].x,
             xs1 = axis_segment_time_us[1].x,
             xs2 = axis_segment_time_us[2].x,
             ys0 = axis_segment_time_us[0].y,
             ys1 = axis_segment_time_us[1].y,
             ys2 = axis_segment_time_us[2].y;

    if (TEST(direction_change, X_AXIS)) {
      xs2 = axis_segment_time_us[2].x = xs1;
      xs1 = axis_segment_time_us[1].x = xs0;
      xs0 = 0;
    }
    xs0 = axis_segment_time_us[0].x = xs0 + segment_time_us;

    if (TEST(direction_change, Y_AXIS)) {
      ys2 = axis_segment_time_us[2].y = axis_segment_time_us[1].y;
      ys1 = axis_segment_time_us[1].y = axis_segment_time_us[0].y;
      ys0 = 0;
    }
    ys0 = axis_segment_time_us[0].y = ys0 + segment_time_us;

    const uint32_t max_x_segment_time = _MAX(xs0, xs1, xs2),
                   max_y_segment_time = _MAX(ys0, ys1, ys2),
                   min_xy_segment_time = _MIN(max_x_segment_time, max_y_segment_time);
    if (min_xy_segment_time < MAX_FREQ_TIME_US) {
      const float low_sf = speed_factor * min_xy_segment_time / (MAX_FREQ_TIME_US);
      NOMORE(speed_factor, low_sf);
    }
  #endif // XY_FREQUENCY_LIMIT

  // Correct the speed
  if (speed_factor < 1.0f) {
    current_speed *= speed_factor;
  #if ENABLED(S_CURVE_ACCELERATION)
    block->nominal_rate *= speed_factor;
  #endif
    block->nominal_speed *= speed_factor;
  }

  // Compute and limit the acceleration rate for the trapezoid generator.
  const float msteps_per_mm = block->mstep_event_count * inverse_millimeters;
  uint32_t accel;
  if (!block->msteps.a && !block->msteps.b && !block->msteps.c) {
    // convert to: acceleration steps/sec^2
    accel = CEIL(settings.retract_acceleration * msteps_per_mm);
  }
  else {
    #define LIMIT_ACCEL_LONG(AXIS,INDX) do{ \
      if (block->msteps[AXIS] && max_acceleration_msteps_per_s2[AXIS+INDX] < accel) { \
        const uint32_t comp = max_acceleration_msteps_per_s2[AXIS+INDX] * block->mstep_event_count; \
        if (accel * block->msteps[AXIS] > comp) accel = comp / block->msteps[AXIS]; \
      } \
    }while(0)

    #define LIMIT_ACCEL_FLOAT(AXIS,INDX) do{ \
      if (block->msteps[AXIS] && max_acceleration_msteps_per_s2[AXIS+INDX] < accel) { \
        const float comp = (float)max_acceleration_msteps_per_s2[AXIS+INDX] * (float)block->mstep_event_count; \
        if ((float)accel * (float)block->msteps[AXIS] > comp) accel = comp / (float)block->msteps[AXIS]; \
      } \
    }while(0)

    // Start with print or travel acceleration
    accel = CEIL((e_msteps ? settings.acceleration : settings.travel_acceleration) * msteps_per_mm);

    #if ENABLED(DISTINCT_E_FACTORS)
      #define ACCEL_IDX extruder
    #else
      #define ACCEL_IDX 0
    #endif

    // Limit acceleration per axis
    if (block->mstep_event_count <= cutoff_long) {
      LIMIT_ACCEL_LONG(A_AXIS, 0);
      LIMIT_ACCEL_LONG(B_AXIS, 0);
      LIMIT_ACCEL_LONG(C_AXIS, 0);
      LIMIT_ACCEL_LONG(E_AXIS, ACCEL_IDX);
    }
    else {
      LIMIT_ACCEL_FLOAT(A_AXIS, 0);
      LIMIT_ACCEL_FLOAT(B_AXIS, 0);
      LIMIT_ACCEL_FLOAT(C_AXIS, 0);
      LIMIT_ACCEL_FLOAT(E_AXIS, ACCEL_IDX);
    }
  }
#if ENABLED(S_CURVE_ACCELERATION)
  block->acceleration_msteps_per_s2 = accel;
#endif
  block->acceleration = accel / msteps_per_mm;
  float vmax_junction_sqr; // Initial limit on the segment entry velocity (mm/s)^2

  #if DISABLED(CLASSIC_JERK)
    /**
     * Compute maximum allowable entry speed at junction by centripetal acceleration approximation.
     * Let a circle be tangent to both previous and current path line segments, where the junction
     * deviation is defined as the distance from the junction to the closest edge of the circle,
     * colinear with the circle center. The circular segment joining the two paths represents the
     * path of centripetal acceleration. Solve for max velocity based on max acceleration about the
     * radius of the circle, defined indirectly by junction deviation. This may be also viewed as
     * path width or max_jerk in the previous Grbl version. This approach does not actually deviate
     * from path, but used as a robust way to compute cornering speeds, as it takes into account the
     * nonlinearities of both the junction angle and junction velocity.
     *
     * NOTE: If the junction deviation value is finite, Grbl executes the motions in an exact path
     * mode (G61). If the junction deviation value is zero, Grbl will execute the motion in an exact
     * stop mode (G61.1) manner. In the future, if continuous mode (G64) is desired, the math here
     * is exactly the same. Instead of motioning all the way to junction point, the machine will
     * just follow the arc circle defined here. The Arduino doesn't have the CPU cycles to perform
     * a continuous mode path, but ARM-based microcontrollers most certainly do.
     *
     * NOTE: The max junction speed is a fixed value, since machine acceleration limits cannot be
     * changed dynamically during operation nor can the line move geometry. This must be kept in
     * memory in the event of a feedrate override changing the nominal speeds of blocks, which can
     * change the overall maximum entry speed conditions of all blocks.
     *
     * #######
     * https://github.com/MarlinFirmware/Marlin/issues/10341#issuecomment-388191754
     *
     * hoffbaked: on May 10 2018 tuned and improved the GRBL algorithm for Marlin:
          Okay! It seems to be working good. I somewhat arbitrarily cut it off at 1mm
          on then on anything with less sides than an octagon. With this, and the
          reverse pass actually recalculating things, a corner acceleration value
          of 1000 junction deviation of .05 are pretty reasonable. If the cycles
          can be spared, a better acos could be used. For all I know, it may be
          already calculated in a different place. */

    // Unit vector of previous path line segment
    static xyze_float_t prev_unit_vec;
    xyze_float_t unit_vec = target_float - position_float;

    /**
     * On CoreXY the length of the vector [A,B] is SQRT(2) times the length of the head movement vector [X,Y].
     * So taking Z and E into account, we cannot scale to a unit vector with "inverse_millimeters".
     * => normalize the complete junction vector
     * Also always normalize when float position is not available and there is E component.
     */
    if (ENABLED(IS_CORE))
      normalize_junction_vector(unit_vec);
    else
      unit_vec *= inverse_millimeters;

    // Skip first block or when previous_nominal_speed is used as a flag for homing and offset cycles.
    if (moves_queued && !UNEAR_ZERO(previous_nominal_speed)) {
      // Compute cosine of angle between previous and current path. (prev_unit_vec is negative)
      // NOTE: Max junction velocity is computed without sin() or acos() by trig half angle identity.
      float junction_cos_theta = (-prev_unit_vec.x * unit_vec.x) + (-prev_unit_vec.y * unit_vec.y)
                               + (-prev_unit_vec.z * unit_vec.z) + (-prev_unit_vec.e * unit_vec.e);
      #if ENABLED(JD_DEBUG_OUTPUT)
        SERIAL_ECHO_F(junction_cos_theta, 7);
      #endif

      // NOTE: Computed without any expensive trig, sin() or acos(), by trig half angle identity of cos(theta).
      if (junction_cos_theta > 0.999999f) {
        // For a 0 degree acute junction, just set minimum junction speed.
        vmax_junction_sqr = sq(float(MINIMUM_PLANNER_SPEED));
      }
      else {
        NOLESS(junction_cos_theta, -0.999999f); // Check for numerical round-off to avoid divide by zero.

        // Convert delta vector to unit vector
        xyze_float_t junction_unit_vec = unit_vec - prev_unit_vec;
        normalize_junction_vector(junction_unit_vec);

        const float junction_acceleration = limit_value_by_axis_maximum(block->acceleration, junction_unit_vec),
                    sin_theta_d2 = SQRT(0.5f * (1.0f - junction_cos_theta)); // Trig half angle identity. Always positive.

        vmax_junction_sqr = (junction_acceleration * junction_deviation_mm * sin_theta_d2) / (1.0f - sin_theta_d2);
        #if ENABLED(JD_SMALL_SEGMENT_HANDLING)
          if (block->millimeters < 1) {

            // Fast acos approximation, minus the error bar to be safe
            const float junction_theta = (RADIANS(-40) * sq(junction_cos_theta) - RADIANS(50)) * junction_cos_theta + RADIANS(90) - 0.18f;

            // If angle is greater than 135 degrees (octagon), find speed for approximate arc
            if (junction_theta > RADIANS(135)) {
              const float limit_sqr = block->millimeters / (RADIANS(180) - junction_theta) * junction_acceleration;
              NOMORE(vmax_junction_sqr, limit_sqr);
            }
          }
        #endif //JD_SMALL_SEGMENT_HANDLING

      }

      // Get the lowest speed
      vmax_junction_sqr = _MIN(vmax_junction_sqr, sq(block->nominal_speed), sq(previous_nominal_speed));
    }
    else // Init entry speed to zero. Assume it starts from rest. Planner will correct this later.
      vmax_junction_sqr = 0;

    prev_unit_vec = unit_vec;

  #endif

  #if HAS_CLASSIC_JERK

    /**
     * Adapted from Průša MKS firmware
     * https://github.com/prusa3d/Prusa-Firmware
     */
    // Exit speed limited by a jerk to full halt of a previous last segment
    static float previous_safe_speed;

    // Start with a safe speed (from which the machine may halt to stop immediately).
    float safe_speed = block->nominal_speed;

    uint8_t limited = 0;
    #if HAS_LINEAR_E_JERK
      LOOP_XYZ(i)
    #else
      LOOP_XYZE(i)
    #endif
    {
      const float jerk = ABS(current_speed[i]),   // cs : Starting from zero, change in speed for this axis
                  maxj = settings.max_jerk[i];             // mj : The max jerk setting for this axis
      if (jerk > maxj) {                          // cs > mj : New current speed too fast?
        if (limited) {                            // limited already?
          const float mjerk = block->nominal_speed * maxj; // ns*mj
          if (jerk * safe_speed > mjerk) safe_speed = mjerk / jerk; // ns*mj/cs
        }
        else {
          safe_speed *= maxj / jerk;              // Initial limit: ns*mj/cs
          ++limited;                              // Initially limited
        }
      }
    }

    float vmax_junction;
    if (moves_queued && !UNEAR_ZERO(previous_nominal_speed)) {
      // Estimate a maximum velocity allowed at a joint of two successive segments.
      // If this maximum velocity allowed is lower than the minimum of the entry / exit safe velocities,
      // then the machine is not coasting anymore and the safe entry / exit velocities shall be used.

      // Factor to multiply the previous / current nominal velocities to get componentwise limited velocities.
      float v_factor = 1;
      limited = 0;

      // The junction velocity will be shared between successive segments. Limit the junction velocity to their minimum.
      // Pick the smaller of the nominal speeds. Higher speed shall not be achieved at the junction during coasting.
      vmax_junction = _MIN(block->nominal_speed, previous_nominal_speed);

      // Now limit the jerk in all axes.
      const float smaller_speed_factor = vmax_junction / previous_nominal_speed;
      #if HAS_LINEAR_E_JERK
        LOOP_XYZ(axis)
      #else
        LOOP_XYZE(axis)
      #endif
      {
        // Limit an axis. We have to differentiate: coasting, reversal of an axis, full stop.
        float v_exit = previous_speed[axis] * smaller_speed_factor,
              v_entry = current_speed[axis];
        if (limited) {
          v_exit *= v_factor;
          v_entry *= v_factor;
        }

        // Calculate jerk depending on whether the axis is coasting in the same direction or reversing.
        const float jerk = (v_exit > v_entry)
            ? //                                  coasting             axis reversal
              ( (v_entry > 0 || v_exit < 0) ? (v_exit - v_entry) : _MAX(v_exit, -v_entry) )
            : // v_exit <= v_entry                coasting             axis reversal
              ( (v_entry < 0 || v_exit > 0) ? (v_entry - v_exit) : _MAX(-v_exit, v_entry) );

        if (jerk > settings.max_jerk[axis]) {
          v_factor *= settings.max_jerk[axis] / jerk;
          ++limited;
        }
      }
      if (limited) vmax_junction *= v_factor;
      // Now the transition velocity is known, which maximizes the shared exit / entry velocity while
      // respecting the jerk factors, it may be possible, that applying separate safe exit / entry velocities will achieve faster prints.
      const float vmax_junction_threshold = vmax_junction * 0.99f;
      if (previous_safe_speed > vmax_junction_threshold && safe_speed > vmax_junction_threshold)
        vmax_junction = safe_speed;
    }
    else
      vmax_junction = safe_speed;

    previous_safe_speed = safe_speed;

    #if DISABLED(CLASSIC_JERK)
      vmax_junction_sqr = _MIN(vmax_junction_sqr, sq(vmax_junction));
    #else
      vmax_junction_sqr = sq(vmax_junction);
    #endif

  #endif // Classic Jerk Limiting

  // Max entry speed of this block equals the max exit speed of the previous block.
  #if ENABLED(JD_DEBUG_OUTPUT)
    SERIAL_ECHO(" ");
    SERIAL_ECHO(vmax_junction_sqr);
    SERIAL_EOL();
  #endif
  block->max_entry_speed_sqr = vmax_junction_sqr;

  // Initialize block entry speed. Compute based on deceleration to user-defined MINIMUM_PLANNER_SPEED.
  const float v_allowable_sqr = max_allowable_speed_sqr(-block->acceleration, sq(float(MINIMUM_PLANNER_SPEED)), block->millimeters);

  // Start with the minimum allowed speed
  block->entry_speed_sqr = sq(float(MINIMUM_PLANNER_SPEED));

  // Initialize planner efficiency flags
  // Set flag if block will always reach maximum junction speed regardless of entry/exit speeds.
  // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
  // the current block and next block junction speeds are guaranteed to always be at their maximum
  // junction speeds in deceleration and acceleration, respectively. This is due to how the current
  // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
  // the reverse and forward planners, the corresponding block junction speed will always be at the
  // the maximum junction speed and may always be ignored for any speed reduction checks.
  block->flag.set_nominal(sq(block->nominal_speed) <= v_allowable_sqr);

  // Update previous path unit_vector and nominal speed
  previous_speed = current_speed;
  previous_nominal_speed = block->nominal_speed;

  #if ENABLED(CRASH_RECOVERY)
  {
    const uint8_t crash_index = block - block_buffer;
    Crash_s::crash_block_t &crash_block = crash_s.crash_block[crash_index];
    auto &move_start = crash_s.move_start;

    // save recovery data for the current block
    crash_block.start_current_position = move_start.start_current_position;
    crash_block.e_position = position_float[E_AXIS];
    crash_block.e_msteps = de;
    crash_block.sdpos = move_start.sdpos;
    crash_block.segment_idx = crash_s.gcode_state.segment_idx;
    crash_block.inhibit_flags = crash_s.gcode_state.inhibit_flags;
    crash_block.fr_mm_s = fr_mm_s;
  }
  #endif

  // Update the position
  position = target;
  position_float = target_float;

  // Movement was accepted
  return true;
} // _populate_block()

/**
 * Planner::buffer_sync_block
 * Add a block to the buffer that just updates the position
 */
void Planner::buffer_sync_block() {
  // Wait for the next available block
  uint8_t next_buffer_head;
  block_t * const block = get_next_free_block(next_buffer_head);
  if (!block) return;

  // Clear block
  block->reset();
  block->flag.apply(BLOCK_BIT_SYNC_POSITION);

  // Convert current mini-steps to absolute step count
  block->sync_step_position = position / PLANNER_STEPS_MULTIPLIER;

  // If this is the first added movement, reload the delay, otherwise, cancel it.
  if (block_buffer_head == block_buffer_tail) {
    // If it was the first queued block, restart the 1st block delivery delay, to
    // give the planner an opportunity to queue more movements and plan them
    // As there are no queued movements, the Stepper ISR will not touch this
    // variable, so there is no risk setting this here (but it MUST be done
    // before the following line!!)
    delay_before_delivering = BLOCK_DELAY_FOR_1ST_MOVE;
  }

  block_buffer_head = next_buffer_head;

  stepper.wake_up();
} // buffer_sync_block()

/**
 * @brief Add a single linear movement
 *
 * @description Add a new linear movement to the buffer in axis units.
 *              Leveling and kinematics should be applied before calling this.
 *
 * @param a,b,c,e       Target positions in mm and/or degrees
 * @param fr_mm_s       (target) speed of the move
 * @param extruder      optional target extruder (otherwise active_extruder)
 * @param hints         optional parameters to aid planner calculations
 */
bool Planner::buffer_segment(const abce_pos_t &abce
  , const_feedRate_t fr_mm_s
  , const uint8_t extruder/*=active_extruder*/
  , const PlannerHints &hints/*=PlannerHints()*/
) {

  // If we are aborting, do not accept queuing of movements
  if (draining_buffer || PreciseStepping::stopping()) return false;

  #if ENABLED(CRASH_RECOVERY)
  // Hints for the current segments might be reset during recovery
  const PlannerHints* segment_hints = &hints;

  {
    auto &move_start = crash_s.move_start;
    auto &gcode_state = crash_s.gcode_state;
    if (gcode_state.sdpos == move_start.sdpos) {
      ++gcode_state.segment_idx;
    } else {
      // we are processing the beginning of a new logical move: update the constant
      // values which are repeated in all subsequent segments
      move_start.start_current_position = current_position;

      // reset segment state
      move_start.sdpos = gcode_state.sdpos;
      gcode_state.segment_idx = 0;
    }

    if (crash_s.get_state() == Crash_s::REPLAY) {
      // replay mode: drop initial segments
      if (crash_s.segments_finished > 0) {
        --crash_s.segments_finished;
        return true;
      }

      // first real segment after recovering, manipulate the current state in order
      // to resume the segment from the crashing position
      set_machine_position_mm(crash_s.crash_position);

      // reset the hints
      static const PlannerHints default_hints;
      segment_hints = &default_hints;

      // continue normally
      crash_s.set_state(Crash_s::PRINTING);
    }
  }
  #endif

  // When changing extruders recalculate mini-steps corresponding to the E position
  #if ENABLED(DISTINCT_E_FACTORS)
    if (last_extruder != extruder && settings.axis_msteps_per_mm[E_AXIS_N(extruder)] != settings.axis_msteps_per_mm[E_AXIS_N(last_extruder)]) {
      position.e = LROUND(position.e * settings.axis_msteps_per_mm[E_AXIS_N(extruder)] * mm_per_mstep[E_AXIS_N(last_extruder)]);
      last_extruder = extruder;
    }
  #endif

  // The target position of the tool in absolute mini-steps
  // Calculate target position in absolute mini-steps
  const abce_long_t target = {
    int32_t(LROUND(abce.a * settings.axis_msteps_per_mm[A_AXIS])),
    int32_t(LROUND(abce.b * settings.axis_msteps_per_mm[B_AXIS])),
    int32_t(LROUND(abce.c * settings.axis_msteps_per_mm[C_AXIS])),
    int32_t(LROUND(abce.e * settings.axis_msteps_per_mm[E_AXIS_N(extruder)]))
  };

  // DRYRUN prevents E moves from taking place
  if (DEBUGGING(DRYRUN) || TERN0(CANCEL_OBJECTS, cancelable.skipping)) {
    position.e = target.e;
    position_float.e = abce.e;
  }

  /* <-- add a slash to enable
    SERIAL_ECHOPAIR("  buffer_segment FR:", fr_mm_s);
    SERIAL_ECHOPAIR(" X:", a);
    SERIAL_ECHOPAIR(" (", position.x);
    SERIAL_ECHOPAIR("->", target.x);
    SERIAL_ECHOPAIR(") Y:", b);
    SERIAL_ECHOPAIR(" (", position.y);
    SERIAL_ECHOPAIR("->", target.y);
    SERIAL_ECHOPAIR(") Z:", c);
    SERIAL_ECHOPAIR(" (", position.z);
    SERIAL_ECHOPAIR("->", target.z);
    SERIAL_ECHOPAIR(") E:", e);
    SERIAL_ECHOPAIR(" (", position.e);
    SERIAL_ECHOPAIR("->", target.e);
    SERIAL_ECHOLNPGM(")");
  //*/

  // Queue the movement. Return 'false' if the move was not queued.
  if (!_buffer_msteps(target, abce
      , fr_mm_s, extruder
#if ENABLED(CRASH_RECOVERY)
      , *segment_hints
#else
      , hints
#endif
  )) return false;

  stepper.wake_up();
  return true;
} // buffer_segment()

/**
 * Add a new linear movement to the buffer.
 * The target is cartesian.
 *
 *  rx,ry,rz,e      - target position in mm or degrees
 *  fr_mm_s         - (target) speed of the move (mm/s)
 *  extruder        - optional target extruder (otherwise active_extruder)
 *  hints           - optional parameters to aid planner calculations
 */
bool Planner::buffer_line(const xyze_pos_t &cart, const_feedRate_t fr_mm_s
  , const uint8_t extruder/*=active_extruder*/
  , const PlannerHints &hints/*=PlannerHints()*/
) {
  xyze_pos_t machine = cart;
  TERN_(HAS_POSITION_MODIFIERS, apply_modifiers(machine));
  return buffer_segment(machine, fr_mm_s, extruder, hints);
} // buffer_line()

/**
 * Directly set the planner ABC position (and stepper positions)
 * converting mm into mini-steps.
 *
 * The provided ABC position is in machine units.
 */

void Planner::set_machine_position_mm(const abce_pos_t &abce) {
  #if ENABLED(DISTINCT_E_FACTORS)
    last_extruder = active_extruder;
  #endif
  position_float = abce;
  position.set(LROUND(abce.a * settings.axis_msteps_per_mm[A_AXIS]),
               LROUND(abce.b * settings.axis_msteps_per_mm[B_AXIS]),
               LROUND(abce.c * settings.axis_msteps_per_mm[C_AXIS]),
               LROUND(abce.e * settings.axis_msteps_per_mm[E_AXIS_N(active_extruder)]));

  if (processing()) {
    //previous_nominal_speed = 0.0f; // Reset planner junction speeds. Assume start from rest.
    //previous_speed.reset();
    buffer_sync_block();
  } else {
    const xyze_long_t stepper_position = { LROUND(abce.a * settings.axis_steps_per_mm[A_AXIS]),
                                           LROUND(abce.b * settings.axis_steps_per_mm[B_AXIS]),
                                           LROUND(abce.c * settings.axis_steps_per_mm[C_AXIS]),
                                           LROUND(abce.e * settings.axis_steps_per_mm[E_AXIS_N(active_extruder)]) };
    stepper.set_position(stepper_position);
  }
}

void Planner::set_position_mm(const xyze_pos_t &xyze) {
  xyze_pos_t machine = xyze;
  TERN_(HAS_POSITION_MODIFIERS, apply_modifiers(machine, true));
  set_machine_position_mm(machine);
}

/**
 * Setters for planner position (also setting stepper position).
 */
void Planner::set_e_position_mm(const_float_t e) {
  const uint8_t axis_index = E_AXIS_N(active_extruder);
  #if ENABLED(DISTINCT_E_FACTORS)
    last_extruder = active_extruder;
  #endif
  #if ENABLED(FWRETRACT)
    float e_new = e - fwretract.current_retract[active_extruder];
  #else
    const float e_new = e;
  #endif
  position.e = LROUND(settings.axis_msteps_per_mm[axis_index] * e_new);
  position_float.e = e_new;

  if (processing())
    buffer_sync_block();
  else
    stepper.set_axis_position(E_AXIS, LROUND(settings.axis_steps_per_mm[axis_index] * e_new));
}

void Planner::reset_position() {
#if ANY(IS_CORE, MARKFORGED_XY, MARKFORGED_YX)
  #if ENABLED(CORE_IS_XY)
    // XY position
    int32_t a = stepper.position(A_AXIS);
    int32_t b = stepper.position(B_AXIS);
    float x = static_cast<float>(a + b) / 2.f;
    float y = static_cast<float>(CORESIGN(a - b)) / 2.f;
    position[0] = LROUND(x * PLANNER_STEPS_MULTIPLIER);
    position[1] = LROUND(y * PLANNER_STEPS_MULTIPLIER);
    position_float[0] = x / settings.axis_steps_per_mm[0];
    position_float[1] = y / settings.axis_steps_per_mm[1];

    // remaining axes
    LOOP_S_L_N(i, C_AXIS, XYZE_N) {
      const int32_t stepper_position_i = stepper.position((AxisEnum)i);
      position[i] = stepper_position_i * PLANNER_STEPS_MULTIPLIER;
      position_float[i] = stepper_position_i / settings.axis_steps_per_mm[i];
    }
  #else
    #error "reset_position() not implemented for this kinematic"
  #endif
#else
  // cartesian
  LOOP_ABCE(i) {
    position[i] = stepper.position((AxisEnum)i) * PLANNER_STEPS_MULTIPLIER;
    position_float[i] = position[i] / settings.axis_msteps_per_mm[i];
  }
#endif
}

// Recalculate the mini-steps/s^2 acceleration rates, based on the mm/s^2
void Planner::refresh_acceleration_rates() {
  #if ENABLED(DISTINCT_E_FACTORS)
    #define AXIS_CONDITION (i < E_AXIS || i == E_AXIS_N(active_extruder))
  #else
    #define AXIS_CONDITION true
  #endif
  uint32_t highest_rate = 1;
  LOOP_XYZE_N(i) {
    max_acceleration_msteps_per_s2[i] = settings.max_acceleration_mm_per_s2[i] * settings.axis_msteps_per_mm[i];
    if (AXIS_CONDITION) NOLESS(highest_rate, max_acceleration_msteps_per_s2[i]);
  }
  cutoff_long = 4294967295UL / highest_rate; // 0xFFFFFFFFUL
  #if HAS_LINEAR_E_JERK
    recalculate_max_e_jerk();
  #endif
}

// Recalculate position, mm_per_step, mm_per_half_step and mm_per_mstep if settings.axis_steps_per_mm or settings.axis_msteps_per_mm changes!
void Planner::refresh_positioning() {
  LOOP_XYZE_N(i) {
    mm_per_step[i] = 1.f / settings.axis_steps_per_mm[i];
    mm_per_half_step[i] = mm_per_step[i] / 2.f;
    mm_per_mstep[i] = 1.f / settings.axis_msteps_per_mm[i];
  }
  set_position_mm(current_position);
  refresh_acceleration_rates();
}

#if ENABLED(DISTINCT_E_FACTORS)
  void Planner::refresh_e_positioning(const uint8_t extruder) {
    mm_per_step[E_AXIS_N(extruder)] = 1.f / settings.axis_steps_per_mm[E_AXIS_N(extruder)];
    mm_per_half_step[E_AXIS_N(extruder)] = mm_per_step[E_AXIS_N(extruder)] / 2.f;
    mm_per_mstep[E_AXIS_N(extruder)] = 1.f / settings.axis_msteps_per_mm[E_AXIS_N(extruder)];
    if (extruder == active_extruder) {
      set_e_position_mm(current_position[E_AXIS]);
      refresh_acceleration_rates();
    }
  }
#endif

inline void limit_and_warn(float &val, const uint8_t axis, PGM_P const setting_name, const xyze_float_t &max_limit) {
  const uint8_t lim_axis = axis > E_AXIS ? E_AXIS : axis;
  const float before = val;
  LIMIT(val, 1, max_limit[lim_axis]);
  if (before != val) {
    SERIAL_CHAR(axis_codes[lim_axis]);
    SERIAL_ECHOPGM(" Max ");
    serialprintPGM(setting_name);
    SERIAL_ECHOLNPAIR(" limited to ", val);
  }
}

void Planner::set_max_acceleration(const uint8_t axis, float targetValue) {
  #if ENABLED(LIMITED_MAX_ACCEL_EDITING)
    #ifdef MAX_ACCEL_EDIT_VALUES
      constexpr xyze_float_t max_accel_edit = MAX_ACCEL_EDIT_VALUES;
      const xyze_float_t &max_acc_edit_scaled = max_accel_edit;
    #else
      constexpr xyze_float_t max_accel_edit = DEFAULT_MAX_ACCELERATION,
                             max_acc_edit_scaled = max_accel_edit * 2;
    #endif
    limit_and_warn(targetValue, axis, PSTR("Acceleration"), max_acc_edit_scaled);
  #endif

  auto new_settings = user_settings;
  new_settings.max_acceleration_mm_per_s2[axis] = targetValue;
  apply_settings(new_settings);
}

void Planner::set_max_feedrate(const uint8_t axis, float targetValue) {
  #if ENABLED(LIMITED_MAX_FR_EDITING)
    #ifdef MAX_FEEDRATE_EDIT_VALUES
      constexpr xyze_float_t max_fr_edit = MAX_FEEDRATE_EDIT_VALUES;
      const xyze_float_t &max_fr_edit_scaled = max_fr_edit;
    #else
      constexpr xyze_float_t max_fr_edit = DEFAULT_MAX_FEEDRATE,
                             max_fr_edit_scaled = max_fr_edit * 2;
    #endif
    limit_and_warn(targetValue, axis, PSTR("Feedrate"), max_fr_edit_scaled);
  #endif

  auto new_settings = user_settings;
  new_settings.max_feedrate_mm_s[axis] = targetValue;
  apply_settings(new_settings);
}

void Planner::set_max_jerk(const AxisEnum axis, float targetValue) {
  #if HAS_CLASSIC_JERK
    #if ENABLED(LIMITED_JERK_EDITING)
      constexpr xyze_float_t max_jerk_edit =
        #ifdef MAX_JERK_EDIT_VALUES
          MAX_JERK_EDIT_VALUES
        #else
          { (DEFAULT_XJERK) * 2, (DEFAULT_YJERK) * 2,
            (DEFAULT_ZJERK) * 2, (DEFAULT_EJERK) * 2 }
        #endif
      ;
      limit_and_warn(targetValue, axis, PSTR("Jerk"), max_jerk_edit);
    #endif
    auto s = user_settings;
    s.max_jerk[axis] = targetValue;
    apply_settings(s);
  #else
    UNUSED(axis); UNUSED(targetValue);
  #endif
}

#if ENABLED(AUTOTEMP)

  void Planner::autotemp_M104_M109() {
    if ((autotemp_enabled = parser.seen('F'))) autotemp_factor = parser.value_float();
    if (parser.seen('S')) autotemp_min = parser.value_celsius();
    if (parser.seen('B')) autotemp_max = parser.value_celsius();
  }

#endif

void Motion_Parameters::save_reset() {
  save();
  reset();
}

void Motion_Parameters::save() {
  const auto &src = planner.user_settings;

  for (int i = 0; i < XYZE_N; ++i) {
    mp.max_acceleration_mm_per_s2[i] = src.max_acceleration_mm_per_s2[i];
    mp.max_feedrate_mm_s[i] = src.max_feedrate_mm_s[i];
  }

  mp.min_segment_time_us = src.min_segment_time_us;
  mp.acceleration = src.acceleration;
  mp.retract_acceleration = src.retract_acceleration;
  mp.travel_acceleration = src.travel_acceleration;
  mp.min_feedrate_mm_s = src.min_feedrate_mm_s;
  mp.min_travel_feedrate_mm_s = src.min_travel_feedrate_mm_s;

  #if DISABLED(CLASSIC_JERK)
    mp.junction_deviation_mm = planner.junction_deviation_mm;
  #endif
  #if HAS_CLASSIC_JERK
    mp.max_jerk = src.max_jerk;
  #endif
}

void Motion_Parameters::load() const {
  auto s = planner.user_settings;

  for (int i = 0; i < XYZE_N; ++i) {
    s.max_acceleration_mm_per_s2[i] = mp.max_acceleration_mm_per_s2[i];
    s.max_feedrate_mm_s[i] = mp.max_feedrate_mm_s[i];
  }

  s.min_segment_time_us = mp.min_segment_time_us;
  s.acceleration = mp.acceleration;
  s.retract_acceleration = mp.retract_acceleration;
  s.travel_acceleration = mp.travel_acceleration;
  s.min_feedrate_mm_s = mp.min_feedrate_mm_s;
  s.min_travel_feedrate_mm_s = mp.min_travel_feedrate_mm_s;

  #if DISABLED(CLASSIC_JERK)
    planner.junction_deviation_mm = mp.junction_deviation_mm;
  #endif
  #if HAS_CLASSIC_JERK
    s.max_jerk = mp.max_jerk;
  #endif

  planner.apply_settings(s);
}

void Motion_Parameters::reset() {
  MarlinSettings::reset_motion();
  planner.refresh_acceleration_rates();
}

