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

#include "../../module/planner.h"
#include "../../module/stepper.h"
#include "../../module/endstops.h"

#include <timing_precise.hpp>

#if defined(ISR_DEADLINE_DEBUGGING) || defined(ISR_EVENT_DEBUGGING)
    #include <sound.hpp>
#endif

#if BOARD_IS_DWARF
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
    #define X_STEP_SET() buddy::hw::xStep.toggle();
    #define Y_STEP_SET() buddy::hw::yStep.toggle();
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

uint32_t PreciseStepping::left_ticks_to_next_step_event = 0;

uint32_t PreciseStepping::stepper_isr_period_in_ticks;
float PreciseStepping::ticks_per_sec;

step_generator_state_t PreciseStepping::step_generator_state;
step_generators_pool_t PreciseStepping::step_generators_pool;

uint8_t PreciseStepping::step_generator_types = CLASSIC_STEP_GENERATOR_X | CLASSIC_STEP_GENERATOR_Y | CLASSIC_STEP_GENERATOR_Z | CLASSIC_STEP_GENERATOR_E;

uint16_t PreciseStepping::inverted_dirs = 0;
double PreciseStepping::global_print_time = 0;
xyze_double_t PreciseStepping::global_start_pos = { 0., 0., 0., 0. };

uint32_t PreciseStepping::waiting_before_delivering_start_time = 0;

std::atomic<bool> PreciseStepping::stop_pending = false;
volatile uint8_t PreciseStepping::step_dl_miss = 0;
volatile uint8_t PreciseStepping::step_ev_miss = 0;

FORCE_INLINE xyze_double_t calc_distance_block(const block_t *block) {
    const xyze_double_t direction = {
        (block->direction_bits & _BV(X_AXIS)) ? -1. : 1.,
        (block->direction_bits & _BV(Y_AXIS)) ? -1. : 1.,
        (block->direction_bits & _BV(Z_AXIS)) ? -1. : 1.,
        (block->direction_bits & _BV(E_AXIS)) ? -1. : 1.
    };
    const xyze_double_t distance = {
        (double)block->steps.x * (double)Planner::mm_per_step[X_AXIS],
        (double)block->steps.y * (double)Planner::mm_per_step[Y_AXIS],
        (double)block->steps.z * (double)Planner::mm_per_step[Z_AXIS],
        (double)block->steps.e * (double)Planner::mm_per_step[E_AXIS]
    };

    return distance * direction;
}

FORCE_INLINE MoveFlag_t get_active_axis_flags_from_block(const block_t *block) {
    MoveFlag_t flags = (block->steps.x > 0 ? MOVE_FLAG_X_ACTIVE : 0)
        | (block->steps.y > 0 ? MOVE_FLAG_Y_ACTIVE : 0)
        | (block->steps.z > 0 ? MOVE_FLAG_Z_ACTIVE : 0)
        | (block->steps.e > 0 ? MOVE_FLAG_E_ACTIVE : 0);
    return flags;
}

