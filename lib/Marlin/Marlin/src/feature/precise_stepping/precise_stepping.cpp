/**
 * Based on the implementation in Klipper [https://github.com/Klipper3d/klipper].
 * Copyright (C) Kevin O'Connor <kevin@koconnor.net>
 *
 * Our implementation takes inspiration from the work of Kevin O'Connor <kevin@koconnor.net> for Klipper
 * in used data structures, and some computations.
 */
#include "precise_stepping.hpp"
#include "../input_shaper/input_shaper.hpp"
#include "../pressure_advance/pressure_advance.hpp"
#include "../phase_stepping/phase_stepping.hpp"
#include "internal.hpp"

#include "../../module/planner.h"
#include "../../module/stepper.h"
#include "../../module/endstops.h"

#include "bsod.h"

#include <timing_precise.hpp>
#include <timing.h>

#include <logging/log.hpp>
LOG_COMPONENT_DEF(PreciseStepping, logging::Severity::debug);

#if defined(ISR_DEADLINE_DEBUGGING) || defined(ISR_EVENT_DEBUGGING)
    #include <sound.hpp>
#endif

#if BOARD_IS_DWARF()
    #define X_APPLY_DIR(v) X_DIR_WRITE(v)
    #define Y_APPLY_DIR(v) Y_DIR_WRITE(v)
    #define Z_APPLY_DIR(v) Z_DIR_WRITE(v)
    #define E_APPLY_DIR(v) E0_DIR_WRITE(v)
#else
    #define X_APPLY_DIR(v) buddy::hw::xDir.write((buddy::hw::Pin::State)(v))
    #define Y_APPLY_DIR(v) buddy::hw::yDir.write((buddy::hw::Pin::State)(v))
    #define Z_APPLY_DIR(v) buddy::hw::zDir.write((buddy::hw::Pin::State)(v))
    #define E_APPLY_DIR(v) buddy::hw::e0Dir.write((buddy::hw::Pin::State)(v))
#endif

#ifdef SQUARE_WAVE_STEPPING
    #if PRINTER_IS_PRUSA_XL() && !BOARD_IS_DWARF()
        // on XLBuddy the XY pin assignment is dynamic depending on board revision
        #define X_STEP_SET() buddy::hw::XStep->toggle();
        #define Y_STEP_SET() buddy::hw::YStep->toggle();
    #else
        #define X_STEP_SET() buddy::hw::xStep.toggle();
        #define Y_STEP_SET() buddy::hw::yStep.toggle();
    #endif
    #define Z_STEP_SET() buddy::hw::zStep.toggle();
    #define E_STEP_SET() buddy::hw::e0Step.toggle();

    #define X_STEP_RESET()
    #define Y_STEP_RESET()
    #define Z_STEP_RESET()
    #define E_STEP_RESET()
#else
    #define X_STEP_SET() X_STEP_WRITE(1)
    #define Y_STEP_SET() Y_STEP_WRITE(1)
    #define Z_STEP_SET() Z_STEP_WRITE(1)
    #define E_STEP_SET() E0_STEP_WRITE(1)

    #define X_STEP_RESET() X_STEP_WRITE(0)
    #define Y_STEP_RESET() Y_STEP_WRITE(0)
    #define Z_STEP_RESET() Z_STEP_WRITE(0)
    #define E_STEP_RESET() E0_STEP_WRITE(0)
#endif

move_segment_queue_t PreciseStepping::move_segment_queue;
step_event_queue_t PreciseStepping::step_event_queue;

uint16_t PreciseStepping::left_ticks_to_next_step_event = 0;

step_generator_state_t PreciseStepping::step_generator_state;
step_generators_pool_t PreciseStepping::step_generators_pool;

uint8_t PreciseStepping::physical_axis_step_generator_types = CLASSIC_STEP_GENERATOR_X | CLASSIC_STEP_GENERATOR_Y | CLASSIC_STEP_GENERATOR_Z | CLASSIC_STEP_GENERATOR_E;
double PreciseStepping::max_lookback_time = 0.;

uint16_t PreciseStepping::inverted_dirs = 0;
double PreciseStepping::total_print_time = 0.;
xyze_double_t PreciseStepping::total_start_pos = { 0., 0., 0., 0. };
xyze_long_t PreciseStepping::total_start_pos_msteps = { 0, 0, 0, 0 };
PreciseSteppingFlag_t PreciseStepping::flags = 0;

uint32_t PreciseStepping::waiting_before_delivering_start_time = 0;
uint32_t PreciseStepping::last_step_isr_delay = 0;

std::atomic<bool> PreciseStepping::stop_pending = false;
std::atomic<bool> PreciseStepping::busy = false;
volatile uint8_t PreciseStepping::step_dl_miss = 0;
volatile uint8_t PreciseStepping::step_ev_miss = 0;

#if !BOARD_IS_DWARF()
std::atomic<uint32_t> PreciseStepping::stall_count = 0;
#endif

FORCE_INLINE xyze_long_t get_oriented_msteps_from_block(const block_t &block) {
    const xyze_long_t direction = {
        { { (block.direction_bits & _BV(X_AXIS)) ? -1 : 1,
            (block.direction_bits & _BV(Y_AXIS)) ? -1 : 1,
            (block.direction_bits & _BV(Z_AXIS)) ? -1 : 1,
            (block.direction_bits & _BV(E_AXIS)) ? -1 : 1 } }
    };

    return block.msteps.asLong() * direction;
}

FORCE_INLINE double convert_oriented_msteps_to_distance(const int32_t msteps, const uint8_t axis) {
    return msteps * (double)Planner::mm_per_mstep[axis];
}

FORCE_INLINE xyze_double_t convert_oriented_msteps_to_distance(const xyze_long_t &msteps) {
    const xyze_double_t distance_mm = {
        { { (double)msteps.x * (double)Planner::mm_per_mstep[X_AXIS],
            (double)msteps.y * (double)Planner::mm_per_mstep[Y_AXIS],
            (double)msteps.z * (double)Planner::mm_per_mstep[Z_AXIS],
            (double)msteps.e * (double)Planner::mm_per_mstep[E_AXIS] } }
    };

    return distance_mm;
}

FORCE_INLINE MoveFlag_t get_active_axis_flags_from_block(const block_t &block) {
    MoveFlag_t flags = (block.msteps.x > 0 ? MOVE_FLAG_X_ACTIVE : 0)
        | (block.msteps.y > 0 ? MOVE_FLAG_Y_ACTIVE : 0)
        | (block.msteps.z > 0 ? MOVE_FLAG_Z_ACTIVE : 0)
        | (block.msteps.e > 0 ? MOVE_FLAG_E_ACTIVE : 0);
    return flags;
}

FORCE_INLINE bool append_move_segment_to_queue(const double move_time, const double start_v, const double half_accel, const double print_time,
    const xyze_double_t axes_r, const xyze_double_t start_pos, const MoveFlag_t flags) {
    assert(PreciseStepping::total_print_time > 0 && PreciseStepping::total_print_time < MAX_PRINT_TIME);
    uint8_t next_move_segment_queue_head;
    if (move_t *m = PreciseStepping::get_next_free_move_segment(next_move_segment_queue_head); m != nullptr) {
        m->move_time = move_time;
        m->start_v = start_v;
        m->half_accel = half_accel;
        m->print_time = print_time;
        m->axes_r = axes_r;
        m->start_pos = start_pos;
        m->flags = flags;
        m->reference_cnt = 0;
        PreciseStepping::move_segment_queue.head = next_move_segment_queue_head;
        return true;
    }
    return false;
}

FORCE_INLINE xyze_double_t calc_axes_r_from_block(const block_t &block) {
    const double millimeters_inv = 1. / double(block.millimeters);
    xyze_double_t axes_r;

    LOOP_XYZE(i) {
        if (!block.msteps[i]) {
            axes_r[i] = 0.;
        } else {
            axes_r[i] = double(block.msteps[i]) * millimeters_inv * double(Planner::mm_per_mstep[i]);
            if (block.direction_bits & _BV(i)) {
                axes_r[i] *= -1.;
            }
        }
    }

    return axes_r;
}

FORCE_INLINE double calc_velocity_after_acceleration(const double start_v, const double accel, const double dist) {
    // Derived from S = v_0 * t + (a / 2) * t^2 by substitution t = (v - v_0) / a.
    return std::sqrt(2 * dist * accel + SQR(start_v));
}

FORCE_INLINE double calc_distance_required_to_reach_cruise_velocity(const double start_v, const double cruise_v, const double accel) {
    // Derived from S = v_0 * t + (a / 2) * t^2 by substitution t = (v - v_0) / a.
    return (SQR(cruise_v) - SQR(start_v)) / (2. * accel);
}

// Clamp distances close to zero or negative.
FORCE_INLINE double calc_distance_required_to_reach_cruise_velocity_clamped(const double start_v, const double cruise_v, const double accel) {
    if (const double dist_out = calc_distance_required_to_reach_cruise_velocity(start_v, cruise_v, accel); dist_out < EPSILON_DISTANCE) {
        return 0.;
    } else {
        return dist_out;
    }
}

