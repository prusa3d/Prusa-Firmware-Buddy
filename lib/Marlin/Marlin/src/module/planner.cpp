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
#include "../feature/motordriver_util.h"

#include "../Marlin.h"

#if HAS_LEVELING
  #include "../feature/bedlevel/bedlevel.h"
#endif

#if ENABLED(AUTO_POWER_CONTROL)
  #include "../feature/power.h"
#endif

#if ENABLED(BACKLASH_COMPENSATION)
  // #error dead code found by automatic analyses (see BFW-5461)
  #include "../feature/backlash.h"
#endif

#include <option/has_cancel_object.h>
#include <option/has_crash_detection.h>
#if HAS_CANCEL_OBJECT()
  #include <feature/cancel_object/cancel_object.hpp>
#endif

#if HAS_CRASH_DETECTION()
  #include "../feature/prusa/crash_recovery.hpp"
#endif

#include <option/has_phase_stepping.h>
#if HAS_PHASE_STEPPING()
#include "../feature/phase_stepping/phase_stepping.hpp"
#endif // HAS_PHASE_STEPPING()

#include <option/has_emergency_stop.h>
#if HAS_EMERGENCY_STOP()
  #include <feature/emergency_stop/emergency_stop.hpp>
#endif

#include <option/has_ceiling_clearance.h>
#if HAS_CEILING_CLEARANCE()
  #include <feature/ceiling_clearance/ceiling_clearance.hpp>
#endif

#include <option/has_auto_retract.h>
#if HAS_AUTO_RETRACT()
  #include <feature/auto_retract/auto_retract.hpp>
#endif

#include <option/has_filament_tracker.h>
#if HAS_FILAMENT_TRACKER()
  #include <feature/filament_tracker/filament_tracker.hpp>
#endif

#include <option/has_indx.h>
#if HAS_INDX()
  #include <feature/indx_tool_lock_hack/indx_tool_lock_hack.hpp>
#endif

#include <option/has_indx.h>

#include <feature/safety_timer/safety_timer.hpp>
#include <freertos/critical_section.hpp>
#include <feature/gcode_exception/gcode_exception.hpp>

#include <marlin_vars.hpp>
#include "configuration_store.h"
#include <raii/auto_restore.hpp>
#include <utils/variant_utils.hpp>
#include <module/prusa/corexy_transform.hpp>

constexpr const int32_t MIN_MSTEPS_PER_SEGMENT = MIN_STEPS_PER_SEGMENT * PLANNER_STEPS_MULTIPLIER;

// Delay for delivery of first block to the stepper ISR to allow planner optimization.
// The delay is measured in milliseconds.
#define BLOCK_DELAY_FOR_1ST_MOVE 200

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
uint32_t Planner::delay_before_delivering;      // Initial milliseconds of delay for planner optimization
std::atomic<bool> Planner::recalculating = false;
#if ENABLED(SLOWDOWN)
std::atomic<uint32_t> Planner::slowdown_count = 0;
#endif

// A flag to indicate that that buffer is being emptied intentionally
bool Planner::emptying_buffer;

planner_settings_t Planner::working_settings_;
user_planner_settings_t Planner::user_settings_;

const planner_settings_t &Planner::settings = working_settings_;
const user_planner_settings_t &Planner::user_settings = user_settings_;

bool Planner::stealth_mode_ = false;