FORCE_INLINE bool append_move_segment_to_queue(const double move_time, const double start_v, const double half_accel, const double print_time,
    const xyze_double_t axes_r, const xyze_double_t start_pos, const MoveFlag_t flags) {
    uint8_t next_move_segment_queue_head;
    if (move_t *m = PreciseStepping::get_next_free_move_segment(next_move_segment_queue_head); m != nullptr) {
        m->move_t = move_time;
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

FORCE_INLINE xyze_double_t calc_axes_r_from_block(const block_t *block, const double steps_per_mm) {
    const double step_event_count_inv = 1. / (double)block->step_event_count;
    xyze_double_t axes_r;

    LOOP_XYZE(i) {
        if (!block->steps[i]) {
            axes_r[i] = 0;
        } else {
            double r = (double)block->steps[i] * step_event_count_inv;
            r *= (block->direction_bits & _BV(i)) ? -1. : 1.;
            r *= steps_per_mm * ((double)Planner::mm_per_step[i]);
            axes_r[i] = r;
        }
    }

    return axes_r;
}

bool append_move_segments_to_queue(const block_t *block) {
    double print_time = PreciseStepping::global_print_time;
    xyze_double_t start_pos = PreciseStepping::global_start_pos;

    double accel_t = 0.;
    double decel_t = 0.;
    double cruise_t = 0.;
    double speed_after_accel = block->initial_rate;
    uint8_t move_blocks_required = 0;

    if (block->accelerate_until > 0) {
        accel_t = calc_time_for_distance_block(block->initial_rate, block->acceleration_steps_per_s2, block->accelerate_until);
        speed_after_accel = block->initial_rate + block->acceleration_steps_per_s2 * accel_t;
        ++move_blocks_required;
    }

    if (block->decelerate_after < block->step_event_count) {
        decel_t = calc_time_for_distance_block(speed_after_accel, -((int32_t)block->acceleration_steps_per_s2), block->step_event_count - block->decelerate_after);
        ++move_blocks_required;
    }

    if ((block->decelerate_after - block->accelerate_until) > 0) {
        cruise_t = calc_time_for_distance_block(speed_after_accel, 0, block->decelerate_after - block->accelerate_until);
        ++move_blocks_required;
    }

    if (PreciseStepping::move_segment_queue_free_slots() < (move_blocks_required + MOVE_SEGMENT_QUEUE_MIN_FREE_SLOTS))
        return false;

    const MoveFlag_t active_axis = get_active_axis_flags_from_block(block);
    const double steps_per_mm = ((double)block->step_event_count) / ((double)block->millimeters);
    const xyze_double_t axes_r = calc_axes_r_from_block(block, steps_per_mm);
    const double half_accel = .5 * (block->acceleration_steps_per_s2 / steps_per_mm);
    const double cruise_v = speed_after_accel / steps_per_mm;

    if (accel_t != 0.) {
        const double start_v = block->initial_rate / steps_per_mm;
        MoveFlag_t flags = MOVE_FLAG_ACCELERATION_PHASE
            | MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK
            | ((cruise_t != 0. || decel_t != 0.) ? 0x00 : MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK)
            | (uint16_t(block->direction_bits & 0x0F) << MOVE_FLAG_DIR_SHIFT)
            | active_axis;
        append_move_segment_to_queue(accel_t, start_v, half_accel, print_time, axes_r, start_pos, flags);
        print_time += accel_t;
        start_pos = calc_position(start_v, half_accel, accel_t, start_pos, axes_r);
    }

    if (cruise_t != 0.) {
        MoveFlag_t flags = MOVE_FLAG_CRUISE_PHASE
            | ((accel_t != 0.) ? 0x00 : MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK)
            | ((decel_t != 0.) ? 0x00 : MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK)
            | (uint16_t(block->direction_bits & 0x0F) << MOVE_FLAG_DIR_SHIFT)
            | active_axis;
        append_move_segment_to_queue(cruise_t, cruise_v, 0., print_time, axes_r, start_pos, flags);
        print_time += cruise_t;
        start_pos = calc_position(cruise_v, 0., cruise_t, start_pos, axes_r);
    }

    if (decel_t != 0.) {
        MoveFlag_t flags = MOVE_FLAG_DECELERATION_PHASE
            | MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK
            | ((accel_t != 0. || cruise_t != 0.) ? 0x00 : MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK)
            | (uint16_t(block->direction_bits & 0x0F) << MOVE_FLAG_DIR_SHIFT)
            | active_axis;
        append_move_segment_to_queue(decel_t, cruise_v, -half_accel, print_time, axes_r, start_pos, flags);
        print_time += decel_t;
    }

    PreciseStepping::global_start_pos += calc_distance_block(block);
    PreciseStepping::global_print_time = print_time;

    // We are appended to the queue, so we should reset indicators that all step generators reach the end of the queue.
    reset_reached_end_of_move_queue_flag(PreciseStepping::step_generator_state);
    return true;
}

FORCE_INLINE float calc_time_for_distance(const classic_step_generator_t &step_generator, const float distance) {
    return calc_time_for_distance(step_generator.start_v, step_generator.accel, distance, step_generator.step_dir);
}

FORCE_INLINE void classic_step_generator_update(classic_step_generator_t &step_generator) {
    const move_t &current_move = *step_generator.current_move;

    // Special case
    if (const float axis_r = float(current_move.axes_r[step_generator.axis]); axis_r == 0.f) {
        step_generator.start_v = 0.f;
        step_generator.accel = 0.f;
    } else {
        step_generator.start_v = float(current_move.start_v) * axis_r;
        step_generator.accel = 2.f * float(current_move.half_accel) * axis_r;
    }

    step_generator.start_pos = float(current_move.start_pos[step_generator.axis]);
    step_generator.step_dir = get_move_step_dir(*step_generator.current_move, step_generator.axis);
}

step_event_info_t classic_step_generator_next_step_event(classic_step_generator_t &step_generator, step_generator_state_t &step_generator_state, const double flush_time) {
    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0 };
    const move_t *next_move = nullptr;
    do {
        const bool step_dir = get_move_step_dir(*step_generator.current_move, step_generator.axis);
        const float half_step_dist = Planner::mm_per_half_step[step_generator.axis];
        const float current_distance = float(step_generator_state.current_distance[step_generator.axis]) * Planner::mm_per_step[step_generator.axis];
        const float next_target = current_distance + (step_dir ? half_step_dist : -half_step_dist);
        const float next_distance = next_target - step_generator.start_pos;
        const float step_time = calc_time_for_distance(step_generator, next_distance);

        // When step_time is NaN, it means that next_distance will never be reached.
        // This happens when next_target exceeds end_position, and deceleration decelerates velocity to zero or negative value.
        // Also, we need to stop when step_time exceeds local_end.
        // Be aware that testing, if flush_time was exceeded, has to be after testing for exceeding print_time.
        const double elapsed_time = double(step_time) + step_generator.current_move->print_time;
        if (isnan(step_time) || elapsed_time > (step_generator.current_move->move_t + step_generator.current_move->print_time + EPSILON)) {
            if (next_move = PreciseStepping::move_segment_queue_next_move(*step_generator.current_move); next_move != nullptr) {
                // The move segment is fully processed, and in the queue is another unprocessed move segment.
                // So we decrement reference count of the current move segment and increment reference count of next move segment.
                --step_generator.current_move->reference_cnt;
                step_generator.current_move = next_move;
                ++step_generator.current_move->reference_cnt;

                // Update step direction flag, which is cached until this move segment is processed.
                // It assumes that dir bit flags for step_event_t and move_t are the same position.
                const StepEventFlag_t current_axis_dir_flag = (STEP_EVENT_FLAG_X_DIR << step_generator.axis);
                step_generator_state.flags &= ~current_axis_dir_flag;
                step_generator_state.flags |= step_generator.current_move->flags & current_axis_dir_flag;

                // Update active axis flag, which is cached until this move segment is processed.
                // It assumes that active bit flags for step_event_t and move_t are the same position.
                const StepEventFlag_t current_axis_active_flag = (STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis);
                step_generator_state.flags &= ~current_axis_active_flag;
                step_generator_state.flags |= step_generator.current_move->flags & current_axis_active_flag;

                classic_step_generator_update(step_generator);
                PreciseStepping::move_segment_processed_handler();
            } else
                step_generator.reached_end_of_move_queue = true;
        } else if (elapsed_time > flush_time) {
            step_generator.reached_end_of_move_queue = true;
            break;
        } else {
            next_step_event.time = elapsed_time;
            next_step_event.flags = STEP_EVENT_FLAG_STEP_X << step_generator.axis;
            next_step_event.flags |= step_generator_state.flags;
            step_generator_state.current_distance[step_generator.axis] += (step_dir ? 1 : -1);
            break;
        }
    } while (next_move != nullptr);

    // When std::numeric_limits<double>::max() is returned, it means that for the current state of the move segment queue, there isn't any next step event for this axis.
    return next_step_event;
}

void classic_step_generator_init(const move_t &move, classic_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    const uint8_t axis = step_generator.axis;
    step_generator.current_move = &move;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)classic_step_generator_next_step_event;

    step_generator_state.flags |= move.flags & (STEP_EVENT_FLAG_X_DIR << axis);
    step_generator_state.flags |= move.flags & (STEP_EVENT_FLAG_X_ACTIVE << axis);
    move.reference_cnt += 1;

    classic_step_generator_update(step_generator);
}