// It assumes that there is no move segment with cruise velocity.
FORCE_INLINE double calc_distance_in_which_we_start_decelerating(const double start_v, const double end_v, const double accel, const double dist) {
    // First, we derived the maximum reachable velocity (v_c) from S = v_s * t_A + (a / 2) * t_A^2 + v_e * t_D + (a / 2) * t_D^2
    // by substituting t_A = (v_c - v_s) / a and t_D = (v_c - v_e) / a.
    // After the substitution, we get v_c = sqrt(2 * S * a + v_s^2 + v_s^2) / 2.
    // Then we derive S_A from S_A = v_s * t_A + (a / 2) * t_A^2 by substitution t_A = (v_c - v_s) / a and then by substitution v_c = sqrt(2 * S * a + v_s^2 + v_s^2) / 2 (from the previous).
    return (2. * dist * accel + SQR(end_v) - SQR(start_v)) / (4. * accel);
}

// Clamp distances close to zero or negative to zero and distances close to "dist" to "dist".
FORCE_INLINE double calc_distance_in_which_we_start_decelerating_clamped(const double start_v, const double end_v, const double accel, const double dist) {
    if (const double dist_out = calc_distance_in_which_we_start_decelerating(start_v, end_v, accel, dist); dist_out <= EPSILON_DISTANCE) {
        return 0.;
    } else if (dist_out > dist - EPSILON_DISTANCE) {
        return dist;
    } else {
        return dist_out;
    }
}

bool append_move_segments_to_queue(const block_t &block) {
    double print_time = PreciseStepping::total_print_time;
    xyze_double_t start_pos = PreciseStepping::total_start_pos;

    const double millimeters = double(block.millimeters);
    const double accel = double(block.acceleration);
    const double start_v = double(block.initial_speed);
    const double end_v = double(block.final_speed);
    double cruise_v = double(block.nominal_speed);

    double accel_dist = calc_distance_required_to_reach_cruise_velocity_clamped(start_v, cruise_v, accel);
    double decel_dist = calc_distance_required_to_reach_cruise_velocity_clamped(end_v, cruise_v, accel);
    double cruise_dist = millimeters - accel_dist - decel_dist;

    if (cruise_dist < EPSILON_DISTANCE) {
        // There is no segment with cruise velocity, or it is very small, and it will be intentionally skipped.
        accel_dist = calc_distance_in_which_we_start_decelerating_clamped(start_v, end_v, accel, millimeters);
        decel_dist = std::max(millimeters - accel_dist, 0.);
        cruise_dist = 0.;

        cruise_v = calc_velocity_after_acceleration(start_v, accel, accel_dist);
    }

    if (uint8_t move_blocks_required = (accel_dist != 0.) + (decel_dist != 0.) + (cruise_dist != 0.); PreciseStepping::move_segment_queue_free_slots() < (move_blocks_required + MOVE_SEGMENT_QUEUE_MIN_FREE_SLOTS)) {
        return false;
    }

    const PreciseSteppingFlag_t old_ps_flags = PreciseStepping::flags;
    const MoveFlag_t active_axis = get_active_axis_flags_from_block(block);
    const xyze_double_t axes_r = calc_axes_r_from_block(block);
    const double half_accel = .5 * accel;

    // Reset position.
    // Because all step event generators accumulate the position in steps, we need to preserve the remaining mini steps
    // to full steps instead of resetting the position to zero.
    LOOP_XYZE(axis) {
        if (const PreciseSteppingFlag_t axis_reset_flag = (PRECISE_STEPPING_FLAG_RESET_POSITION_X << axis); PreciseStepping::flags & axis_reset_flag) {
            PreciseStepping::flags &= ~axis_reset_flag;
            PreciseStepping::total_start_pos_msteps[axis] %= PLANNER_STEPS_MULTIPLIER;
            PreciseStepping::total_start_pos[axis] = convert_oriented_msteps_to_distance(PreciseStepping::total_start_pos_msteps[axis], axis);
            start_pos[axis] = PreciseStepping::total_start_pos[axis];
        }
    }

    if (accel_dist != 0.) {
        const double accel_t = (cruise_v - start_v) / accel;
        MoveFlag_t flags = MOVE_FLAG_ACCELERATION_PHASE
            | MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK
            | ((cruise_dist != 0. || decel_dist != 0.) ? 0x00 : MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK)
            | (uint16_t(block.direction_bits & 0x0F) << MOVE_FLAG_DIR_SHIFT)
            | active_axis
            | (uint32_t(old_ps_flags) << MOVE_FLAG_RESET_POSITION_SHIFT);
        if (!append_move_segment_to_queue(accel_t, start_v, half_accel, print_time, axes_r, start_pos, flags)) {
            bsod("Acceleration move segment wasn't append into the queue.");
        }

        print_time += accel_t;
        start_pos = calc_end_position(start_pos, axes_r, accel_dist);
    }

    if (cruise_dist != 0.) {
        const double cruise_t = cruise_dist / cruise_v;
        MoveFlag_t flags = MOVE_FLAG_CRUISE_PHASE
            | ((accel_dist != 0.) ? 0x00 : MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK)
            | ((decel_dist != 0.) ? 0x00 : MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK)
            | (uint16_t(block.direction_bits & 0x0F) << MOVE_FLAG_DIR_SHIFT)
            | active_axis
            | ((accel_dist != 0.) ? 0x00 : (uint32_t(old_ps_flags) << MOVE_FLAG_RESET_POSITION_SHIFT));
        if (!append_move_segment_to_queue(cruise_t, cruise_v, 0., print_time, axes_r, start_pos, flags)) {
            bsod("Cruise move segment wasn't append into the queue.");
        }

        print_time += cruise_t;
        start_pos = calc_end_position(start_pos, axes_r, cruise_dist);
    }

    if (decel_dist != 0.) {
        const double decel_t = (cruise_v - end_v) / accel;
        MoveFlag_t flags = MOVE_FLAG_DECELERATION_PHASE
            | MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK
            | ((accel_dist != 0. || cruise_dist != 0.) ? 0x00 : MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK)
            | (uint16_t(block.direction_bits & 0x0F) << MOVE_FLAG_DIR_SHIFT)
            | active_axis
            | ((accel_dist != 0. || cruise_dist != 0.) ? 0x00 : (uint32_t(old_ps_flags) << MOVE_FLAG_RESET_POSITION_SHIFT));
        if (!append_move_segment_to_queue(decel_t, cruise_v, -half_accel, print_time, axes_r, start_pos, flags)) {
            bsod("Deceleration move segment wasn't append into the queue.");
        }

        print_time += decel_t;
    }

    PreciseStepping::flags |= active_axis;
    PreciseStepping::total_start_pos_msteps += get_oriented_msteps_from_block(block);
    PreciseStepping::total_start_pos = convert_oriented_msteps_to_distance(PreciseStepping::total_start_pos_msteps);
    PreciseStepping::total_print_time = print_time;

    return true;
}

FORCE_INLINE float calc_time_for_distance(const classic_step_generator_t &step_generator, const float distance) {
    return std::max(calc_time_for_distance(step_generator.start_v, step_generator.accel, distance, step_generator.step_dir), 0.f);
}

float get_move_axis_r(const move_t &move, const int axis) {
#ifdef COREXY
    if (axis == A_AXIS) {
        return float(move.axes_r[X_AXIS]) + float(move.axes_r[Y_AXIS]);
    } else if (axis == B_AXIS) {
        return float(move.axes_r[X_AXIS]) - float(move.axes_r[Y_AXIS]);
    } else {
        return float(move.axes_r[axis]);
    }
#else
    return float(move.axes_r[axis]);
#endif
}

#ifdef COREXY
static bool classic_state_step_dir(classic_step_generator_t &state) {
    float start_v = float(state.start_v);
    float accel = float(state.accel);

    if (start_v < 0.f || (start_v == 0.f && accel < 0.f)) {
        return false;
    } else if (start_v > 0.f || (start_v == 0.f && accel > 0.f)) {
        return true;
    } else {
        assert(start_v == 0.f && state.accel == 0.f);
        return state.step_dir;
    }
}
#endif

FORCE_INLINE void classic_step_generator_update(classic_step_generator_t &step_generator) {
    const uint8_t axis = step_generator.axis;
    const move_t &current_move = *step_generator.current_move;

    // Special case
    if (const float axis_r = get_move_axis_r(current_move, axis); axis_r == 0.f) {
        step_generator.start_v = 0.f;
        step_generator.accel = 0.f;
    } else {
        step_generator.start_v = float(current_move.start_v) * axis_r;
        step_generator.accel = 2.f * float(current_move.half_accel) * axis_r;
    }

#ifdef COREXY
    // TODO @hejllukas: It can be moved into get_move_start_pos().
    if (axis == A_AXIS) {
        step_generator.start_pos = float(current_move.start_pos.x) + float(current_move.start_pos.y);
    } else if (axis == B_AXIS) {
        step_generator.start_pos = float(current_move.start_pos.x) - float(current_move.start_pos.y);
    } else {
        step_generator.start_pos = float(current_move.start_pos[step_generator.axis]);
    }

    if (axis == A_AXIS || axis == B_AXIS) {
        step_generator.step_dir = classic_state_step_dir(step_generator);
    } else {
        step_generator.step_dir = get_move_step_dir(*step_generator.current_move, step_generator.axis);
    }
#else
    step_generator.start_pos = float(current_move.start_pos[step_generator.axis]);
    step_generator.step_dir = get_move_step_dir(*step_generator.current_move, step_generator.axis);
#endif
}