void Planner::apply_settings(const user_planner_settings_t &settings, const bool no_limits) {
  static constexpr planner_settings_t standard_limits = {
    .max_acceleration_mm_per_s2 = HWLIMIT_NORMAL_MAX_ACCELERATION,
    .max_feedrate_mm_s = HWLIMIT_NORMAL_MAX_FEEDRATE,
    #if HAS_CLASSIC_JERK
    .max_jerk = HWLIMIT_NORMAL_JERK,
    #endif
  };
  static constexpr planner_settings_t stealth_limits = {
    .max_acceleration_mm_per_s2 = HWLIMIT_STEALTH_MAX_ACCELERATION,
    .max_feedrate_mm_s = HWLIMIT_STEALTH_MAX_FEEDRATE,
    #if HAS_CLASSIC_JERK
    .max_jerk = HWLIMIT_STEALTH_JERK,
    #endif
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

  if (!no_limits) {
    apply_limit(&planner_settings_t::max_feedrate_mm_s);
    apply_limit(&planner_settings_t::max_acceleration_mm_per_s2);
    #if HAS_CLASSIC_JERK
    apply_limit(&planner_settings_t::max_jerk);
    #endif
  }

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
  // #error dead code found by automatic analyses (see BFW-5461)
  uint8_t Planner::last_extruder = 0;     // Respond to extruder change
#endif

StrongIndexArray<int16_t, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> Planner::flow_percentage (stdext::make_filled_array<int16_t, VirtualToolIndex::count>( 100 )); // Extrusion factor for each extruder
StrongIndexArray<float, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> Planner::e_factor (stdext::make_filled_array<float, VirtualToolIndex::count>( 1.0f )); // The flow percentage and volumetric multiplier combine to scale E movement

#if DISABLED(NO_VOLUMETRICS)
  StrongIndexArray<float, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> Planner::filament_size(stdext::make_filled_array<float, VirtualToolIndex::count>( DEFAULT_NOMINAL_FILAMENT_DIA ));
  StrongIndexArray<float, VirtualToolIndex::count, VirtualToolIndex, VirtualToolIndex::to_raw_static> Planner::volumetric_multiplier {};
#endif

#if HAS_LEVELING
  bool Planner::leveling_active = false; // Flag that auto bed leveling is enabled
  #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
    float Planner::z_fade_height,      // Initialized by settings.load()
          Planner::inverse_z_fade_height;
  #endif
#else
  // #error dead code found by automatic analyses (see BFW-5461)
  constexpr bool Planner::leveling_active;
#endif

// private:

xyze_msteps_t Planner::position{0};

uint32_t Planner::cutoff_long;

xyze_float_t Planner::previous_speed;
float Planner::previous_nominal_speed;

uint8_t Planner::g_uc_extruder_last_move[EXTRUDERS] = { 0 };

#ifdef XY_FREQUENCY_LIMIT
  // #error dead code found by automatic analyses (see BFW-5461)
  // Old direction bits. Used for speed calculations
  unsigned char Planner::old_direction_bits = 0;
  // Segment times (in µs). Used for speed calculations
  xy_ulong_t Planner::axis_segment_time_us[3] = { { MAX_FREQ_TIME_US + 1, MAX_FREQ_TIME_US + 1 } };
#endif

MachinePosXYZE Planner::position_float; // Needed for accurate maths. Steps cannot be used!

float Planner::max_printed_z = 0;

void Planner::update_max_printed_z(const MachinePosXYZE &xyze) {
  const float native_z = to_native_pos(xyze.xyz()).z;
  max_printed_z = std::max(max_printed_z, native_z);
}

struct PlannerMoveTools {
  std::optional<VirtualToolIndex> virtual_tool;
  std::optional<PhysicalToolIndex> physical_tool;

  /// Extruder index, for ye old indexed arrays that don't have the strong thing yet
  [[deprecated("Try to rather rely on virtual_tool/physical_tool")]]
  uint8_t extruder;

  explicit PlannerMoveTools(std::variant<PhysicalToolIndex, NoTool> tool) {
    physical_tool = stdext::get_optional<PhysicalToolIndex>(tool);

    if(physical_tool.has_value()) {
      virtual_tool = stdext::get_optional<VirtualToolIndex>(physical_tool->currently_selected_virtual_tool());
    } else {
      #if HOTENDS == 1
        // This should never happen on non-toolchangers
        bsod("No PhysicalTool");
      #endif
    }

    if(virtual_tool.has_value()) {
      extruder = virtual_tool->to_raw();
    } else {
      #if EXTRUDERS > 1
        static_assert(EXTRUDERS == VirtualToolIndex::count + 1);
        extruder = VirtualToolIndex::count;
      #else
        // Non-toolchanger and non-mmu targets should never get to a situation where no virtual tool is selected
        bsod("No VirtualTool");
      #endif
    }
  }
};


static float get_move_e_factor(const PlannerMoveTools &tools, const MoveHints &hints) {
  #if HAS_INDX()
    if (hints.is_service_extruder_move) {
      return EXTRUDER_SERVICE_MOVE_E_FACTOR;
    }
  #endif

  if(!hints.ignore_e_factor && tools.virtual_tool.has_value()) {
    return Planner::e_factor[*tools.virtual_tool];
  }

  return 1.0f;
}

/**
 * Class and Instance Methods
 */

void Planner::init() {
  position = {};
  position_float = {};
  previous_speed = {};
  previous_nominal_speed = 0;
  clear_block_buffer();
  delay_before_delivering = 0;
}

#if ENABLED(S_CURVE_ACCELERATION)
  // #error dead code found by automatic analyses (see BFW-5461)
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
void Planner::calculate_trapezoid_for_block(block_t * const block, const float entry_speed, const float exit_speed) {
  // Store new block parameters
  block->initial_speed = entry_speed;
  block->final_speed = exit_speed;

  #if ENABLED(S_CURVE_ACCELERATION)
    // #error dead code found by automatic analyses (see BFW-5461)
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
void Planner::reverse_pass_kernel(block_t * const previous, block_t * const current, const block_t * const next) {
  // If entry speed is already at the maximum entry speed, and there was no change of speed
  // in the next block, there is no need to recheck. Block is cruising and there is no need to
  // compute anything for this block,
  // If not, block entry speed needs to be recalculated to ensure maximum possible planned speed.
  const float max_entry_speed_sqr = current->max_entry_speed_sqr;

  // Compute maximum entry speed decelerating over the current block from its exit speed.
  // If not at the maximum entry speed, or the previous block entry speed changed
  if (!current->flag.raw_block && !previous->flag.raw_block && (current->entry_speed_sqr != max_entry_speed_sqr || (next && next->flag.recalculate))) {

    // If nominal length true, max junction speed is guaranteed to be reached.
    // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
    // the current block and next block junction speeds are guaranteed to always be at their maximum
    // junction speeds in deceleration and acceleration, respectively. This is due to how the current
    // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
    // the reverse and forward planners, the corresponding block junction speed will always be at the
    // the maximum junction speed and may always be ignored for any speed reduction checks.

    const float next_entry_speed_sqr = next ? next->entry_speed_sqr : sq(float(MINIMUM_PLANNER_SPEED)),
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
void Planner::reverse_pass() {
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
        reverse_pass_kernel(previous, current, next);
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
  if (!current->flag.raw_block && !previous->flag.raw_block && !previous->flag.nominal_length && previous->entry_speed_sqr < current->entry_speed_sqr) {

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
  if (current->entry_speed_sqr == current->max_entry_speed_sqr) {
    // The planned block might be advanced automatically as it transitions to busy state.
    // Since this can happen even without recomputation for an already-optimal plan, we
    // need to ensure the block stays non-busy as we move the pointer.
    if (previous->flag.recalculate) {
      // the block was already locked, we can safely advance the pointer
      block_buffer_planned = prev_index;
    } else {
      // ensure the ISR doesn't advance the pointer behind our's back
      MoveIsrDisabler _;
      if (!is_block_busy(previous)) {
        block_buffer_planned = prev_index;
      }
    }
  }
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
void Planner::recalculate_trapezoids() {
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
          if (block->flag.raw_block) {
              bsod("It isn't allowed to recalculate a raw block.");
          }

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

  if (block && block->flag.recalculate && block->flag.raw_block) {
    bsod("It isn't allowed to recalculate a raw block.");
  }

  // Last/newest block in buffer. Always recalculated.
  if (block && !block->flag.raw_block) {
    // Exit speed is set with MINIMUM_PLANNER_SPEED unless some code higher up knows better.
    next_entry_speed = float(MINIMUM_PLANNER_SPEED);

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

void Planner::recalculate() {
  // We need an operation that contains read for acquire to work properly
  // (and the acquire-release pair works kind of like a lock, so other operations stay within)
  recalculating.exchange(true, std::memory_order_acquire);
  // Initialize block index to the last block in the planner buffer.
  const uint8_t block_index = prev_block_index(block_buffer_head);
  // If there is just one block, no planning can be done. Avoid it!
  if (block_index != block_buffer_planned) {
    reverse_pass();
    forward_pass();
  }
  recalculate_trapezoids();
  recalculating.store(false, std::memory_order_release);

  // Inform the move ISR that there is a new block added to the queue. If it
  // wants one, now is a good time to pick it up when it's fresh instead of
  // waiting up to 1ms for it to be its turn.
  PreciseStepping::wake_up();
}

/**
 * Discard the current unprocessed block.
 * Caller must ensure that there is something to discard.
 */
void Planner::discard_current_unprocessed_block() {
  debug_assert(has_unprocessed_blocks_queued());

  block_t * block = &block_buffer[block_buffer_nonbusy];
  debug_assert(!block->busy);
  block->busy = true;

  if (block_buffer_nonbusy != block_buffer_planned)
    block_buffer_nonbusy = next_block_index(block_buffer_nonbusy);
  else {
    // push "planned" block as it became busy as well
    block_buffer_nonbusy = next_block_index(block_buffer_nonbusy);
    block_buffer_planned = block_buffer_nonbusy;
  }
}

/**
 * Maintain fans, paste extruder pressure,
 */
void Planner::check_axes_activity() {

  #if ANY(DISABLE_X, DISABLE_Y, DISABLE_Z, DISABLE_E)
    // #error dead code found by automatic analyses (see BFW-5461)
    xyze_bool_t axis_active = { false };
  #endif

  // In the current implementation of PreciseStepping, a sync position block can spend some time at the top of the block queue in contrast with the original Marlin.
  // So we have to ignore sync position blocks because they always have zero fan speeds.
  if (const block_t *block = get_first_move_block(); block != nullptr) {
      thermalManager.apply_print_fan_speed(block->print_fan_speed);

    #if ANY(DISABLE_X, DISABLE_Y, DISABLE_Z, DISABLE_E)
      // #error dead code found by automatic analyses (see BFW-5461)
      for (uint8_t b = block_buffer_tail; b != block_buffer_head; b = next_block_index(b)) {
        block_t *block = &block_buffer[b];
        LOOP_XYZE(i) if (block->msteps[i]) axis_active[i] = true;
      }
    #endif
  }
  else {
      thermalManager.apply_print_fan_speed();
  }

  //
  // Disable inactive axes
  //
  #if (ENABLED(XY_LINKED_ENABLE) && (ENABLED(DISABLE_X) || ENABLED(DISABLE_Y)))
    // #error dead code found by automatic analyses (see BFW-5461)
    if (!axis_active.x && !axis_active.y) disable_XY();
  #else
    #if ENABLED(DISABLE_X)
      // #error dead code found by automatic analyses (see BFW-5461)
      if (!axis_active.x) disable_X();
    #endif
    #if ENABLED(DISABLE_Y)
      // #error dead code found by automatic analyses (see BFW-5461)
      if (!axis_active.y) disable_Y();
    #endif
  #endif
  #if ENABLED(DISABLE_Z)
    // #error dead code found by automatic analyses (see BFW-5461)
    if (!axis_active.z) disable_Z();
  #endif
  #if ENABLED(DISABLE_E)
    // #error dead code found by automatic analyses (see BFW-5461)
    if (!axis_active.e) disable_e_steppers();
  #endif
}

#if DISABLED(NO_VOLUMETRICS)

  /**
   * Get a volumetric multiplier from a filament diameter.
   * This is the reciprocal of the circular cross-section area.
   * Return 1.0 with volumetric off or a diameter of 0.0.
   */
  inline float calculate_volumetric_multiplier(const float diameter) {
    return (parser.volumetric_enabled && diameter) ? 1.0f / CIRCLE_AREA(diameter * 0.5f) : 1;
  }

  /**
   * Convert the filament sizes into volumetric multipliers.
   * The multiplier converts a given E value into a length.
   */
  void Planner::calculate_volumetric_multipliers() {
    for (auto tool : VirtualToolIndex::all()) {
      volumetric_multiplier[tool] = calculate_volumetric_multiplier(filament_size[tool]);
      refresh_e_factor(tool);
    }
  }

#endif // !NO_VOLUMETRICS

#if HAS_LEVELING

  constexpr xy_pos_t level_fulcrum = {
    #if ENABLED(Z_SAFE_HOMING)
      Z_SAFE_HOMING_X_POINT, Z_SAFE_HOMING_Y_POINT
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      X_HOME_POS, Y_HOME_POS
    #endif
  };

  /**
   * rx, ry, rz - Cartesian positions in mm
   *              Leveled XYZ on completion
   */
  void Planner::apply_leveling(xyz_pos_t &raw) {
    // This hack works only because the NativePosTag is currently the same as MachinePosTag
    // TODO REMOVEME
    raw = to_machine_pos(raw);
  }

  void Planner::unapply_leveling(xyz_pos_t &raw) {
    // This hack works only because the NativePosTag is currently the same as MachinePosTag
    // TODO REMOVEME
    raw = to_native_pos(raw);
  }

#endif // HAS_LEVELING

bool Planner::draining() {
  return gcode_exceptions().is_unwinding();
}

void Planner::quick_stop() {
  gcode_exceptions().throw_unhandled();
}

void Planner::quick_stop_and_resume() {
  if(draining()) return;

  quick_stop();
  while (PreciseStepping::stopping()) {
    PreciseStepping::loop();
  }
  clear_block_buffer();
  resume_queuing();
}

void Planner::resume_queuing() {
  gcode_exceptions().finish_unwinding_unhandled_exception();
}

// Called from ISR
void Planner::endstop_triggered(const AxisEnum axis) {
  #if HAS_CRASH_DETECTION()
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
  if (!draining()) disable_all_steppers();
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

  for(uint8_t i = 0; i != cnt; ++i)
    pos[i] = axis_steps[i] * Planner::mm_per_step[i];

  #if IS_CORE
    #if CORE_IS_XY
      const auto xy = corexy_ab_to_xy(ab_steps_t{.a = axis_steps[0], .b = axis_steps[1]});
      pos[X_AXIS] = xy.x;
      pos[Y_AXIS] = xy.y;
    #else
      #error "unsupported core type"
    #endif
  #endif
}

void Planner::get_axis_position_mm(MachinePosXY& pos) {
  get_multi_axis_position_mm(pos.pos, 2);
}

void Planner::get_axis_position_mm(MachinePosXYZ& pos) {
  get_multi_axis_position_mm(pos.pos, NUM_AXES);
}

void Planner::get_axis_position_mm(MachinePosXYZE& pos) {
  get_multi_axis_position_mm(pos.pos, LOGICAL_AXES);
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
        MachinePosXY pos;
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
  return !draining() && processing();
}

/**
 * Block until all buffered steps are executed / cleaned
 */
void Planner::synchronize() {
  AutoRestore eb(emptying_buffer, true);
  start_moving();
  while (busy()) idle(true);
#if HAS_PHASE_STEPPING()
  phase_stepping::check_state();
#endif // HAS_PHASE_STEPPING()
}

/**
 * @brief Add a new linear movement to the planner queue (in terms of steps).
 *
 * @param target        Target position in mini-steps units
 * @param target_float  Target position in direct (mm, degrees) units.
 * @param fr_mm_s       (target) speed of the move
 * @param tools         tool indices for the move
 * @param hints         parameters to aid planner calculations
 *
 * Returns true if movement was properly queued, false otherwise
 */
bool Planner::_buffer_msteps(const xyze_msteps_t &target, const MachinePosXYZE &target_float
  , feedRate_t fr_mm_s, const PlannerMoveTools &tools, const PlannerHints &hints
) {
  debug_assert(fr_mm_s > 0);

  // Wait for the next available block
  uint8_t next_buffer_head;
  block_t * const block = get_next_free_block(next_buffer_head);
  if (!block) return false;

  // Fill the block with the specified movement
  if (!_populate_block(block, target, target_float, fr_mm_s, tools, hints)) {
    // Movement was not queued, probably because it was too short.
    // Simply accept that as movement queued and done
    return true;
  }

  if (block_buffer_head == block_buffer_tail) {
    // If it was the first queued block, set the 1st block delivery delay to
    // give the planner an opportunity to queue more movements and plan them
    delay_before_delivering = BLOCK_DELAY_FOR_1ST_MOVE;
  }

  // Move buffer head
  block_buffer_head = next_buffer_head;

  // Recalculate and optimize trapezoidal speed profiles
  recalculate();

  // Movement successfully queued!
  return true;
}

bool Planner::_buffer_msteps(const xyze_msteps_t &target, const MachinePosXYZE &target_float
  , feedRate_t fr_mm_s, std::variant<PhysicalToolIndex, NoTool> tool, const PlannerHints &hints
) {
  return _buffer_msteps(target, target_float, fr_mm_s, PlannerMoveTools(tool), hints);
}

void Planner::manage_extruders(PhysicalToolIndex physical_tool) {
#if HAS_INDX()
  enable_E(0);
#else
  const uint8_t extruder = physical_tool.to_raw();
  for (uint8_t i = 0; i < EXTRUDERS; i++) {
    if (i != extruder) {
      auto &counter = g_uc_extruder_last_move[i];
      if (counter > 0) {
        counter--;
      }
      if (counter == 0) {
        disable_E(i);
      }
    }
  }
  if (uint8_t i = extruder; i < EXTRUDERS) {
    auto &counter = g_uc_extruder_last_move[i];
    enable_E(i);
    counter = (BLOCK_BUFFER_SIZE) * 2;
  }
#endif
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
 * @param tools         tool indices for the move
 * @param hints         parameters to aid planner calculations
 *
 * @return  true if movement is acceptable, false otherwise
 */
bool Planner::_populate_block(block_t * const block,
  const xyze_msteps_t &target, const MachinePosXYZE &target_float
  , feedRate_t fr_mm_s, const PlannerMoveTools &tools, const PlannerHints &hints
) {
  debug_assert(fr_mm_s > 0);
  
  const int32_t da = target.a - position.a,
                db = target.b - position.b,
                dc = target.c - position.c;

  int32_t de = target.e - position.e;

  /* <-- add a slash to enable
    SERIAL_ECHOLNPAIR("  _populate_block FR:", fr_mm_s,
                      " A:", target.a, " (", da, " msteps)"
                      " B:", target.b, " (", db, " msteps)"
                      " C:", target.c, " (", dc, " msteps)"
                      " E:", target.e, " (", de, " msteps)"
                    );
  //*/

  // Compute direction bit-mask for this block
  uint8_t dm = 0;
  if (da < 0) SBI(dm, X_AXIS);
  if (db < 0) SBI(dm, Y_AXIS);
  if (dc < 0) SBI(dm, Z_AXIS);
  if (de < 0) SBI(dm, E_AXIS);

  const float e_fac = get_move_e_factor(tools, hints.move);
  const float e_msteps_float = de * e_fac;
  const int32_t e_msteps = static_cast<int32_t>(std::abs(e_msteps_float) + 0.5f);

  // Clear all flags, including the "busy" bit
  block->flag.clear();
  block->busy = false;

  // TODO: raw_block (no_discard) = hints.last_segment
  // Don't discard last segment even if it's shorted than MIN_MSTEPS_PER_SEGMENT

  // Set direction bits
  block->direction_bits = dm;

  // Number of mini-steps for each axis
  // default non-h-bot planning
  block->msteps = xyze_msteps_t{.a = ABS(da), .b = ABS(db), .c = ABS(dc), ._e = e_msteps};

  // Calculates the total length of the movement
  xyze_float_t delta_mm;
  delta_mm.x = da * mm_per_mstep[X_AXIS];
  delta_mm.y = db * mm_per_mstep[Y_AXIS];
  delta_mm.z = dc * mm_per_mstep[Z_AXIS];
  delta_mm.e = e_msteps_float * mm_per_mstep[E_AXIS_N(tools.extruder)];
  block->mstep_event_count = _MAX(block->msteps.a, block->msteps.b, block->msteps.c, e_msteps);

  // Always calculate the block length if we are going to keep it
  if (block->mstep_event_count >= MIN_MSTEPS_PER_SEGMENT || hints.raw_block) {
    if (block->msteps.a || block->msteps.b || block->msteps.c) {
      block->millimeters = SQRT(sq(delta_mm.x) + sq(delta_mm.y) + sq(delta_mm.z));
    } else {
      block->millimeters = abs(delta_mm.e);
    }
  }

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
    // #error dead code found by automatic analyses (see BFW-5461)
    if (!hints.no_discard) {
      backlash.add_correction_msteps(da, db, dc, dm, block);
    }
  #endif

  if (!hints.raw_block) {
    // Bail if this is a regular short block
    if (block->mstep_event_count < MIN_MSTEPS_PER_SEGMENT)
      return false;
  } else if (!block->mstep_event_count) {
    // Bail if this is a zero-length block
    return false;
  }

  block->print_fan_speed = thermalManager.print_fan_speed;
  
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
    // #error dead code found by automatic analyses (see BFW-5461)
    if (block->msteps.a || block->msteps.c) {
      enable_X();
      enable_Z();
    }
    if (block->msteps.y) enable_Y();
  #elif CORE_IS_YZ
    // #error dead code found by automatic analyses (see BFW-5461)
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

  // Perform XYZ pre-move hooks
  if (block->msteps.x || block->msteps.y) {
    motor_prepare_move_xy();
  }
  if (block->msteps.z) {
    motor_prepare_move_z();
  }

  // Enable extruder(s)
  if (e_msteps) {
    #if ENABLED(AUTO_POWER_CONTROL)
      powerManager.power_on();
    #endif

    if (tools.physical_tool.has_value()) {
      Planner::manage_extruders(*tools.physical_tool);
    }
    #if HAS_INDX()
    else if (hints.move.is_service_extruder_move) {
      // INDX service moves (lock/unlock mechanism) run without a picked tool.
      // Enable the single physical E stepper directly.
      enable_E(0);
    }
    #endif

    // Perform E pre-move hooks
    motor_prepare_move_e();
  }



  if (e_msteps)
    NOLESS(fr_mm_s, settings.min_feedrate_mm_s);
  else
    NOLESS(fr_mm_s, settings.min_travel_feedrate_mm_s);

  if (!block->mstep_event_count) {
    // reset block velocities to invalid values to avoid use
    block->nominal_speed = 0;
    block->entry_speed_sqr = 0;
    block->max_entry_speed_sqr = 0;
    block->millimeters = 0;
    block->acceleration = 0;
    block->initial_speed = 0;
    block->final_speed = 0;
  } else {
    const float inverse_millimeters = 1.0f / block->millimeters;  // Inverse millimeters to remove multiple divides

    // Calculate inverse time for this move. No divide by zero due to previous checks.
    // Example: At 120mm/s a 60mm move takes 0.5s. So this will give 2.0.
    float inverse_secs = fr_mm_s * inverse_millimeters;

    // Get the number of non busy movements in queue (non busy means that they can be altered)
    const uint8_t moves_queued = nonbusy_movesplanned();

    // Slow down when the buffer starts to empty, rather than wait at the corner for a buffer refill
    #if ENABLED(SLOWDOWN) || defined(XY_FREQUENCY_LIMIT)
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
      if (!draining() && !emptying_buffer && queue.length <= 3 && WITHIN(total_blocks_queued, 2, (BLOCK_BUFFER_SIZE) / (SLOWDOWN_DIVISOR) - 1)) {
        const int32_t time_diff = static_cast<int32_t>(settings.min_segment_time_us) - segment_time_us;
        if (time_diff > 0) {
          // Count actual feedrate reductions (not just the buffer-depth gate above).
          slowdown_count.fetch_add(1, std::memory_order_relaxed);
          // Buffer is draining so add extra time. The amount of time added increases if the buffer is still emptied more.
          const uint32_t nst = segment_time_us + LROUND(2 * time_diff / total_blocks_queued);
          inverse_secs = 1000000.0f / nst;
          #if defined(XY_FREQUENCY_LIMIT)
            // #error dead code found by automatic analyses (see BFW-5461)
            segment_time_us = nst;
          #endif
        }
      }
    #endif

    block->nominal_speed = block->millimeters * inverse_secs;           // (mm/sec) Always > 0
    #if ENABLED(S_CURVE_ACCELERATION)
      // #error dead code found by automatic analyses (see BFW-5461)
      block->nominal_rate = CEIL(block->mstep_event_count * inverse_secs); // (mini-step/sec) Always > 0
    #endif
    debug_assert(block->nominal_speed > 0); // This assert just saved you 4 hours of digging through input shaper internals. You're welcome.

    // Calculate and limit speed in mm/sec for each axis
    xyze_float_t current_speed;
    float speed_factor = 1.0f; // factor <1 decreases speed

    #ifdef COREXY
      const float speed_mm_x = std::abs(current_speed[X_AXIS] = delta_mm[X_AXIS] * inverse_secs);
      const float speed_mm_y = std::abs(current_speed[Y_AXIS] = delta_mm[Y_AXIS] * inverse_secs);
      const feedRate_t highest_strain = speed_mm_x + speed_mm_y;

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
        // #error dead code found by automatic analyses (see BFW-5461)
        if (i == E_AXIS) i += extruder;
      #endif
    }

    // Max segment time in µs.
    #ifdef XY_FREQUENCY_LIMIT
      // #error dead code found by automatic analyses (see BFW-5461)
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
      // #error dead code found by automatic analyses (see BFW-5461)
      block->nominal_rate *= speed_factor;
    #endif
      block->nominal_speed *= speed_factor;
    }

    // Compute and limit the acceleration rate for the trapezoid generator.
    const float msteps_per_mm = block->mstep_event_count * inverse_millimeters;
    float accel;
    if (!block->msteps.a && !block->msteps.b && !block->msteps.c) {
      // convert to: acceleration steps/sec^2
      accel = CEIL(settings.retract_acceleration * msteps_per_mm);
    }
    else {
      #define LIMIT_ACCEL_LONG(AXIS,INDX) do{ \
        if (block->msteps[AXIS] && max_acceleration_msteps_per_s2[AXIS+INDX] < accel) { \
          const uint32_t comp = static_cast<uint32_t>(max_acceleration_msteps_per_s2[AXIS+INDX] * block->mstep_event_count); \
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
        // #error dead code found by automatic analyses (see BFW-5461)
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
      // #error dead code found by automatic analyses (see BFW-5461)
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
      static MachinePosXYZE prev_unit_vec;
      MachinePosXYZE unit_vec = target_float - position_float;

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
          // #error dead code found by automatic analyses (see BFW-5461)
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
          MachinePosXYZE junction_unit_vec = unit_vec - prev_unit_vec;
          normalize_junction_vector(junction_unit_vec);

          const float junction_acceleration = limit_value_by_axis_maximum(block->acceleration, junction_unit_vec),
                      sin_theta_d2 = SQRT(0.5f * (1.0f - junction_cos_theta)); // Trig half angle identity. Always positive.

          vmax_junction_sqr = (junction_acceleration * junction_deviation_mm * sin_theta_d2) / (1.0f - sin_theta_d2);
          #if ENABLED(JD_SMALL_SEGMENT_HANDLING)
            // For small moves with >135° junction (octagon) find speed for approximate arc
            if (block->millimeters < 1 && junction_cos_theta < -0.7071067812f) {
              // Fast acos(-t) approximation (max. error +-0.033rad = 1.89°)
              // Based on MinMax polynomial published by W. Randolph Franklin, see
              // https://wrf.ecse.rpi.edu/Research/Short_Notes/arcsin/onlyelem.html
              //  acos( t) = pi / 2 - asin(x)
              //  acos(-t) = pi - acos(t) ... pi / 2 + asin(x)

              const float neg = junction_cos_theta < 0 ? -1 : 1,
                          t = neg * junction_cos_theta,
                          asinx =       0.032843707f
                                + t * (-1.451838349f
                                + t * ( 29.66153956f
                                + t * (-131.1123477f
                                + t * ( 262.8130562f
                                + t * (-242.7199627f
                                + t * ( 84.31466202f ) ))))),
                          junction_theta = RADIANS(90) + neg * asinx; // acos(-t)

              // NOTE: junction_theta bottoms out at 0.033 which avoids divide by 0.

              const float limit_sqr = (block->millimeters * junction_acceleration) / junction_theta;
              NOMORE(vmax_junction_sqr, limit_sqr);
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
        // #error dead code found by automatic analyses (see BFW-5461)
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
          // #error dead code found by automatic analyses (see BFW-5461)
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
        // #error dead code found by automatic analyses (see BFW-5461)
        vmax_junction_sqr = _MIN(vmax_junction_sqr, sq(vmax_junction));
      #else
        vmax_junction_sqr = sq(vmax_junction);
      #endif

    #endif // Classic Jerk Limiting

    // Max entry speed of this block equals the max exit speed of the previous block.
    #if ENABLED(JD_DEBUG_OUTPUT)
      // #error dead code found by automatic analyses (see BFW-5461)
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
  }

  #if HAS_CRASH_DETECTION()
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
    crash_block.recover_flags = crash_s.gcode_state.recover_flags;
    crash_block.fr_mm_s = fr_mm_s;
  }
  #endif

  // Update the position
  position = target;
  position_float = target_float;

  // Movement was accepted
  return true;
} // _populate_block()

bool Planner::populate_raw_block(block_t *const block, const xyze_msteps_t &target, const MachinePosXYZE &target_float, const float acceleration, const float nominal_speed, const float entry_speed, const float exit_speed, const PlannerMoveTools &tools) {
    const int32_t da = target.a - position.a,
                  db = target.b - position.b,
                  dc = target.c - position.c;

    int32_t de = target.e - position.e;

    // Clear all flags, including the "busy" bit
    block->flag.clear();
    block->busy = false;

    // Compute direction bit-mask for this block
    uint8_t dm = 0;
    if (da < 0) {
        SBI(dm, X_AXIS);
    }

    if (db < 0) {
        SBI(dm, Y_AXIS);
    }

    if (dc < 0) {
        SBI(dm, Z_AXIS);
    }

    if (de < 0) {
        SBI(dm, E_AXIS);
    }

    // Set direction bits
    block->direction_bits = dm;

    // Number of mini-steps for each axis
    // default non-h-bot planning
    block->msteps.set(ABS(da), ABS(db), ABS(dc), ABS(de));
    block->mstep_event_count = _MAX(block->msteps.a, block->msteps.b, block->msteps.c, block->msteps.e);

    if (!block->mstep_event_count) {
        // Bail if this is a zero-length block
        return false;
    }

    // Calculates the total length of the movement
    xyze_float_t delta_mm;
    delta_mm.x = float(da) * mm_per_mstep[X_AXIS];
    delta_mm.y = float(db) * mm_per_mstep[Y_AXIS];
    delta_mm.z = float(dc) * mm_per_mstep[Z_AXIS];
    delta_mm.e = float(de) * mm_per_mstep[E_AXIS_N(tools.extruder)];

    if (da == 0 && db == 0 && dc == 0) {
        block->millimeters = TERN0(HAS_EXTRUDERS, ABS(delta_mm.e));
    } else {
        block->millimeters = SQRT(sq(delta_mm.x) + sq(delta_mm.y) + sq(delta_mm.z));
    }

    block->print_fan_speed = Temperature::print_fan_speed;

    #if ENABLED(AUTO_POWER_CONTROL)
        if (block->msteps.x || block->msteps.y || block->msteps.z) {
            Power::power_on();
        }
    #endif

    // Enable active axes
    #if CORE_IS_XY
            if (block->msteps.a || block->msteps.b) {
                enable_XY();
            }
        #if DISABLED(Z_LATE_ENABLE)
            if (block->msteps.z) {
                enable_Z();
            }
        #endif
    #elif CORE_IS_XZ
      // #error dead code found by automatic analyses (see BFW-5461)
        if (block->msteps.a || block->msteps.c) {
            enable_X();
            enable_Z();
        }
        if (block->msteps.y) {
            enable_Y();
        }
    #elif CORE_IS_YZ
      // #error dead code found by automatic analyses (see BFW-5461)
        if (block->msteps.b || block->msteps.c) {
            enable_Y();
            enable_Z();
        }
        if (block->msteps.x) {
            enable_X();
        }
    #else
        #if ENABLED(XY_LINKED_ENABLE)
        if (block->msteps.x || block->msteps.y) {
            enable_XY();
        }
        #else
        if (block->msteps.x) {
            enable_X();
        }
        if (block->msteps.y) {
            enable_Y();
        }
        #endif
        #if DISABLED(Z_LATE_ENABLE)
        if (block->msteps.z) {
            enable_Z();
        }
        #endif
    #endif

    // Enable extruder(s)
    if (block->msteps.e) {
    #if ENABLED(AUTO_POWER_CONTROL)
        Power::power_on();
    #endif

        if (tools.physical_tool.has_value()) {
            Planner::manage_extruders(*tools.physical_tool);
        }
    }

    block->acceleration = acceleration;
    block->nominal_speed = nominal_speed;
    block->initial_speed = entry_speed;
    block->final_speed = exit_speed;

    block->entry_speed_sqr = sq(entry_speed);
    block->max_entry_speed_sqr = block->entry_speed_sqr;

    // Initialize block entry speed. Compute based on deceleration to user-defined MINIMUM_PLANNER_SPEED.
    const float v_allowable_sqr = max_allowable_speed_sqr(-block->acceleration, sq(float(MINIMUM_PLANNER_SPEED)), block->millimeters);

    // Initialize planner efficiency flags
    // Set flag if block will always reach maximum junction speed regardless of entry/exit speeds.
    // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
    // the current block and next block junction speeds are guaranteed to always be at their maximum
    // junction speeds in deceleration and acceleration, respectively. This is due to how the current
    // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
    // the reverse and forward planners, the corresponding block junction speed will always be at the
    // the maximum junction speed and may always be ignored for any speed reduction checks.
    block->flag.nominal_length = sq(block->nominal_speed) <= v_allowable_sqr;

    // Calculate speed in mm/sec for each axis.
    xyze_float_t current_speed;
    LOOP_XYZE(i) {
        current_speed[i] = (block->nominal_speed * delta_mm[i]) / block->millimeters;
    }

    // Update previous path unit_vector and nominal speed
    previous_speed = current_speed;
    previous_nominal_speed = block->nominal_speed;

    #if HAS_CRASH_DETECTION()
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
        crash_block.recover_flags = crash_s.gcode_state.recover_flags;
        crash_block.fr_mm_s = nominal_speed;
    }
    #endif

    // Update the position
    position = target;
    position_float = target_float;

    block->flag.raw_block = true;

    // Movement was accepted
    return true;
} // populate_raw_block()

bool Planner::buffer_raw_block(const xyze_msteps_t &target, const MachinePosXYZE &target_float, const float acceleration, const float nominal_speed, const float entry_speed, const float exit_speed, const PlannerMoveTools &tools) {
    // Wait for the next available block
    uint8_t next_buffer_head;
    block_t *const block = get_next_free_block(next_buffer_head);
    if (!block) {
        return false;
    }

    // Fill the block with the specified movement.
    if (!Planner::populate_raw_block(block, target, target_float, acceleration, nominal_speed, entry_speed, exit_speed, tools)) {
        // Movement was not queued, probably because it was too short.
        // Simply accept that as movement queued and done
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
    recalculate();

    // Movement successfully queued!
    return true;
}

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
  block->sync_step_position = {
    position[X_AXIS] / PLANNER_STEPS_MULTIPLIER,
    position[Y_AXIS] / PLANNER_STEPS_MULTIPLIER,
    position[Z_AXIS] / PLANNER_STEPS_MULTIPLIER,
    position[E_AXIS] / PLANNER_STEPS_MULTIPLIER
  };

  block_buffer_head = next_buffer_head;
} // buffer_sync_block()

/**
 * @brief Add a single linear movement
 *
 * @description Add a new linear movement to the buffer in axis units.
 *              Leveling and kinematics should be applied before calling this.
 *
 * @param xyze          Target positions in mm and/or degrees
 * @param fr_mm_s       (target) speed of the move
 * @param tool          physical tool for the move
 * @param hints         optional parameters to aid planner calculations
 */
bool Planner::buffer_segment(const MachinePosXYZE &xyze, const feedRate_t fr_mm_s, std::variant<PhysicalToolIndex, NoTool> tool, const PlannerHints &hints/*=PlannerHints()*/) {
#if defined(Z_CEILING_CLEARANCE) != HAS_CEILING_CLEARANCE()
  #error Z_CEILING_CLEARANCE must be defined only if HAS_CEILING_CLEARANCE()
#endif

  debug_assert(fr_mm_s > 0);

  PlannerMoveTools tools(tool);

  // The target position of the tool in absolute mini-steps
  // Calculate target position in absolute mini-steps
  const xyze_msteps_t target = {
    int32_t(LROUND(xyze.x * settings.axis_msteps_per_mm[X_AXIS])),
    int32_t(LROUND(xyze.y * settings.axis_msteps_per_mm[Y_AXIS])),
    int32_t(LROUND(xyze.z * settings.axis_msteps_per_mm[Z_AXIS])),
    int32_t(LROUND(xyze.e * settings.axis_msteps_per_mm[E_AXIS_N(tools.extruder)]))
  };

#if HAS_CEILING_CLEARANCE()
  // BFW-7734 only check when a negative Z-move is planned - it's a workaround for ceiling check sometimes reporting false positives yet for unknown reasons
  if(target.z < position.z){
    // ! Important: call before checking for draining()
    // Note: I don't remember why I thought it was important and now I think it should probably be under the check
    buddy::check_ceiling_clearance(xyze);
  }
#endif

  // If we are aborting, do not accept queuing of movements
  if (draining() || PreciseStepping::stopping()) return false;

  // Make sure we are at the correct temperatures before doing any move whatsoever
  // Also doing any printer movement resets the timer
  buddy::safety_timer().reset_restore_blocking();


  if (target.x != position.x || target.y != position.y || target.z != position.z) {
    #if HAS_EMERGENCY_STOP()
      // E-moves alone are always allowed
      buddy::emergency_stop().maybe_block();
      buddy::emergency_stop().assert_can_plan_movement();

      // Check once more (this could have changed during the maybe_block)
      if (draining() || PreciseStepping::stopping()) return false;
    #endif

    gcode_exceptions().report_xyz_move();
  }

  if (!hints.move.is_service_extruder_move && target.e != position.e) {
    /// Extruder movement without a tool picked doesn't make any sense
    if(!tools.physical_tool.has_value()) {
      bsod("E move without tool");
    }

    [[maybe_unused]] const float delta_e_mm = (xyze.e - position_float.e) * get_move_e_factor(tools, hints.move);

#if HAS_INDX()
    buddy::indx_tool_lock_hack().track_extruder_move(delta_e_mm, Badge<Planner>{});
#endif

#if HAS_FILAMENT_TRACKER()
    if(tools.virtual_tool.has_value()) {
      // Note: This is not >>ideal<<, because although the moves get planned, they might get discarded through (gcode_exceptions/quick_stop)
      // Most notably, this will track some extra filament usage if user intterupts purging
      // Tying this directly to the immediate motor positions might be better, but one would also need to also handle the origin resets
      buddy::filament_tracker().track_extruder_move(*tools.virtual_tool, delta_e_mm);
    }
#endif
  }

#if HAS_AUTO_RETRACT()
  if (hints.move.is_service_extruder_move || !tools.physical_tool.has_value()) {
    // Extruder switch move is out of filament gears
    // Or tool is not in the extruder
    
  } else if (target.e > position.e) {
    buddy::auto_retract().maybe_deretract_to_nozzle();
    
  } else if (hints.move.is_printing_move && buddy::auto_retract().will_deretract()) {
    // Ignore retraction commands if we're retracted to prevent the filament getting out of the extruder
    // Only limit to printing moves - otherwise we screw up things like unload
    position.e = target.e;
    position_float.e = xyze.e;
    
  } else if (target.e < position.e) {
    // If we've managed to retract while auto_retracted, the auto_retract data is no longer valid and would only cause a mess
    buddy::auto_retract().set_retracted_distance(*tools.physical_tool, std::nullopt);
  }
#endif

  #if HAS_CRASH_DETECTION()
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
      set_machine_position_mm(crash_s.crash_machine_position);

      // continue normally
      crash_s.set_state(Crash_s::PRINTING);
    }
  }
  #endif

  if (hints.move.is_printing_move
      && xyze.e > position_float.e
      && is_xy_in_print_region(xyze.xy())) {
    update_max_printed_z(xyze);
  }

  // When changing extruders recalculate mini-steps corresponding to the E position
  #if ENABLED(DISTINCT_E_FACTORS)
    // #error dead code found by automatic analyses (see BFW-5461)
    if (last_extruder != tools.extruder && settings.axis_msteps_per_mm[E_AXIS_N(tools.extruder)] != settings.axis_msteps_per_mm[E_AXIS_N(last_extruder)]) {
      position.e = LROUND(position.e * settings.axis_msteps_per_mm[E_AXIS_N(tools.extruder)] * mm_per_mstep[E_AXIS_N(last_extruder)]);
      last_extruder = tools.extruder;
    }
  #endif

  // DRYRUN prevents E moves from taking place
  if (DEBUGGING(DRYRUN)) {
    position.e = target.e;
    position_float.e = xyze.e;
  }

  #if EITHER(PREVENT_COLD_EXTRUSION, PREVENT_LENGTHY_EXTRUDE)
    if (const float de = target.e - position.e) {
      #if ENABLED(PREVENT_COLD_EXTRUSION)
        if (hints.move.extrusion_safety_checks && thermalManager.tooColdToExtrude(*tools.physical_tool)) {
          position.e = target.e; // Behave as if the move really took place, but ignore E part
          position_float.e = xyze.e;
          SERIAL_ECHO_MSG(MSG_ERR_COLD_EXTRUDE_STOP);
        }
      #endif // PREVENT_COLD_EXTRUSION
      #if ENABLED(PREVENT_LENGTHY_EXTRUDE)
        const float e_fac = get_move_e_factor(tools, hints.move);
        const float e_msteps = ABS(de * e_fac);
        const float max_e_msteps = settings.axis_msteps_per_mm[E_AXIS_N(tools.extruder)] * (EXTRUDE_MAXLENGTH);
        if (e_msteps > max_e_msteps) {
          constexpr bool ignore_e = true;
          if (ignore_e) {
            position.e = target.e; // Behave as if the move really took place, but ignore E part
            position_float.e = xyze.e;
            SERIAL_ECHO_MSG(MSG_ERR_LONG_EXTRUDE_STOP);
          }
        }
      #endif // PREVENT_LENGTHY_EXTRUDE
    }
  #endif // PREVENT_COLD_EXTRUSION || PREVENT_LENGTHY_EXTRUDE

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
  if (!_buffer_msteps(target, xyze
      , fr_mm_s, tools
      , hints
  )) return false;

  return true;
} // buffer_segment()

bool Planner::buffer_raw_segment(const MachinePosXYZE &xyze, const float acceleration, const float nominal_speed, const float entry_speed, const float exit_speed, std::variant<PhysicalToolIndex, NoTool> tool) {
    PlannerMoveTools tools(tool);

    // If we are aborting, do not accept queuing of movements
    if (draining() || PreciseStepping::stopping()) {
        return false;
    }

    #if HAS_CRASH_DETECTION()
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
            set_machine_position_mm(crash_s.crash_machine_position);

            // continue normally
            crash_s.set_state(Crash_s::PRINTING);
        }
    }
    #endif

    // The target position of the tool in absolute mini-steps
    // Calculate target position in absolute mini-steps
    const xyze_msteps_t target = {
      int32_t(LROUND(xyze.x * settings.axis_msteps_per_mm[X_AXIS])),
      int32_t(LROUND(xyze.y * settings.axis_msteps_per_mm[Y_AXIS])),
      int32_t(LROUND(xyze.z * settings.axis_msteps_per_mm[Z_AXIS])),
      int32_t(LROUND(xyze.e * settings.axis_msteps_per_mm[E_AXIS_N(tools.extruder)]))
    };

    // DRYRUN prevents E moves from taking place
    if (DEBUGGING(DRYRUN)) {
        position.e = target.e;
        position_float.e = xyze.e;
    }

    // Queue the movement. Return 'false' if the move was not queued.
    if (!buffer_raw_block(target, xyze, acceleration, nominal_speed, entry_speed, exit_speed, tools)) {
        return false;
    }
    return true;
} // buffer_raw_segment()

/**
 * Add a new linear movement to the buffer.
 * The target is cartesian.
 *
 *  cart            - target position in mm or degrees
 *  fr_mm_s         - (target) speed of the move (mm/s)
 *  tool            - physical tool for the move
 *  hints           - optional parameters to aid planner calculations
 */
bool Planner::buffer_line(const MachinePosXYZE &cart, const feedRate_t fr_mm_s, std::variant<PhysicalToolIndex, NoTool> tool, const PlannerHints &hints/*=PlannerHints()*/) {
  return buffer_segment(cart, fr_mm_s, tool, hints);
} // buffer_line()

bool Planner::buffer_raw_line(const MachinePosXYZE &cart, const float acceleration, const float nominal_speed, const float entry_speed, const float exit_speed, std::variant<PhysicalToolIndex, NoTool> tool) {
    return buffer_raw_segment(cart, acceleration, nominal_speed, entry_speed, exit_speed, tool);
} // buffer_raw_line()

/**
 * Directly set the planner XYZE position (and underlying stepper positions)
 * by converting mm into mini-steps.
 */

void Planner::set_machine_position_mm_planner_only(const MachinePosXYZE &xyze) {
  #if ENABLED(DISTINCT_E_FACTORS)
    // #error dead code found by automatic analyses (see BFW-5461)
    last_extruder = active_extruder;
  #endif
  position_float = xyze;
  position = {
    LROUND(xyze.x * settings.axis_msteps_per_mm[X_AXIS]),
    LROUND(xyze.y * settings.axis_msteps_per_mm[Y_AXIS]),
    LROUND(xyze.z * settings.axis_msteps_per_mm[Z_AXIS]),
    LROUND(xyze.e * settings.axis_msteps_per_mm[E_AXIS_N(active_extruder)]),
  };
}

void Planner::set_machine_position_mm(const MachinePosXYZE &xyze) {
  set_machine_position_mm_planner_only(xyze);

  if (processing()) {
    //previous_nominal_speed = 0.0f; // Reset planner junction speeds. Assume start from rest.
    //previous_speed.reset();
    buffer_sync_block();
  } else {
    const xyze_steps_t stepper_position = {
      LROUND(xyze.x * settings.axis_steps_per_mm[X_AXIS]),
      LROUND(xyze.y * settings.axis_steps_per_mm[Y_AXIS]),
      LROUND(xyze.z * settings.axis_steps_per_mm[Z_AXIS]),
      LROUND(xyze.e * settings.axis_steps_per_mm[E_AXIS_N(active_extruder)])
    };
    stepper.set_position(stepper_position);
  }
}

void Planner::set_position_mm(const xyze_pos_t &xyze) {
  set_machine_position_mm(to_machine_pos(xyze));
}

/**
 * Setters for planner position (also setting stepper position).
 * @param e_axis_index  When provided, skips tool lookup and uses this axis index directly.
 *                      Use for service moves (e.g. INDX lever) that operate without an active tool.
 */
void Planner::set_e_position_mm(const float e, std::optional<uint8_t> e_axis_index) {
  if(!e_axis_index.has_value()) {
      const auto current_tool = PhysicalToolIndex::currently_selected_opt();
      if (!current_tool.has_value()) {
        // You should not be trying to set e_position without an active tool
        debug_assert(false);
        return;
      }
      e_axis_index = E_AXIS_N(*current_tool);
  }

  #if ENABLED(DISTINCT_E_FACTORS)
    // #error dead code found by automatic analyses (see BFW-5461)
    last_extruder = active_extruder;
  #endif
  position.e = LROUND(settings.axis_msteps_per_mm[*e_axis_index] * e);
  position_float.e = e;

  if (processing())
    buffer_sync_block();
  else
    stepper.set_axis_position(E_AXIS, LROUND(settings.axis_steps_per_mm[*e_axis_index] * e));
}

void Planner::reset_position() {
  MachinePosXYZE pos;
  
  // Sample position from steppers
  get_axis_position_mm(pos);

  // Shove the sampled position to the planner
  set_machine_position_mm_planner_only(pos);
}

// Recalculate the mini-steps/s^2 acceleration rates, based on the mm/s^2
void Planner::refresh_acceleration_rates() {
  #if ENABLED(DISTINCT_E_FACTORS)
    // #error dead code found by automatic analyses (see BFW-5461)
    #define AXIS_CONDITION (i < E_AXIS || i == E_AXIS_N(active_extruder))
  #else
    #define AXIS_CONDITION true
  #endif
  uint32_t highest_rate = 1;
  LOOP_XYZE_N(i) {
    max_acceleration_msteps_per_s2[i] = static_cast<uint32_t>(settings.max_acceleration_mm_per_s2[i] * settings.axis_msteps_per_mm[i]);
    if (AXIS_CONDITION) NOLESS(highest_rate, static_cast<uint32_t>(max_acceleration_msteps_per_s2[i]));
  }
  cutoff_long = std::numeric_limits<uint32_t>::max() / highest_rate;
  #if HAS_LINEAR_E_JERK
    // #error dead code found by automatic analyses (see BFW-5461)
    recalculate_max_e_jerk();
  #endif
}

// Recalculate position, mm_per_step, mm_per_half_step and mm_per_mstep if settings.axis_steps_per_mm or settings.axis_msteps_per_mm changes!
void Planner::refresh_positioning() {
  debug_assert(!planner.processing());
  LOOP_XYZE_N(i) {
    mm_per_step[i] = 1.f / settings.axis_steps_per_mm[i];
    mm_per_half_step[i] = mm_per_step[i] / 2.f;
    mm_per_mstep[i] = 1.f / settings.axis_msteps_per_mm[i];
  }
  set_position_mm(current_position);
  refresh_acceleration_rates();
}

#if ENABLED(DISTINCT_E_FACTORS)
  // #error dead code found by automatic analyses (see BFW-5461)
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
    // #error dead code found by automatic analyses (see BFW-5461)
    #ifdef MAX_ACCEL_EDIT_VALUES
      // #error dead code found by automatic analyses (see BFW-5461)
      constexpr xyze_float_t max_accel_edit = MAX_ACCEL_EDIT_VALUES;
      const xyze_float_t &max_acc_edit_scaled = max_accel_edit;
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
      constexpr xyze_float_t max_accel_edit = DEFAULT_MAX_ACCELERATION,
                             max_acc_edit_scaled = max_accel_edit * 2;
    #endif
    limit_and_warn(targetValue, axis, PSTR("Acceleration"), max_acc_edit_scaled);
  #endif

  auto new_settings = user_settings;
  new_settings.max_acceleration_mm_per_s2[axis] = static_cast<uint32_t>(targetValue);
  apply_settings(new_settings);
}

void Planner::set_max_feedrate(const uint8_t axis, float targetValue) {
  #if ENABLED(LIMITED_MAX_FR_EDITING)
    // #error dead code found by automatic analyses (see BFW-5461)
    #ifdef MAX_FEEDRATE_EDIT_VALUES
      // #error dead code found by automatic analyses (see BFW-5461)
      constexpr xyze_float_t max_fr_edit = MAX_FEEDRATE_EDIT_VALUES;
      const xyze_float_t &max_fr_edit_scaled = max_fr_edit;
    #else
      // #error dead code found by automatic analyses (see BFW-5461)
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
      // #error dead code found by automatic analyses (see BFW-5461)
      constexpr xyze_float_t max_jerk_edit =
        #ifdef MAX_JERK_EDIT_VALUES
          // #error dead code found by automatic analyses (see BFW-5461)
          MAX_JERK_EDIT_VALUES
        #else
          // #error dead code found by automatic analyses (see BFW-5461)
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

void Motion_Parameters::save_reset(const bool no_limits) {
  save();
  reset(no_limits);
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

void Motion_Parameters::reset(const bool no_limits) {
  MarlinSettings::reset_motion(no_limits);
}