FORCE_INLINE step_event_info_t step_generator_next_step_event(step_generator_state_t &step_generator_state, const uint8_t axis, const double flush_time) {
    return (*step_generator_state.next_step_func[axis])(
        static_cast<move_segment_step_generator_t &>(*step_generator_state.step_generator[axis]),
        step_generator_state, flush_time);
}

// Return 1 when move is fully process and there is no other work for this move segment.
// Return 2 when step event queue is full or when step_state->initialized was false before writing new step event time to data structure.
// Return 0 otherwise.
StepGeneratorStatus generate_next_step_event(step_generator_state_t &step_state, const double flush_time) {
    const size_t old_nearest_step_event_idx = step_state.nearest_step_event_idx;
    const double old_nearest_step_event = step_state.step_events[old_nearest_step_event_idx].time;

    // Sorting buffer isn't fulfilled for all active axis, so we need to fulfill.
    // So we don't have anything to put into step_event_buffer.
    if (old_nearest_step_event != 0 && old_nearest_step_event != std::numeric_limits<double>::max()) {
        const double step_time_absolute = old_nearest_step_event;
        double step_time_relative = step_time_absolute - step_state.previous_step_time;

        // TODO: This is just hotfix to postpone processing of the first step event.
        if (step_state.previous_step_time == 0)
            step_time_relative = 0.025;

        uint16_t next_step_event_queue_head = 0;
        if (step_event_t *step_event = PreciseStepping::get_next_free_step_event(next_step_event_queue_head); step_event != nullptr) {
            if (step_time_relative < 0.) {
                // FIXME Lukas H.: Now, because of the numeric issue after switching from doubles to floats. This condition is triggered with a very small negative value.
                //                 For now should be enough to ignore it, but later it should be investigated more deeply.
#ifdef FAIL_ON_NEGATIVE_STEP_TIME
                fatal_error("Negative step time.", "generate_next_step_event");
#endif
                step_time_relative = 0;
            }

            step_event->time_ticks = int32_t(step_time_relative * PreciseStepping::ticks_per_sec);
            step_event->flags = step_state.step_events[old_nearest_step_event_idx].flags;

            if (step_state.left_insert_start_of_move_segment) {
                step_event->flags |= STEP_EVENT_FLAG_BEGINNING_OF_MOVE_SEGMENT;
                --step_state.left_insert_start_of_move_segment;
            }

            PreciseStepping::step_event_queue.head = next_step_event_queue_head;
            step_state.previous_step_time = step_time_absolute;
        } else {
            // There no free space in step event queue or step_state was reset (endstop),
            // so we skip computing next step event, and also we didn't change anything.
            return STEP_GENERATOR_STATUS_FULL_STEP_EVENT_QUEUE;
        }
    }

    // Now we have to compute next step_event instead of the one that we putted into step event queue.
    const step_event_info_t new_nearest_step_event = step_generator_next_step_event(step_state, int(old_nearest_step_event_idx), flush_time);
    step_state.step_events[old_nearest_step_event_idx] = new_nearest_step_event;

    // Update nearest step event index.
    step_generator_state_update_nearest_idx(step_state, new_nearest_step_event.time);
    return StepGeneratorStatus(step_state.step_events[step_state.nearest_step_event_idx].time == std::numeric_limits<double>::max());
}