// Reset position in steps (current_distance) based on the difference in the position.
void classic_step_generator_reset_position(classic_step_generator_t &step_generator, step_generator_state_t &step_generator_state, const move_t &next_move) {
    const uint8_t axis = step_generator.axis;
    const move_t &current_move = *step_generator.current_move;
    const double c_end_pos = get_move_end_pos(current_move, axis);
    const double n_start_pos = get_move_start_pos(next_move, axis);

    // In every case, the difference in axis position between next_move and current_move is equal to a whole number of steps.
    const int32_t axis_diff_steps = int32_t(std::round(float(n_start_pos - c_end_pos) * Planner::settings.axis_steps_per_mm[axis]));
    step_generator_state.current_distance[axis] += axis_diff_steps;
}

step_event_info_t classic_step_generator_next_step_event(classic_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    assert(step_generator.current_move != nullptr);
    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };

    const float half_step_dist = Planner::mm_per_half_step[step_generator.axis];
    const float next_target = float(step_generator_state.current_distance[step_generator.axis] + (step_generator.step_dir ? 0 : -1)) * Planner::mm_per_step[step_generator.axis] + half_step_dist;
    const float next_distance = next_target - step_generator.start_pos;
    const float step_time = calc_time_for_distance(step_generator, next_distance);

    // When step_time is infinity, it means that next_distance will never be reached.
    // This happens when next_target exceeds end_position, and deceleration decelerates velocity to zero or negative value.
    // Also, we need to stop when step_time exceeds local_end.
    if (const double step_time_d = double(step_time); step_time_d > (step_generator.current_move->move_time + EPSILON)) {
        if (const move_t *next_move = PreciseStepping::move_segment_queue_next_move(*step_generator.current_move); next_move != nullptr) {
            next_step_event.time = next_move->print_time;

            if (!is_ending_empty_move(*next_move)) {
                next_step_event.flags |= STEP_EVENT_FLAG_KEEP_ALIVE;
                next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_KEEP_ALIVE;
            }

            // Reset position in steps (current_distance) based on the difference in the position.
            if (next_move->flags & (MOVE_FLAG_RESET_POSITION_X << step_generator.axis)) {
                classic_step_generator_reset_position(step_generator, step_generator_state, *next_move);
            }

            // The move segment is fully processed, and in the queue is another unprocessed move segment.
            // So we decrement reference count of the current move segment and increment reference count of next move segment.
            --step_generator.current_move->reference_cnt;
            step_generator.current_move = next_move;
            ++step_generator.current_move->reference_cnt;

            classic_step_generator_update(step_generator);

            // Update the direction and activity flags for the entire next move
            step_generator.move_step_flags = 0;
            step_generator.move_step_flags |= !step_generator.step_dir * (STEP_EVENT_FLAG_X_DIR << step_generator.axis);
            step_generator.move_step_flags |= step_generator.current_move->flags & (STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis);

            PreciseStepping::move_segment_processed_handler();
        } else {
            next_step_event.time = step_generator.current_move->print_time + step_generator.current_move->move_time;
        }
    } else {
        const double elapsed_time = step_time_d + step_generator.current_move->print_time;
        next_step_event.time = elapsed_time;
        next_step_event.flags = STEP_EVENT_FLAG_STEP_X << step_generator.axis;
        next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_VALID;
        step_generator_state.current_distance[step_generator.axis] += (step_generator.step_dir ? 1 : -1);
    }

    // When std::numeric_limits<double>::max() is returned, it means that for the current state of the move segment queue, there isn't any next step event for this axis.
    return next_step_event;
}

void classic_step_generator_init(const move_t &move, classic_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    const uint8_t axis = step_generator.axis;
    step_generator.current_move = &move;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)classic_step_generator_next_step_event;

    // Set the initial direction and activity flags for the entire next move
    step_generator.move_step_flags = 0;
    step_generator.move_step_flags |= move.flags & (STEP_EVENT_FLAG_X_DIR << axis);
    step_generator.move_step_flags |= move.flags & (STEP_EVENT_FLAG_X_ACTIVE << axis);
    move.reference_cnt += 1;

    classic_step_generator_update(step_generator);
}

FORCE_INLINE step_event_info_t step_generator_next_step_event(step_generator_state_t &step_generator_state, const uint8_t axis) {
    const step_event_info_t new_step_event = (*step_generator_state.next_step_func[axis])(static_cast<move_segment_step_generator_t &>(*step_generator_state.step_generator[axis]), step_generator_state);
    if (new_step_event.status == STEP_EVENT_INFO_STATUS_GENERATED_VALID) {
        // a new valid step has been produced: update the cached axis activity flags
        step_generator_state.step_generator[axis]->step_flags = step_generator_state.step_generator[axis]->move_step_flags;
    }
    return new_step_event;
}

// Return true when move is fully processed and there is no other work for this move segment.
// step_event.flags is set to non-zero when a step is produced.
bool generate_next_step_event(step_event_i32_t &step_event, step_generator_state_t &step_state) {
    const step_index_t old_nearest_step_event_idx = step_state.step_event_index[0];

    // Sorting buffer isn't fulfilled for all active axis, so we need to fulfill.
    // So we don't have anything to put into step_event_buffer.
    auto step_status = step_state.step_events[old_nearest_step_event_idx].status;
    if (step_status == STEP_EVENT_INFO_STATUS_GENERATED_VALID || step_status == STEP_EVENT_INFO_STATUS_GENERATED_KEEP_ALIVE) {
        const double step_time_absolute = step_state.step_events[old_nearest_step_event_idx].time;
        const uint64_t step_time_absolute_ticks = uint64_t(step_time_absolute * STEPPER_TICKS_PER_SEC);
        const uint64_t step_time_relative_ticks = step_time_absolute_ticks - step_state.previous_step_time_ticks;
        // FIXME Commented out:
        // probably valid assert, but triggering often and making any other debugging difficult.
        // BFW-5954.
        // assert(step_time_relative_ticks < std::numeric_limits<uint32_t>::max());

        step_event.time_ticks = int32_t(step_time_relative_ticks);
        step_event.flags = step_state.step_events[old_nearest_step_event_idx].flags;
        assert(step_event.flags); // ensure flags are non-zero

        // The timer ticks mustn't be negative in any case. Because if it is negative, there is an issue in the code.
        if (step_event.time_ticks < 0) {
            /*
             * FIXME: Same as above (BFW-5954).
#ifndef NDEBUG
            bsod("Negative step time: %d, flags: %d", static_cast<int>(step_event.time_ticks), step_event.flags);
#endif
*/
            step_event.time_ticks = 0;
        }

        if (step_state.left_insert_start_of_move_segment) {
            step_event.flags |= STEP_EVENT_FLAG_BEGINNING_OF_MOVE_SEGMENT;
            --step_state.left_insert_start_of_move_segment;
        }

        if (step_state.previous_step_time == 0.) {
            step_event.flags |= STEP_EVENT_FLAG_FIRST_STEP_EVENT;
        }

        step_state.previous_step_time = step_time_absolute;
        step_state.previous_step_time_ticks = step_time_absolute_ticks;
    } else {
        // Reset flags to indicate no step has been produced
        step_event.flags = 0;
    }

    // Now we have to compute next step event instead of the one that we putted into step event queue.
    const step_event_info_t new_nearest_step_event = step_generator_next_step_event(step_state, (uint8_t)old_nearest_step_event_idx);
    step_state.step_events[old_nearest_step_event_idx] = new_nearest_step_event;

    // Update nearest step event index.
    step_generator_state_update_nearest_idx(step_state);

    StepEventInfoStatus nearest_step_event_status = step_state.step_events[step_state.step_event_index[0]].status;
    return (nearest_step_event_status == STEP_EVENT_INFO_STATUS_GENERATED_INVALID) || (nearest_step_event_status == STEP_EVENT_INFO_STATUS_GENERATED_PENDING);
}

HAL_MOVE_TIMER_ISR() {
    HAL_timer_isr_prologue(MOVE_TIMER_NUM);
    PreciseStepping::move_isr();
    HAL_timer_isr_epilogue(MOVE_TIMER_NUM);
}

HAL_STEP_TIMER_ISR() {
    if (__HAL_TIM_GET_FLAG(&TimerHandle[STEP_TIMER_NUM].handle, TIM_FLAG_CC1) != RESET) {
        __HAL_TIM_CLEAR_IT(&TimerHandle[STEP_TIMER_NUM].handle, TIM_IT_CC1);
        PreciseStepping::step_isr();

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        // ensure FPU wasn't accidentally used in this ISR for performance reasons
        assert(!(__get_CONTROL() & 0b100) || (FPU->FPCCR & FPU_FPCCR_LSPACT_Msk));
#endif
    }
}

void PreciseStepping::init() {
    PreciseStepping::inverted_dirs = (!INVERT_X_DIR ? STEP_EVENT_FLAG_X_DIR : 0)
        | (!INVERT_Y_DIR ? STEP_EVENT_FLAG_Y_DIR : 0)
        | (!INVERT_Z_DIR ? STEP_EVENT_FLAG_Z_DIR : 0)
        | (!INVERT_E0_DIR ? STEP_EVENT_FLAG_E_DIR : 0);

    // Reset initial direction state
    X_APPLY_DIR((Stepper::last_direction_bits ^ inverted_dirs) & STEP_EVENT_FLAG_X_DIR);
    Y_APPLY_DIR((Stepper::last_direction_bits ^ inverted_dirs) & STEP_EVENT_FLAG_Y_DIR);
    Z_APPLY_DIR((Stepper::last_direction_bits ^ inverted_dirs) & STEP_EVENT_FLAG_Z_DIR);
    E_APPLY_DIR((Stepper::last_direction_bits ^ inverted_dirs) & STEP_EVENT_FLAG_E_DIR);
    Stepper::count_direction.x = (Stepper::last_direction_bits & STEP_EVENT_FLAG_X_DIR) ? -1 : 1;
    Stepper::count_direction.y = (Stepper::last_direction_bits & STEP_EVENT_FLAG_Y_DIR) ? -1 : 1;
    Stepper::count_direction.z = (Stepper::last_direction_bits & STEP_EVENT_FLAG_Z_DIR) ? -1 : 1;
    Stepper::count_direction.e = (Stepper::last_direction_bits & STEP_EVENT_FLAG_E_DIR) ? -1 : 1;
#if HAS_PHASE_STEPPING()
    for (std::size_t i = 0; i != phase_stepping::opts::SUPPORTED_AXIS_COUNT; ++i) {
        PreciseStepping::step_generators_pool.classic_step_generator[i].phase_step_state = &phase_stepping::axis_states[i];
    }
#endif
#ifdef ADVANCED_STEP_GENERATORS
    LOOP_XYZ(i) {
        PreciseStepping::step_generators_pool.input_shaper_step_generator[i].is_state = &InputShaper::is_state[i];
    #if HAS_PHASE_STEPPING()
        if (i < phase_stepping::opts::SUPPORTED_AXIS_COUNT) {
            PreciseStepping::step_generators_pool.input_shaper_step_generator[i].phase_step_state = &phase_stepping::axis_states[i];
        }
    #endif
    }
    PreciseStepping::step_generators_pool.pressure_advance_step_generator_e.pa_state = &PressureAdvance::pressure_advance_state;
#endif

    PreciseStepping::move_segment_queue_clear();
    PreciseStepping::step_event_queue_clear();
    PreciseStepping::reset_from_halt(false);
    PreciseStepping::update_maximum_lookback_time();

    HAL_timer_start(MOVE_TIMER_NUM, MOVE_TIMER_FREQUENCY);
    ENABLE_MOVE_INTERRUPT();
}

void PreciseStepping::reset_from_halt(bool preserve_step_fraction) {
    assert(!PreciseStepping::has_blocks_queued());

    if (!preserve_step_fraction) {
        // rebuild msteps from the stepper counters unconditionally
#ifdef COREXY
        total_start_pos_msteps.x = (stepper.count_position_from_startup.x + stepper.count_position_from_startup.y) * PLANNER_STEPS_MULTIPLIER / 2;
        total_start_pos_msteps.y = (stepper.count_position_from_startup.x - stepper.count_position_from_startup.y) * PLANNER_STEPS_MULTIPLIER / 2;
        total_start_pos_msteps.z = stepper.count_position_from_startup.z * PLANNER_STEPS_MULTIPLIER;
#else
        total_start_pos_msteps = stepper.count_position_from_startup * PLANNER_STEPS_MULTIPLIER;
#endif
    } else {
        // Attempt to recover the step fraction only on axes which weren't interrupted by a stop as
        // the generator state can be ahead of the actual motion. We rely on reset_from_halt being
        // called from the main thread just before clearing the stop_pending flag

#ifdef COREXY
        // Handle XY as linked, resetting both if any is used
        if (PreciseStepping::stop_pending && (PreciseStepping::flags & (PRECISE_STEPPING_FLAG_X_USED | PRECISE_STEPPING_FLAG_Y_USED))) {
            total_start_pos_msteps.x = (stepper.count_position_from_startup.x + stepper.count_position_from_startup.y) * PLANNER_STEPS_MULTIPLIER / 2;
            total_start_pos_msteps.y = (stepper.count_position_from_startup.x - stepper.count_position_from_startup.y) * PLANNER_STEPS_MULTIPLIER / 2;
        }

        if (PreciseStepping::stop_pending && (PreciseStepping::flags & PRECISE_STEPPING_FLAG_Z_USED)) {
            total_start_pos_msteps.z = stepper.count_position_from_startup.z * PLANNER_STEPS_MULTIPLIER;
        }
#else
        LOOP_XYZ(i) {
            if (PreciseStepping::stop_pending && (PreciseStepping::flags & (PRECISE_STEPPING_FLAG_X_USED << i))) {
                total_start_pos_msteps[i] = stepper.count_position_from_startup[i] * PLANNER_STEPS_MULTIPLIER;
            }
        }
#endif
    }

    // Because of pressure advance, the amount of material in total_start_pos_msteps doesn't
    // have to equal to step_generator_state.current_distance.e. So we always reset extrude
    // steps to zero because losing a fraction of a step in the E-axis shouldn't cause any
    // issues.
    total_start_pos_msteps.e = 0;

    total_start_pos = convert_oriented_msteps_to_distance(total_start_pos_msteps);

#if HAS_PHASE_STEPPING()
    #ifdef COREXY
    phase_stepping::set_phase_origin(X_AXIS, total_start_pos[X_AXIS] + total_start_pos[Y_AXIS]);
    phase_stepping::set_phase_origin(Y_AXIS, total_start_pos[X_AXIS] - total_start_pos[Y_AXIS]);
    #else
    phase_stepping::set_phase_origin(X_AXIS, total_start_pos[X_AXIS]);
    phase_stepping::set_phase_origin(Y_AXIS, total_start_pos[Y_AXIS]);
    #endif
#endif

    PreciseStepping::step_generator_state_clear();
    PreciseStepping::total_print_time = 0.;
    PreciseStepping::flags = 0;
}

// Update the merged axis activity/direction flags from all generators
void update_step_generator_state_current_flags() {
    PreciseStepping::step_generator_state.current_flags = 0;
    for (uint8_t i = 0; i != PS_AXIS_COUNT; ++i) {
        const auto axis_flags = PreciseStepping::step_generator_state.step_generator[i]->step_flags;
        // ensure each generator is setting only per-axis direction/active flags
        assert(!(axis_flags & ~((STEP_EVENT_FLAG_X_DIR | STEP_EVENT_FLAG_X_ACTIVE) << i)));
        PreciseStepping::step_generator_state.current_flags |= axis_flags;
    }
}