HAL_MOVE_TIMER_ISR() {
    HAL_timer_isr_prologue(MOVE_TIMER_NUM);
    PreciseStepping::process_queue_of_move_segments();
    HAL_timer_isr_epilogue(MOVE_TIMER_NUM);
}

HAL_STEP_TIMER_ISR() {
    if (__HAL_TIM_GET_FLAG(&TimerHandle[STEP_TIMER_NUM].handle, TIM_FLAG_CC1) != RESET) {
        __HAL_TIM_CLEAR_IT(&TimerHandle[STEP_TIMER_NUM].handle, TIM_IT_CC1);
        PreciseStepping::isr();

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        // ensure FPU wasn't accidentally used in this ISR for performance reasons
        assert(!(__get_CONTROL() & 0b100) || (FPU->FPCCR & FPU_FPCCR_LSPACT_Msk));
#endif
    }
}

void PreciseStepping::init() {
    // If no queued step event, just wait 1ms for the next move
    stepper_isr_period_in_ticks = (STEPPER_TIMER_RATE / 1000);
    ticks_per_sec = float(STEPPER_TIMER_RATE);

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

#ifdef ADVANCED_STEP_GENERATORS
    PreciseStepping::step_generators_pool.pressure_advance_step_generator_e.pa_state = &PressureAdvance::pressure_advance_state;

    PreciseStepping::step_generators_pool.input_shaper_step_generator_x.is_state = &InputShaper::is_state_x;
    PreciseStepping::step_generators_pool.input_shaper_step_generator_x.is_pulses = &InputShaper::is_pulses_x;

    PreciseStepping::step_generators_pool.input_shaper_step_generator_y.is_state = &InputShaper::is_state_y;
    PreciseStepping::step_generators_pool.input_shaper_step_generator_y.is_pulses = &InputShaper::is_pulses_y;

    PreciseStepping::step_generators_pool.input_shaper_step_generator_z.is_state = &InputShaper::is_state_z;
    PreciseStepping::step_generators_pool.input_shaper_step_generator_z.is_pulses = &InputShaper::is_pulses_z;
#endif

    PreciseStepping::move_segment_queue_clear();
    PreciseStepping::step_event_queue_clear();
    PreciseStepping::reset_from_halt();

    HAL_timer_start(MOVE_TIMER_NUM, MOVE_TIMER_FREQUENCY);
    ENABLE_MOVE_INTERRUPT();
}