uint16_t PreciseStepping::process_one_step_event_from_queue() {
    // If no queued step event, just wait some time for the next move.
    uint16_t ticks_to_next_isr = STEPPER_ISR_PERIOD_IN_TICKS;

    if (const step_event_u16_t *step_event = get_current_step_event(); step_event != nullptr) {
        const StepEventFlag_t step_flags = step_event->flags;
        const StepEventFlag_t step_dir = (step_flags & STEP_EVENT_FLAG_DIR_MASK);
        const StepEventFlag_t step_dir_inv = (step_dir ^ PreciseStepping::inverted_dirs);
        const StepEventFlag_t axis_move = (step_flags & STEP_EVENT_FLAG_AXIS_ACTIVE_MASK);

        if (step_flags & STEP_EVENT_FLAG_BEGINNING_OF_MOVE_SEGMENT) {
            // a new move is about to start (or a discarding event has been requested): discard the previous one
            if (const move_t *current_move = get_current_move_segment(); current_move->flags & MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK) {
                // discard the current block if this move is also the last move segment of a block
                block_t *current_block = Planner::get_current_processed_block();
                if (current_block->flag.sync_position) {
                    Stepper::_set_position(current_block->sync_step_position);
                }
                Planner::discard_current_block();
                Stepper::count_position_last_block = Stepper::count_position;
            }
            discard_current_move_segment();
        }

        discard_current_step_event();

        Stepper::axis_did_move = uint8_t(axis_move >> STEP_EVENT_FLAG_AXIS_ACTIVE_SHIFT);

        // Most of the time, direction signal are the same, so we will write on those pins just when direction changes.
        if (const uint8_t changed_dir_bits = uint8_t(step_dir >> STEP_EVENT_FLAG_DIR_SHIFT) ^ Stepper::last_direction_bits; changed_dir_bits) {
            Stepper::last_direction_bits = uint8_t(step_dir >> STEP_EVENT_FLAG_DIR_SHIFT);

            if (!phase_stepping::is_enabled(X_AXIS) && TEST(changed_dir_bits, X_AXIS)) {
                X_APPLY_DIR(step_dir_inv & STEP_EVENT_FLAG_X_DIR);
                Stepper::count_direction.x = (step_dir & STEP_EVENT_FLAG_X_DIR) ? -1 : 1;
            }

            if (!phase_stepping::is_enabled(Y_AXIS) && TEST(changed_dir_bits, Y_AXIS)) {
                Y_APPLY_DIR(step_dir_inv & STEP_EVENT_FLAG_Y_DIR);
                Stepper::count_direction.y = (step_dir & STEP_EVENT_FLAG_Y_DIR) ? -1 : 1;
            }

            if (!phase_stepping::is_enabled(Z_AXIS) && TEST(changed_dir_bits, Z_AXIS)) {
                Z_APPLY_DIR(step_dir_inv & STEP_EVENT_FLAG_Z_DIR);
                Stepper::count_direction.z = (step_dir & STEP_EVENT_FLAG_Z_DIR) ? -1 : 1;
            }

            if (!phase_stepping::is_enabled(E_AXIS) && TEST(changed_dir_bits, E_AXIS)) {
                E_APPLY_DIR(step_dir_inv & STEP_EVENT_FLAG_E_DIR);
                Stepper::count_direction.e = (step_dir & STEP_EVENT_FLAG_E_DIR) ? -1 : 1;
            }
        }

        if (step_flags & STEP_EVENT_FLAG_STEP_X) {
            X_STEP_SET();
            Stepper::count_position.x += Stepper::count_direction.x;
            Stepper::count_position_from_startup.x += Stepper::count_direction.x;
            X_STEP_RESET();
        }

        if (step_flags & STEP_EVENT_FLAG_STEP_Y) {
            Y_STEP_SET();
            Stepper::count_position.y += Stepper::count_direction.y;
            Stepper::count_position_from_startup.y += Stepper::count_direction.y;
            Y_STEP_RESET();
        }

        if (step_flags & STEP_EVENT_FLAG_STEP_Z) {
            Z_STEP_SET();
            Stepper::count_position.z += Stepper::count_direction.z;
            Stepper::count_position_from_startup.z += Stepper::count_direction.z;
            Z_STEP_RESET();
        }

        if (step_flags & STEP_EVENT_FLAG_STEP_E) {
            E_STEP_SET();
            Stepper::count_position.e += Stepper::count_direction.e;
            Stepper::count_position_from_startup.e += Stepper::count_direction.e;
            E_STEP_RESET();
        }

        if (step_event_u16_t *next_step_event = get_current_step_event(); next_step_event != nullptr) {
            ticks_to_next_isr = next_step_event->time_ticks;
        } else if ((step_flags & STEP_EVENT_FLAG_END_OF_MOTION) == false) {
            ++step_ev_miss;
        }
    } else {
        // The step event queue drained or ended
        Stepper::axis_did_move = 0;
    }

    return ticks_to_next_isr;
}

static int32_t counter_signed_diff(uint16_t timestamp1, uint16_t timestamp2) {
    int32_t diff = int32_t(timestamp1) - int32_t(timestamp2);
    if (diff > 0xFFFF / 2 - 1) {
        diff -= 0xFFFF;
    } else if (diff < -0xFFFF / 2) {
        diff += 0xFFFF;
    }
    return diff;
}

void PreciseStepping::step_isr() {
#ifndef ISR_DEADLINE_TRACKING
    constexpr uint16_t min_delay = 6; // fuse isr for steps below this threshold (us)
#else
    constexpr uint32_t min_delay = 11; // fuse isr for steps below this threshold (us)
#endif
    constexpr uint16_t min_reserve = 5; // minimum interval for isr re-entry (us)
    constexpr uint16_t max_ticks = (UINT16_MAX / 2); // maximum isr interval for skip detection (us)

#ifdef ISR_DEADLINE_TRACKING
    // in addition to checking for forward misses, check for past ones
    static uint32_t scheduled_ts = 0;
    if (scheduled_ts && ticks_us() > scheduled_ts + min_reserve * 2) {
        ++step_dl_miss;
    }
#endif

    const uint32_t compare = __HAL_TIM_GET_COMPARE(&TimerHandle[STEP_TIMER_NUM].handle, TIM_CHANNEL_1);

    uint32_t next = 0;
    uint32_t time_increment = 0;
    uint32_t timer_remaining_time = 0;
    do {
        if (stop_pending)
            [[unlikely]] {
            next = compare + STEPPER_ISR_PERIOD_IN_TICKS;
            Stepper::axis_did_move = 0;
            break;
        }

        if (!left_ticks_to_next_step_event) {
            left_ticks_to_next_step_event = process_one_step_event_from_queue();
        }

        // limit the interval to avoid a counter overflow or runout
        uint16_t ticks_to_next_step_event = left_ticks_to_next_step_event;
        NOMORE(ticks_to_next_step_event, max_ticks);

        // Compute the time remaining until the next step event.
        left_ticks_to_next_step_event -= ticks_to_next_step_event;

        // Compute the number of ticks for the next ISR.
        time_increment += ticks_to_next_step_event;
        if (ticks_to_next_step_event <= min_delay) {
            continue;
        }

        next = compare + time_increment;

        const uint32_t counter = __HAL_TIM_GET_COUNTER(&TimerHandle[STEP_TIMER_NUM].handle);
        timer_remaining_time = next - counter;
    } while (timer_remaining_time < min_reserve || (timer_remaining_time & 0xFFFF) > max_ticks);

    // What follows is a not-straightforward scheduling of the next step ISR
    // time event. If this ISR is interrupted by another routine, we might
    // schedule an event that is in the past from the perspective of the step
    // timer. Thus, we check if this is the case and possibly adjust the time.
    // On the next ISR we will pick up the slack.
    const auto timer_handle_ptr = &TimerHandle[STEP_TIMER_NUM].handle;
    uint32_t adjusted_next = next - last_step_isr_delay;
    __disable_irq();
    int32_t tim_counter = __HAL_TIM_GET_COUNTER(timer_handle_ptr);
    int32_t diff = counter_signed_diff(adjusted_next, tim_counter);
    if (diff < min_reserve) {
        adjusted_next = tim_counter + min_reserve;
    }
    __HAL_TIM_SET_COMPARE(timer_handle_ptr, TIM_CHANNEL_1, adjusted_next);
    __enable_irq();

    if (diff < min_delay) {
        last_step_isr_delay = -diff + min_reserve;
    } else {
        last_step_isr_delay = 0;
    }

#ifdef ISR_DEADLINE_TRACKING
    uint32_t scheduled_ticks = (((next & 0xFFFF) - __HAL_TIM_GET_COUNTER(&TimerHandle[STEP_TIMER_NUM].handle)) & 0xFFFF);
    scheduled_ts = ticks_us() + scheduled_ticks;
#endif
}

FORCE_INLINE move_t *append_beginning_empty_move() {
    assert(PreciseStepping::total_print_time == 0.);
    uint8_t next_move_segment_queue_head = 0;
    move_t *move = PreciseStepping::get_next_free_move_segment(next_move_segment_queue_head);
    if (move != nullptr) {
        move->flags = MOVE_FLAG_BEGINNING_EMPTY_MOVE;
        move->start_v = 0.;
        move->half_accel = 0.;
        move->axes_r = { 0., 0., 0., 0. };
        move->move_time = PreciseStepping::max_lookback_time + 0.001; // For now, the epsilon o 1ms is applied to ensure that even with big rounding errors, move_time will be much bigger than max_lookback_time.
        move->start_pos = PreciseStepping::total_start_pos;
        move->print_time = 0.;
        move->reference_cnt = 0;
        PreciseStepping::move_segment_queue.head = next_move_segment_queue_head;
        PreciseStepping::total_print_time = move->print_time + move->move_time;
    }

    return move;
}

FORCE_INLINE move_t *append_block_discarding_move() {
    assert(PreciseStepping::total_print_time > 0 && PreciseStepping::total_print_time < MAX_PRINT_TIME);
    uint8_t next_move_segment_queue_head = 0;
    move_t *move = PreciseStepping::get_next_free_move_segment(next_move_segment_queue_head);
    if (move != nullptr) {
        move->flags = MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK | MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK;
        move->start_v = 0.;
        move->half_accel = 0.;
        move->axes_r = { 0., 0., 0., 0. };
        move->move_time = 0.;
        move->start_pos = PreciseStepping::total_start_pos;
        move->print_time = PreciseStepping::total_print_time;
        move->reference_cnt = 0;
        PreciseStepping::move_segment_queue.head = next_move_segment_queue_head;
    }

    return move;
}

FORCE_INLINE move_t *append_ending_empty_move() {
    assert(PreciseStepping::total_print_time > 0 && PreciseStepping::total_print_time < MAX_PRINT_TIME);
    uint8_t next_move_segment_queue_head = 0;
    move_t *move = PreciseStepping::get_next_free_move_segment(next_move_segment_queue_head);
    if (move != nullptr) {
        move->flags = MOVE_FLAG_ENDING_EMPTY_MOVE;
        move->start_v = 0.;
        move->half_accel = 0.;
        move->axes_r = { 0., 0., 0., 0. };
        move->move_time = MAX_PRINT_TIME;
        move->start_pos = PreciseStepping::total_start_pos;
        move->print_time = PreciseStepping::total_print_time;
        move->reference_cnt = 0;
        PreciseStepping::move_segment_queue.head = next_move_segment_queue_head;
        PreciseStepping::total_print_time = move->print_time + move->move_time;
    }

    return move;
}

FORCE_INLINE bool append_move_discarding_step_event(step_generator_state_t &step_state, StepEventFlag_t extra_step_flags = 0) {
    uint16_t next_step_event_queue_head = 0;
    if (step_event_u16_t *step_event = PreciseStepping::get_next_free_step_event(next_step_event_queue_head); step_event != nullptr) {
        step_event->time_ticks = 0;
        step_event->flags = step_state.current_flags | STEP_EVENT_FLAG_BEGINNING_OF_MOVE_SEGMENT | extra_step_flags;

        PreciseStepping::step_event_queue.head = next_step_event_queue_head;
        step_state.previous_step_time = 0.;
        step_state.previous_step_time_ticks = 0;
        return true;
    }
    return false;
}

bool PreciseStepping::is_waiting_before_delivering() {
    if (Planner::delay_before_delivering) {
        if (waiting_before_delivering_start_time == 0) {
            waiting_before_delivering_start_time = ticks_ms();
            return true;
        } else if (Planner::nonbusy_movesplanned() >= 3 || (ticks_ms() - waiting_before_delivering_start_time) >= Planner::delay_before_delivering) {
            Planner::delay_before_delivering = 0;
            waiting_before_delivering_start_time = 0;
        } else {
            return true;
        }
    }

    if (const uint8_t waiting_for_discard = Planner::movesplanned_processed(); waiting_for_discard >= (BLOCK_BUFFER_SIZE / 2)) {
        // In case the block queue contains plenty of short blocks waiting for discarding and step generators are unable to produce new
        // step events, we have to ensure that the next block can be processed (or the empty move segment can be placed into the queue).
        if (PreciseStepping::get_nearest_step_event_status() == STEP_EVENT_INFO_STATUS_GENERATED_INVALID) {
            return false;
        }

        return true;
    }

    return false;
}

void PreciseStepping::process_queue_of_blocks() {
    if (is_waiting_before_delivering()) {
        return;
    }

    // When the ending move segment is on the bottom of the queue (then Planner::total_print_time
    // contains the value bigger then MAX_PRINT_TIME) we're waiting for motion to halt and reset.
    if (PreciseStepping::total_print_time >= MAX_PRINT_TIME) {
        // ensure all motion has stopped
        if (has_blocks_queued() || phase_stepping::processing()) {
            return;
        }

        // we can now reset to a halt
        reset_from_halt();
    }

    // fetch next block
    block_t *current_block;
    while ((current_block = Planner::get_current_unprocessed_block()) != nullptr) {
        if (current_block->is_move()) {
            // block is a regular block, proceed normally
            break;
        }

        // process sync blocks directly when motion wasn't started already.
        if (PreciseStepping::total_print_time == 0.) {
            assert(!PreciseStepping::has_blocks_queued());
            Stepper::_set_position(current_block->sync_step_position);
            Planner::discard_current_unprocessed_block();
            Planner::discard_current_block();
            continue;
        }

        // sync blocks should only be queued if motion was already started;
        // the counters should be manipulated directly otherwise
        assert(PreciseStepping::total_print_time != 0.);

        if (!append_block_discarding_move()) {
            return;
        }

        // To avoid accumulating E-axis into very big numbers that are causing numerical issues, we reset
        // the E-axis position with every SYNC block.
        // Other axes don't need that.
        // @hejllukas: If we sometimes implement reset for input shaper and decide to synchronize total_start_pos
        //             with the Planner position, we should introduce something like SYNC move segment and replace
        //             append_block_discarding_move() with that. That should simplify the resetting mechanism.
        if (current_block->is_sync() && current_block->sync_step_position.e == 0) {
            PreciseStepping::flags |= PRECISE_STEPPING_FLAG_RESET_POSITION_E;
        }

        // pass-through SYNC blocks, they will be processed in the ISR
        Planner::discard_current_unprocessed_block();
    }

    if (current_block == nullptr) {
        if (PreciseStepping::total_print_time && PreciseStepping::get_nearest_step_event_status() == STEP_EVENT_INFO_STATUS_GENERATED_INVALID) {
            // motion was already started and the move queue is about to (or ran) dry: enqueue an end block
            append_ending_empty_move();
#if !BOARD_IS_DWARF()
            stall_count++;
#endif
        } else if (PreciseStepping::total_print_time == 0. && busy) {
            // motion reset has completed and there is no pending block to process, we're now free
            assert(!has_blocks_queued() && !phase_stepping::processing());
            busy = false;
        }
        return;
    }

    if (PreciseStepping::total_print_time == 0.) {
        // we're restarting from zero, prepend a beginning move
        if (!append_beginning_empty_move()) {
            return;
        }
        busy = true;
    }

    if (append_move_segments_to_queue(*current_block)) {
        Planner::discard_current_unprocessed_block();
    }
}

void PreciseStepping::loop() {
    if (stop_pending) {
        reset_queues();
        return;
    }

#ifdef ISR_DEADLINE_DEBUGGING
    uint8_t step_dl_miss_buf = step_dl_miss;
    if (step_dl_miss_buf) {
        step_dl_miss_buf = __atomic_exchange_n(&step_dl_miss, 0, __ATOMIC_RELAXED);
        SERIAL_ECHOLNPAIR("STEP DEADLINES MISSED: ", step_dl_miss_buf);
        Sound_Play(eSOUND_TYPE::SingleBeep);
    }
#endif
#ifdef ISR_EVENT_DEBUGGING
    uint8_t step_ev_miss_buf = step_ev_miss;
    if (step_ev_miss_buf) {
        step_ev_miss_buf = __atomic_exchange_n(&step_ev_miss, 0, __ATOMIC_RELAXED);
        SERIAL_ECHOLNPAIR("STEP EVENTS MISSED: ", step_ev_miss_buf);
        Sound_Play(eSOUND_TYPE::SingleBeep);
    }
#endif
}

void PreciseStepping::move_isr() {
    if (stop_pending) {
        return;
    }

    StepGeneratorStatus status = process_one_move_segment_from_queue();
    if (status == STEP_GENERATOR_STATUS_OK) {
        // we produced enough steps in this iteration:
        // stop immediately to avoid taking too much time
        return;
    } else if (status == STEP_GENERATOR_STATUS_FULL_STEP_EVENT_QUEUE) {
        // full queue directly on the first iteration:
        // spare some extra time to process a new block ahead of time
        process_queue_of_blocks();
        return;
    }

    // we produced no steps and/or one of the generators reached the end of the move queue:
    // at this point we need to keep trying advancing the block queue in order to allow
    // generators to continue producing steps or we risk that a slew of short segments
    // causes too few steps to be produced per iteration, eventually running it dry
    assert(status == STEP_GENERATOR_STATUS_NO_STEP_EVENT_PRODUCED);

    // Until we break from this loop, no new blocks are appended into the block queue.
    // To ensure that we are never stuck in the infinite loop (when some unexpected state happens),
    // we will limit the number of iterations by the number of all blocks + 1.
    // +1 is there to make one additional call when all blocks are processed because this additional
    // call can append the ending empty move segment when all blocks were already processed.
    for (uint16_t i = 0; i <= Planner::movesplanned(); ++i) {
        process_queue_of_blocks();
        if (!has_unprocessed_move_segments_queued()) {
            // the queue didn't avance: we're stuck
            break;
        }

        status = process_one_move_segment_from_queue();
        if (status != STEP_GENERATOR_STATUS_NO_STEP_EVENT_PRODUCED) {
            // all generators are finally producing steps
            break;
        }
    }
}

// Contains step event that is split into two parts to handle cases when time_ticks in step_event_i32_t doesn't fit time_ticks
// in step_event_u16_t. The first part is called an empty step event that could be zero (when time_ticks in step_event_i32_t
// fits time_ticks in step_event_u16_t) or several empty step events. Those step events have modified flags, and time_ticks
// will be STEP_TIMER_MAX_TICKS_LIMIT. The second part is the actual step event.
struct split_step_event_t {
    int32_t empty_step_event_cnt = 0;
    StepEventFlag_t empty_step_event_flags = 0;
    uint16_t last_step_event_time_ticks = 0;
    StepEventFlag_t last_step_event_flags = 0;
};