void PreciseStepping::reset_from_halt() {
    PreciseStepping::step_generator_state_clear();
    PreciseStepping::global_print_time = 0.;
    PreciseStepping::global_start_pos = { 0., 0., 0., 0. };
}

uint32_t PreciseStepping::process_one_step_event_from_queue() {
    // If no queued step event, just wait some time for the next move.
    uint32_t ticks_to_next_isr = stepper_isr_period_in_ticks;

    if (const step_event_t *step_event = get_current_step_event(); step_event != nullptr) {
        const StepEventFlag_t step_flags = step_event->flags;
        const StepEventFlag_t step_dir = (step_flags & STEP_EVENT_FLAG_DIR_MASK);
        const StepEventFlag_t step_dir_inv = (step_dir ^ PreciseStepping::inverted_dirs);
        const StepEventFlag_t axis_move = (step_flags & STEP_EVENT_FLAG_AXIS_ACTIVE_MASK);

        if (step_flags & STEP_EVENT_FLAG_BEGINNING_OF_MOVE_SEGMENT) {
            // a new move is about to start (or a discarding event has been requested): discard the previous one
            if (const move_t *current_move = get_current_move_segment(); current_move->flags & MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK) {
                // discard the current block if this move is also the last move segment of a block
                block_t *current_block = Planner::get_current_processed_block();
                if (current_block->flag.sync_position)
                    Stepper::_set_position(current_block->position);
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

            if (TEST(changed_dir_bits, X_AXIS)) {
                X_APPLY_DIR(step_dir_inv & STEP_EVENT_FLAG_X_DIR);
                Stepper::count_direction.x = (step_dir & STEP_EVENT_FLAG_X_DIR) ? -1 : 1;
            }

            if (TEST(changed_dir_bits, Y_AXIS)) {
                Y_APPLY_DIR(step_dir_inv & STEP_EVENT_FLAG_Y_DIR);
                Stepper::count_direction.y = (step_dir & STEP_EVENT_FLAG_Y_DIR) ? -1 : 1;
            }

            if (TEST(changed_dir_bits, Z_AXIS)) {
                Z_APPLY_DIR(step_dir_inv & STEP_EVENT_FLAG_Z_DIR);
                Stepper::count_direction.z = (step_dir & STEP_EVENT_FLAG_Z_DIR) ? -1 : 1;
            }

            if (TEST(changed_dir_bits, E_AXIS)) {
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

        if (step_event_t *next_step_event = get_current_step_event(); next_step_event != nullptr)
            ticks_to_next_isr = next_step_event->time_ticks;
        else if ((step_flags & STEP_EVENT_END_OF_MOTION) == false)
            ++step_ev_miss;
    } else {
        // The step event queue drained or ended
        Stepper::axis_did_move = 0;
    }

    return ticks_to_next_isr;
}

void PreciseStepping::isr() {
#ifndef ISR_DEADLINE_TRACKING
    constexpr uint32_t min_delay = 6; // fuse isr for steps below this threshold (us)
#else
    constexpr uint32_t min_delay = 11; // fuse isr for steps below this threshold (us)
#endif
    constexpr uint32_t min_reserve = 5;              // minimum interval for isr re-entry (us)
    constexpr uint32_t max_ticks = (UINT16_MAX / 2); // maximum isr interval for skip detection (us)
    constexpr uint8_t max_steps = 4;                 // maximum number of steps to per isr to limit latency

#ifdef ISR_DEADLINE_TRACKING
    // in addition to checking for forward misses, check for past ones
    static uint32_t scheduled_ts = 0;
    if (scheduled_ts && ticks_us() > scheduled_ts + min_reserve * 2)
        ++step_dl_miss;
#endif

    uint32_t time_increment = 0;
    for (uint8_t steps = 0; steps != max_steps;) {
        if (stop_pending)
            [[unlikely]] {
            time_increment = stepper_isr_period_in_ticks;
            Stepper::axis_did_move = 0;
            break;
        }

        if (!left_ticks_to_next_step_event) {
            left_ticks_to_next_step_event = process_one_step_event_from_queue();
            ++steps;
        }

        // limit the interval to avoid a counter overflow or runout
        uint32_t ticks_to_next_step_event = left_ticks_to_next_step_event;
        NOMORE(ticks_to_next_step_event, max_ticks);

        // Compute the time remaining until the next step event.
        left_ticks_to_next_step_event -= ticks_to_next_step_event;

        // Compute the number of ticks for the next ISR.
        time_increment += ticks_to_next_step_event;
        if (ticks_to_next_step_event > min_delay || steps >= max_steps)
            break;

        // the next step is too close for a new isr but still within margin,
        // spin-wait for accurate delivery
        if (left_ticks_to_next_step_event)
            delay_us_precise(left_ticks_to_next_step_event);
    }

    uint32_t compare = __HAL_TIM_GET_COMPARE(&TimerHandle[STEP_TIMER_NUM].handle, TIM_CHANNEL_1);
    uint32_t next = compare + time_increment;
    uint32_t counter = __HAL_TIM_GET_COUNTER(&TimerHandle[STEP_TIMER_NUM].handle);
    uint32_t deadline = counter + min_reserve;
    if (((next - deadline) & 0xFFFF) > max_ticks)
        [[unlikely]] {
        // next isr too close or missed: reschedule
        next = __HAL_TIM_GET_COUNTER(&TimerHandle[STEP_TIMER_NUM].handle) + min_reserve;
        ++step_dl_miss;
    }
    __HAL_TIM_SET_COMPARE(&TimerHandle[STEP_TIMER_NUM].handle, TIM_CHANNEL_1, next);

#ifdef ISR_DEADLINE_TRACKING
    uint32_t scheduled_ticks = (((next & 0xFFFF) - __HAL_TIM_GET_COUNTER(&TimerHandle[STEP_TIMER_NUM].handle)) & 0xFFFF);
    scheduled_ts = ticks_us() + scheduled_ticks;
#endif
}

FORCE_INLINE move_t *append_beginning_empty_move() {
    uint8_t next_move_segment_queue_head = 0;
    move_t *move = PreciseStepping::get_next_free_move_segment(next_move_segment_queue_head);
    if (move != nullptr) {
        move->flags = MOVE_FLAG_BEGINNING_EMPTY_MOVE;
        move->start_v = 0.;
        move->half_accel = 0.;
        move->axes_r = { 0., 0., 0., 0. };
        move->move_t = 0.1; // The value is chosen to be sufficient for 3hump_ei with 15Hz.
        move->start_pos = PreciseStepping::global_start_pos;
        move->print_time = 0.;
        move->reference_cnt = 0;
        PreciseStepping::move_segment_queue.head = next_move_segment_queue_head;
        PreciseStepping::global_print_time = move->print_time + move->move_t;

        // We are appended to the queue, so we should reset indicators that all step generators reach the end of the queue.
        reset_reached_end_of_move_queue_flag(PreciseStepping::step_generator_state);
    }

    return move;
}

FORCE_INLINE move_t *append_block_discarding_move() {
    uint8_t next_move_segment_queue_head = 0;
    move_t *move = PreciseStepping::get_next_free_move_segment(next_move_segment_queue_head);
    if (move != nullptr) {
        move->flags = MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK | MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK;
        move->start_v = 0.;
        move->half_accel = 0.;
        move->axes_r = { 0., 0., 0., 0. };
        move->move_t = 0.;
        move->start_pos = PreciseStepping::global_start_pos;
        move->print_time = PreciseStepping::global_print_time;
        move->reference_cnt = 0;
        PreciseStepping::move_segment_queue.head = next_move_segment_queue_head;
    }

    return move;
}

FORCE_INLINE move_t *append_ending_empty_move() {
    uint8_t next_move_segment_queue_head = 0;
    move_t *move = PreciseStepping::get_next_free_move_segment(next_move_segment_queue_head);
    if (move != nullptr) {
        move->flags = MOVE_FLAG_ENDING_EMPTY_MOVE;
        move->start_v = 0.;
        move->half_accel = 0.;
        move->axes_r = { 0., 0., 0., 0. };
        move->move_t = MAX_PRINT_TIME;
        move->start_pos = PreciseStepping::global_start_pos;
        move->print_time = PreciseStepping::global_print_time;
        move->reference_cnt = 0;
        PreciseStepping::move_segment_queue.head = next_move_segment_queue_head;
        PreciseStepping::global_print_time = move->print_time + move->move_t;

        // We are appended to the queue, so we should reset indicators that all step generators reach the end of the queue.
        reset_reached_end_of_move_queue_flag(PreciseStepping::step_generator_state);
    }

    return move;
}

FORCE_INLINE bool append_move_discarding_step_event(step_generator_state_t &step_state, StepEventFlag_t extra_step_flags = 0) {
    uint16_t next_step_event_queue_head = 0;
    if (step_event_t *step_event = PreciseStepping::get_next_free_step_event(next_step_event_queue_head); step_event != nullptr) {
        step_event->flags = step_state.flags | STEP_EVENT_FLAG_BEGINNING_OF_MOVE_SEGMENT | extra_step_flags;

        PreciseStepping::step_event_queue.head = next_step_event_queue_head;
        step_state.previous_step_time = 0;
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
        } else
            return true;
    }

    if (const uint8_t waiting_for_discard = Planner::movesplanned_processed(); waiting_for_discard >= (BLOCK_BUFFER_SIZE / 2)) {
        // In case the block queue contains plenty of short blocks waiting for discarding and step generators are unable to produce new
        // step events, we have to ensure that the next block can be processed (or the empty move segment can be placed into the queue).
        if (has_all_generators_reached_end_of_move_queue(PreciseStepping::step_generator_state)) {
            // We reset indicators that all step generators reach the end of the queue to ensure that this
            // condition will not be triggered multiple times before the move interrupt handler is called.
            reset_reached_end_of_move_queue_flag(PreciseStepping::step_generator_state);
            return false;
        }

        return true;
    }

    return false;
}

void PreciseStepping::process_queue_of_blocks() {
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

    if (is_waiting_before_delivering())
        return;

    // When the ending move segment is on the bottom of the queue (then Planner::global_print_time
    // contains the value bigger then MAX_PRINT_TIME) we're waiting for motion to halt and reset.
    if (PreciseStepping::global_print_time >= MAX_PRINT_TIME) {
        // ensure all motion has stopped
        if (has_blocks_queued())
            return;

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

        // sync blocks should only be queued if motion was already started;
        // the counters should be manipulated directly otherwise
        assert(PreciseStepping::global_print_time != 0.);

        if (!append_block_discarding_move())
            return;

        // pass-through SYNC blocks, they will be processed in the ISR
        Planner::discard_current_unprocessed_block();
    }

    if (current_block == nullptr) {
        if (PreciseStepping::global_print_time && has_all_generators_reached_end_of_move_queue(PreciseStepping::step_generator_state)) {
            // motion was already started and the move queue is about to (or ran) dry: enqueue an end block
            if (PreciseStepping::global_print_time) {
                append_ending_empty_move();
                return;
            }
        }
        return;
    }

    if (PreciseStepping::global_print_time == 0.) {
        // we're restarting from zero, prepend a beginning move
        if (!append_beginning_empty_move())
            return;
    }

    if (append_move_segments_to_queue(current_block))
        Planner::discard_current_unprocessed_block();
}

void PreciseStepping::process_queue_of_move_segments() {
    if (stop_pending)
        return;

    uint16_t produced_step_events_cnt = 0;
    if (const move_t *move = get_current_unprocessed_move_segment(); move != nullptr && step_event_queue_free_slots() > MIN_STEP_EVENT_FREE_SLOT) {
        if (!step_generator_state.initialized)
            step_generator_state_init(*move);

        // Used for ensuring that none of the step event generators will produce step event beyond
        // the flush time. Because for the same state of the move segment queue, some step event
        // generator could generate step events far away from others, which could let to incorrect
        // ordering of step events.
        const double flush_time = global_print_time - step_generator_state.max_lookback_time;

        step_generator_state_restart(step_generator_state);
        // Next move segment could have different active axes than the previous move segment, so we need to update the index of the axis of the nearest step event.
        step_generator_state_update_nearest_idx(step_generator_state, std::numeric_limits<double>::max());

        StepGeneratorStatus status;
        while ((status = generate_next_step_event(step_generator_state, flush_time)) == STEP_GENERATOR_STATUS_OK && produced_step_events_cnt < MAX_STEP_EVENTS_PRODUCED_PER_ONE_CALL)
            ++produced_step_events_cnt;

        if (const move_t *unprocessed_move = get_current_unprocessed_move_segment(); unprocessed_move != nullptr && is_ending_empty_move(*unprocessed_move)) {
            // Place discarding step events (just empty step events) into the step event queue if there are
            // still remaining step events that have to be marked to free already processed move segments.
            while (step_generator_state.left_insert_start_of_move_segment > 0 && append_move_discarding_step_event(step_generator_state))
                --step_generator_state.left_insert_start_of_move_segment;

            assert(step_generator_state.left_insert_start_of_move_segment >= 0);

            // The step event queue can be full, and all left_insert_start_of_move_segment couldn't be all processed.
            // So discard this move only when left_insert_start_of_move_segment is fully processed.
            if (step_generator_state.left_insert_start_of_move_segment == 0) {
                // we reached an explicit end block with all generators waiting on it, we can advance
                if (!is_step_event_queue_full()) {
                    discard_current_unprocessed_move_segment();
                    [[maybe_unused]] const bool appended = append_move_discarding_step_event(step_generator_state, STEP_EVENT_END_OF_MOTION);
                    assert(appended);
                }

                assert(step_generator_state.left_insert_start_of_move_segment == 0);
            }
        }
    }
}

static double calc_maximum_lookback_time() {
    double max_lookback_time = 0.;

#ifdef ADVANCED_STEP_GENERATORS
    if (PreciseStepping::step_generator_types & INPUT_SHAPER_STEP_GENERATOR_X)
        max_lookback_time = std::max(max_lookback_time, -InputShaper::is_pulses_x.pulses[0].t);

    if (PreciseStepping::step_generator_types & INPUT_SHAPER_STEP_GENERATOR_Y)
        max_lookback_time = std::max(max_lookback_time, -InputShaper::is_pulses_y.pulses[0].t);

    if (PreciseStepping::step_generator_types & INPUT_SHAPER_STEP_GENERATOR_Z)
        max_lookback_time = std::max(max_lookback_time, -InputShaper::is_pulses_z.pulses[0].t);

    if (PreciseStepping::step_generator_types & PRESSURE_ADVANCE_STEP_GENERATOR_E) {
        const pressure_advance_params_t &pa_params = PressureAdvance::pressure_advance_params;
        max_lookback_time = std::max(max_lookback_time, pa_params.sampling_rate * (double)((pa_params.filter.length + 1) / 2));
    }
#endif

    return max_lookback_time;
}

void PreciseStepping::step_generator_state_init(const move_t &move) {
    assert(is_beginning_empty_move(move));

    step_generator_state.flags = 0;
    step_generator_state.previous_step_time = 0.;
    step_generator_state.nearest_step_event_idx = 0;
    step_generator_state.left_insert_start_of_move_segment = 0;
    step_generator_state.max_lookback_time = calc_maximum_lookback_time();

    for (step_event_info_t &step_event_info : step_generator_state.step_events) {
        step_event_info.time = 0.;
        step_event_info.flags = 0;
    }

    for (basic_step_generator_t *step_generator : step_generator_state.step_generator) {
        step_generator->reached_end_of_move_queue = false;
    }

    // X-axis
#ifdef ADVANCED_STEP_GENERATORS
    if (step_generator_types & INPUT_SHAPER_STEP_GENERATOR_X) {
        input_shaper_step_generator_init(move, step_generators_pool.input_shaper_step_generator_x, step_generator_state);
    } else {
        classic_step_generator_init(move, step_generators_pool.classic_step_generator_x, step_generator_state);
    }
#else
    classic_step_generator_init(move, step_generators_pool.classic_step_generator_x, step_generator_state);
#endif

    // Y-axis
#ifdef ADVANCED_STEP_GENERATORS
    if (step_generator_types & INPUT_SHAPER_STEP_GENERATOR_Y) {
        input_shaper_step_generator_init(move, step_generators_pool.input_shaper_step_generator_y, step_generator_state);
    } else {
        classic_step_generator_init(move, step_generators_pool.classic_step_generator_y, step_generator_state);
    }
#else
    classic_step_generator_init(move, step_generators_pool.classic_step_generator_y, step_generator_state);
#endif

    // Z-axis
#ifdef ADVANCED_STEP_GENERATORS
    if (step_generator_types & INPUT_SHAPER_STEP_GENERATOR_Z) {
        input_shaper_step_generator_init(move, step_generators_pool.input_shaper_step_generator_z, step_generator_state);
    } else {
        classic_step_generator_init(move, step_generators_pool.classic_step_generator_z, step_generator_state);
    }
#else
    classic_step_generator_init(move, step_generators_pool.classic_step_generator_z, step_generator_state);
#endif

    // E-axis
#ifdef ADVANCED_STEP_GENERATORS
    if (step_generator_types & PRESSURE_ADVANCE_STEP_GENERATOR_E) {
        pressure_advance_step_generator_init(move, step_generators_pool.pressure_advance_step_generator_e, step_generator_state);
    } else {
        classic_step_generator_init(move, step_generators_pool.classic_step_generator_e, step_generator_state);
    }
#else
    classic_step_generator_init(move, step_generators_pool.classic_step_generator_e, step_generator_state);
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

    ENABLE_MOVE_INTERRUPT();
    if (was_enabled)
        stepper.wake_up();
}