FORCE_INLINE split_step_event_t split_buffered_step(const step_generator_state_t &step_generator_state) {
    split_step_event_t split_step_event = { 0, 0, 0, 0 };
    if (step_generator_state.buffered_step.flags) {
        if (step_generator_state.buffered_step.time_ticks <= STEP_TIMER_MAX_TICKS_LIMIT) {
            split_step_event.last_step_event_time_ticks = step_generator_state.buffered_step.time_ticks;
            split_step_event.last_step_event_flags = step_generator_state.buffered_step.flags;
        } else {
            split_step_event.empty_step_event_cnt = (step_generator_state.buffered_step.time_ticks - 1) / STEP_TIMER_MAX_TICKS_LIMIT;
            split_step_event.last_step_event_time_ticks = step_generator_state.buffered_step.time_ticks - split_step_event.empty_step_event_cnt * STEP_TIMER_MAX_TICKS_LIMIT;

            // We left only the direction and the active axes flags because other flags have to be present only in the last step event.
            split_step_event.empty_step_event_flags = step_generator_state.buffered_step.flags & (STEP_EVENT_FLAG_DIR_MASK | STEP_EVENT_FLAG_AXIS_ACTIVE_MASK);
            split_step_event.last_step_event_flags = step_generator_state.buffered_step.flags;
        }
    }

    return split_step_event;
}

FORCE_INLINE void trigger_first_step_event_after_specified_ticks(const uint32_t ticks) {
    assert(ticks <= STEP_TIMER_MAX_TICKS_LIMIT);

    DISABLE_STEPPER_DRIVER_INTERRUPT();
    const uint16_t counter = __HAL_TIM_GET_COUNTER(&TimerHandle[STEP_TIMER_NUM].handle);
    const uint16_t deadline = counter + ticks;
    __HAL_TIM_SET_COMPARE(&TimerHandle[STEP_TIMER_NUM].handle, TIM_CHANNEL_1, deadline);
    ENABLE_STEPPER_DRIVER_INTERRUPT();
}

FORCE_INLINE void append_split_step_event(const split_step_event_t &split_step_event, step_event_u16_t *&next_step_event, uint16_t &next_step_event_queue_head) {
    assert(next_step_event != nullptr);
    assert(split_step_event.empty_step_event_cnt + 1 <= PreciseStepping::step_event_queue_free_slots());

    for (int32_t empty_step_event_idx = 0; empty_step_event_idx < split_step_event.empty_step_event_cnt; ++empty_step_event_idx) {
        if ((split_step_event.last_step_event_flags & STEP_EVENT_FLAG_FIRST_STEP_EVENT) && empty_step_event_idx == 0) {
            trigger_first_step_event_after_specified_ticks(STEP_TIMER_MAX_TICKS_LIMIT);
        }

        next_step_event->time_ticks = STEP_TIMER_MAX_TICKS_LIMIT;
        next_step_event->flags = split_step_event.empty_step_event_flags;
        PreciseStepping::step_event_queue.head = next_step_event_queue_head;

        // advance in the queue: the required space should be already checked-for before calling append!
        next_step_event = PreciseStepping::get_next_free_step_event(next_step_event_queue_head);
        assert(next_step_event);
    }

    if ((split_step_event.last_step_event_flags & STEP_EVENT_FLAG_FIRST_STEP_EVENT) && split_step_event.empty_step_event_cnt == 0) {
        trigger_first_step_event_after_specified_ticks(split_step_event.last_step_event_time_ticks);
    }

    next_step_event->time_ticks = split_step_event.last_step_event_time_ticks;
    next_step_event->flags = split_step_event.last_step_event_flags;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    // next_step_event_queue_head is guaranteed to be set when next_step_event is
    // non-null, which we already checked before calling this function, that there
    // is enough space for all step events and also above (when null the new step
    // is immediately buffered instead in order to produce max 1 step per cycle)
    PreciseStepping::step_event_queue.head = next_step_event_queue_head;
#pragma GCC diagnostic pop
}

#ifndef NDEBUG
/// @brief Ensure a new step event never exceeds the time of the last move in the queue
static void check_step_time(const step_event_i32_t &step_event) {
    // Due to the buffered step, the maximum time delta of a new step might refer to a move which
    // has just been processed
    move_t *prev_move = PreciseStepping::get_last_processed_move_segment();
    if (prev_move == nullptr) {
        // ... unless it's also the first move being processed
        prev_move = PreciseStepping::get_current_move_segment();

        // the move queue should never be empty though, as the move should only be processed (and
        // discarded) when all step events for it have been consumed.
        assert(prev_move != nullptr);
    }
    const double prev_move_time = prev_move->print_time;

    // Fetch the last move, as the produced step event can span past the current move
    const move_t *last_move = PreciseStepping::get_last_move_segment();
    assert(last_move != nullptr);

    double last_move_time;
    if (is_ending_empty_move(*last_move)) {
        // When calculating the absolute move end time restrict the limit on ending empty moves to
        // ensure that any scheduled keep-alive event is never split past the end of motion.
        last_move_time = (STEP_TIMER_MAX_TICKS_LIMIT / STEPPER_TICKS_PER_SEC);
    } else {
        last_move_time = last_move->move_time;
        if (last_move == prev_move) {
            // We have no look-ahead, but we must still account for the possibility of the last step
            // being just past the end of the current move
            last_move_time += (STEP_TIMER_MAX_TICKS_LIMIT / STEPPER_TICKS_PER_SEC);
        }
    }
    const double last_move_time_end = last_move->print_time + last_move_time;
    assert(last_move_time_end >= prev_move_time);

    const int32_t max_move_ticks = float(last_move_time_end - prev_move_time) * float(STEPPER_TICKS_PER_SEC);
    assert(step_event.time_ticks <= max_move_ticks);
}
#endif

StepGeneratorStatus PreciseStepping::process_one_move_segment_from_queue() {
    uint16_t produced_step_events_cnt = 0;

    if (const move_t *move = get_current_unprocessed_move_segment(); move != nullptr) {
        if (!step_generator_state.initialized) {
            assert(is_beginning_empty_move(*move));
            step_generator_state_init(*move);
        }

        for (; produced_step_events_cnt < MAX_STEP_EVENTS_PRODUCED_PER_ONE_CALL; ++produced_step_events_cnt) {
            uint16_t next_step_event_queue_head;
            step_event_u16_t *next_step_event = PreciseStepping::get_next_free_step_event(next_step_event_queue_head);

            // ensure there is space free for flushing the buffered step
            split_step_event_t split_step_event;
            if (step_generator_state.buffered_step.flags) {
                if (next_step_event == nullptr) {
                    return STEP_GENERATOR_STATUS_FULL_STEP_EVENT_QUEUE;
                }
                split_step_event = split_buffered_step(step_generator_state);
                if (split_step_event.empty_step_event_cnt && split_step_event.empty_step_event_cnt + 1 > step_event_queue_free_slots()) {
                    return STEP_GENERATOR_STATUS_FULL_STEP_EVENT_QUEUE;
                }
            }

            step_event_i32_t new_step_event;
            bool done = generate_next_step_event(new_step_event, step_generator_state);

            // accumulate into or flush the buffered step
            if (new_step_event.flags) {
                // a new step event was produced
#ifndef NDEBUG
                check_step_time(new_step_event);
#endif

                // Update the current axis flags and merge them into the new step
                update_step_generator_state_current_flags();
                new_step_event.flags |= step_generator_state.current_flags;

                if (!step_generator_state.buffered_step.flags) {
                    // no previous buffer: replace
                    step_generator_state.buffered_step = new_step_event;
                } else if (new_step_event.time_ticks == 0 // zero delay
                    && !((step_generator_state.buffered_step.flags & new_step_event.flags)
                        & ((STEP_EVENT_FLAG_AXIS_MASK | STEP_EVENT_FLAG_AXIS_OTHER_MASK) & ~STEP_EVENT_FLAG_KEEP_ALIVE)) // no step or move flag overlap except keep-alive flag
                    && !((step_generator_state.buffered_step.flags ^ new_step_event.flags) & STEP_EVENT_FLAG_DIR_MASK) // identical direction flags
                ) {
                    // TODO @wavexx: there are currently cases where a useless change in direction
                    //   in a disabled axis is preventing legitimate steps to be merged. This can't
                    //   be handled here, as we might want a direction change to be delivered before
                    //   the step itself.

                    // merge allowed: accumulate step into buffer
                    step_generator_state.buffered_step.flags |= new_step_event.flags;
                } else {
                    // merge disallowed: flush buffer and replace
                    assert(split_step_event.last_step_event_time_ticks <= STEP_TIMER_MAX_TICKS_LIMIT);
                    append_split_step_event(split_step_event, next_step_event, next_step_event_queue_head);
                    step_generator_state.buffered_step = new_step_event;
                }
            }

            if (done) {
                // the move is complete: if we just flushed a buffered step and produced new one in
                // the same iteration then we need to check for a free slot again; do so just below
                // by resetting produced_step_events_cnt (we don't need the counter anymore)
                produced_step_events_cnt = 0;
                break;
            }
        }
    }

    if (!produced_step_events_cnt) {
        // no moves or end of steps, check if we're waiting on the ending move
        if (const move_t *unprocessed_move = get_current_unprocessed_move_segment();
            unprocessed_move != nullptr && is_ending_empty_move(*unprocessed_move)) {

            // check if a buffered step is present and flush it first
            if (step_generator_state.buffered_step.flags) {
                uint16_t next_step_event_queue_head;
                step_event_u16_t *next_step_event = PreciseStepping::get_next_free_step_event(next_step_event_queue_head);
                if (next_step_event == nullptr) {
                    return STEP_GENERATOR_STATUS_FULL_STEP_EVENT_QUEUE;
                }

                split_step_event_t split_step_event = split_buffered_step(step_generator_state);
                if (split_step_event.empty_step_event_cnt && split_step_event.empty_step_event_cnt + 1 > step_event_queue_free_slots()) {
                    return STEP_GENERATOR_STATUS_FULL_STEP_EVENT_QUEUE;
                }

                assert(split_step_event.last_step_event_time_ticks <= STEP_TIMER_MAX_TICKS_LIMIT);
                append_split_step_event(split_step_event, next_step_event, next_step_event_queue_head);
                step_generator_state.buffered_step.flags = 0;
            }

            // Place discarding step events (just empty step events) into the step event queue if there are
            // still remaining step events that have to be marked to free already processed move segments.
            while (step_generator_state.left_insert_start_of_move_segment > 0 && append_move_discarding_step_event(step_generator_state)) {
                --step_generator_state.left_insert_start_of_move_segment;
            }

            assert(step_generator_state.left_insert_start_of_move_segment >= 0);

            // The step event queue can be full, and all left_insert_start_of_move_segment couldn't be all processed.
            // So discard this move only when left_insert_start_of_move_segment is fully processed.
            if (step_generator_state.left_insert_start_of_move_segment == 0) {
                // we reached an explicit end block with all generators waiting on it, we can advance
                if (!is_step_event_queue_full()) {
                    discard_current_unprocessed_move_segment();
                    [[maybe_unused]] const bool appended = append_move_discarding_step_event(step_generator_state, STEP_EVENT_FLAG_END_OF_MOTION);
                    assert(appended);
                }

                assert(step_generator_state.left_insert_start_of_move_segment == 0);
            }
        }
    }

    return (produced_step_events_cnt == 0 ? STEP_GENERATOR_STATUS_NO_STEP_EVENT_PRODUCED : STEP_GENERATOR_STATUS_OK);
}

void PreciseStepping::update_maximum_lookback_time() {
    max_lookback_time = 0.;

#ifdef ADVANCED_STEP_GENERATORS
    LOOP_XYZ(i) {
        if (physical_axis_step_generator_types & (INPUT_SHAPER_STEP_GENERATOR_X << i)) {
    #ifdef COREXY
            if ((physical_axis_step_generator_types & INPUT_SHAPER_STEP_GENERATOR_X) || (physical_axis_step_generator_types & INPUT_SHAPER_STEP_GENERATOR_Y)) {
                max_lookback_time = std::max(max_lookback_time, std::max(-InputShaper::logical_axis_pulses[X_AXIS].pulses[0].t, -InputShaper::logical_axis_pulses[Y_AXIS].pulses[0].t));
            } else {
                max_lookback_time = std::max(max_lookback_time, -InputShaper::logical_axis_pulses[i].pulses[0].t);
            }
    #else
            max_lookback_time = std::max(max_lookback_time, -InputShaper::logical_axis_pulses[i].pulses[0].t);
    #endif
        }
    }
    if (physical_axis_step_generator_types & PRESSURE_ADVANCE_STEP_GENERATOR_E) {
        const pressure_advance_params_t &pa_params = PressureAdvance::pressure_advance_params;
        max_lookback_time = std::max(max_lookback_time, pa_params.filter_total_time);
    }
#endif
}

void PreciseStepping::step_generator_state_init(const move_t &move) {
    assert(is_beginning_empty_move(move));
    if (max_lookback_time > move.move_time) {
        bsod("Max lookback time exceeds the length of the beginning empty move segment.");
    }

    step_generator_state.previous_step_time = 0.;
    step_generator_state.previous_step_time_ticks = 0;
    step_generator_state.buffered_step.flags = 0;
    step_generator_state.current_distance = stepper.count_position_from_startup;
    step_generator_state.current_distance.e = 0;
    step_generator_state.left_insert_start_of_move_segment = 0;
    step_generator_state.initial_time = ticks_us();

    // Reset step events and index
    for (step_index_t i = 0; i != step_generator_state.step_event_index.size(); ++i) {
        step_generator_state.step_event_index[i] = i;
    }

    for (step_event_info_t &step_event_info : step_generator_state.step_events) {
        step_event_info.time = 0.;
        step_event_info.flags = 0;
        step_event_info.status = STEP_EVENT_INFO_STATUS_NOT_GENERATED;
    }

    // Reset current global and per-axis activity flags to running values
    step_generator_state.current_flags = (StepEventFlag_t(Stepper::last_direction_bits) << STEP_EVENT_FLAG_DIR_SHIFT) & STEP_EVENT_FLAG_DIR_MASK;
    for (uint8_t i = 0; i != PS_AXIS_COUNT; ++i) {
        StepEventFlag_t mask = ((STEP_EVENT_FLAG_X_DIR | STEP_EVENT_FLAG_X_ACTIVE) << i);
        PreciseStepping::step_generator_state.step_generator[i]->step_flags = step_generator_state.current_flags & mask;
    }

    LOOP_XYZ(i) {
#ifdef ADVANCED_STEP_GENERATORS
        if (physical_axis_step_generator_types & (INPUT_SHAPER_STEP_GENERATOR_X << i)) {
            if (physical_axis_step_generator_types & (PHASE_STEPPING_GENERATOR_X << i)) {
                phase_stepping::init_step_generator_input_shaping(move, step_generators_pool.input_shaper_step_generator[i], step_generator_state);
            } else {
                input_shaper_step_generator_init(move, step_generators_pool.input_shaper_step_generator[i], step_generator_state);
            }
        } else {
            if (physical_axis_step_generator_types & (PHASE_STEPPING_GENERATOR_X << i)) {
                phase_stepping::init_step_generator_classic(move, step_generators_pool.classic_step_generator[i], step_generator_state);
            } else {
                classic_step_generator_init(move, step_generators_pool.classic_step_generator[i], step_generator_state);
            }
        }
#else
        if (physical_axis_step_generator_types & (PHASE_STEPPING_GENERATOR_X << i)) {
            phase_stepping::init_step_generator_classic(move, step_generators_pool.classic_step_generator[i], step_generator_state);
        } else {
            classic_step_generator_init(move, step_generators_pool.classic_step_generator[i], step_generator_state);
        }
#endif
    }

    // E-axis
#ifdef ADVANCED_STEP_GENERATORS
    if (physical_axis_step_generator_types & PRESSURE_ADVANCE_STEP_GENERATOR_E) {
        pressure_advance_step_generator_init(move, step_generators_pool.pressure_advance_step_generator_e, step_generator_state);
    } else {
        classic_step_generator_init(move, step_generators_pool.classic_step_generator[E_AXIS], step_generator_state);
    }
#else
    classic_step_generator_init(move, step_generators_pool.classic_step_generator[E_AXIS], step_generator_state);
#endif

    step_generator_state.initialized = true;
}

void PreciseStepping::move_segment_processed_handler() {
    if (const move_t *move = get_current_unprocessed_move_segment(); move != nullptr && move->reference_cnt == 0) {
        discard_current_unprocessed_move_segment();
        ++step_generator_state.left_insert_start_of_move_segment;
    }
}

void PreciseStepping::reset_queues() {
#if HAS_PHASE_STEPPING()
    phase_stepping::clear_targets();
    if (phase_stepping::processing()) {
        // wait until the last spi/burst transaction is complete before allowing reset to continue
        return;
    }
#endif

    const bool was_enabled = stepper.suspend();
    DISABLE_MOVE_INTERRUPT();

    // reset internal state and queues
    step_event_queue_clear();
    move_segment_queue_clear();
    reset_from_halt();

    // at this point the planner might still have queued extra moves, flush them
    planner.clear_block_buffer();

    step_dl_miss = 0;
    step_ev_miss = 0;
    left_ticks_to_next_step_event = 0;
    Stepper::axis_did_move = 0;
    stop_pending = false;
    busy = false;

    ENABLE_MOVE_INTERRUPT();
    if (was_enabled) {
        stepper.wake_up();
    }
}

void mark_ownership(move_t &move) {
    move.reference_cnt++;
}

void discard_ownership(move_t &move) {
    move.reference_cnt--;
    PreciseStepping::move_segment_processed_handler();
}
