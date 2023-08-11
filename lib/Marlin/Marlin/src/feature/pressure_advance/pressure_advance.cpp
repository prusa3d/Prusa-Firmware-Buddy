/**
 * Based on the implementation of the pressure advance in Klipper [https://github.com/Klipper3d/klipper].
 * Copyright (C) Kevin O'Connor <kevin@koconnor.net>
 *
 * Our implementation takes inspiration from the work of Kevin O'Connor <kevin@koconnor.net> for Klipper
 * in used data structures, and some computations.
 *
 * We implement the pressure advance using FIR filter that is less computationally demanding
 * and can be fully run on a single 32-bit MCU with nearly identical results as the implementation in Klipper.
 */
#include "pressure_advance.hpp"

#include "../precise_stepping/precise_stepping.hpp"
#include "../precise_stepping/internal.hpp"

#include <cmath>
#include "core/macros.h"
#include "module/planner.h"

pressure_advance_params_t PressureAdvance::pressure_advance_params;
pressure_advance_state_t PressureAdvance::pressure_advance_state;

FORCE_INLINE double calc_distance_for_time(const double start_velocity, const double half_accel, const double time) {
    return start_velocity * time + half_accel * SQR(time);
}

FORCE_INLINE bool is_pressure_advance_active(const move_t &move) {
    // Pressure advance will be active only when the E axis is extruding (positive direction) and moves simultaneously with at least the X or Y axes.
    // When the material is retracted, the pressure advance is never active.
    // The same way for activating pressure advance is used in Klipper.
    // Klipper, even when the pressure advance is inactive (for retraction and deretraction), it still uses smoothing. So we also do it.
    return is_active_e_axis(move) && !get_dir_e_axis(move) && (is_active_x_axis(move) || is_active_y_axis(move));
}

FORCE_INLINE void pressure_advance_precalculate_parameters(pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    if (is_active_e_axis(*state.current_move)) {
        state.start_v = state.current_move->start_v * state.current_move->axes_r.e;
        state.half_accel = state.current_move->half_accel * state.current_move->axes_r.e;

        if (is_pressure_advance_active(*state.current_move))
            state.start_v += (2. * state.half_accel * params.pressure_advance_value);
    } else {
        state.start_v = 0.;
        state.half_accel = 0.;
    }
}

FORCE_INLINE double calc_distance_for_time_with_pressure_advance_move(const pressure_advance_state_t &state, const double time) {
    assert(time >= 0.);
    return calc_distance_for_time(state.start_v, state.half_accel, time);
}

pressure_advance_window_filter_t create_normalized_bartlett_window_filter(const uint16_t filter_length) {
    if (filter_length > MAX_PRESSURE_ADVANCE_FILTER_LENGTH)
        fatal_error("Filter length is above maximum.", "create_normalized_bartlett_window_filter");

    pressure_advance_window_filter_t filter;
    if (filter_length > 1) {
        filter.length = filter_length;
        const double window_area = (double)(filter_length - 1) / 2.;
        const double length_minus_one = (double)(filter_length - 1);
        for (int idx = 0; idx < filter_length; ++idx) {
            const double idx_d = (double)idx;
            filter.window[idx] = (1. - ABS(2. * (idx_d - 0.5 * length_minus_one) / length_minus_one)) / window_area;
        }
    } else {
        filter.length = 1;
        filter.window[0] = 1.;
    }

    return filter;
}

pressure_advance_window_filter_t create_simple_window_filter(const uint16_t filter_length) {
    if (filter_length > MAX_PRESSURE_ADVANCE_FILTER_LENGTH)
        fatal_error("Filter length is above maximum.", "create_simple_window_filter");

    pressure_advance_window_filter_t filter;
    if (filter_length > 1) {
        filter.length = filter_length;
        for (int idx = 0; idx < filter_length; ++idx)
            filter.window[idx] = 0.;

        int idx = (filter_length - 1) / 2;
        filter.window[idx] = 1.;
        if (filter_length % 2 == 0) {
            filter.window[idx] = 0.5;
            filter.window[idx + 1] = 0.5;
        }
    } else {
        filter.length = 1;
        filter.window[0] = 1.;
    }

    return filter;
}

pressure_advance_params_t create_pressure_advance_params(const pressure_advance::Config &config) {
    pressure_advance_params_t params;
    params.pressure_advance_value = config.pressure_advance;
    params.half_smooth_time = config.smooth_time / 2.;

    // TODO: window type/parameters are currently hardcoded
    constexpr double sampling_rate = 0.001;
    constexpr double filter_length = 41;

    params.sampling_rate = sampling_rate;
    params.filter = create_normalized_bartlett_window_filter(filter_length);
    // params.filter = create_simple_window_filter(filter_length);

    return params;
}

void pressure_advance_step_generator_init(const move_t &move, pressure_advance_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    const uint8_t axis = step_generator.axis;
    pressure_advance_state_t *const pa_state = step_generator.pa_state;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)pressure_advance_step_generator_next_step_event;

    pressure_advance_state_init(*pa_state, PressureAdvance::pressure_advance_params, move, axis);

    step_generator_state.flags |= (!pa_state->step_dir) * STEP_EVENT_FLAG_E_DIR;
    step_generator_state.flags |= ((pa_state->current_interpolated_position != pa_state->previous_interpolated_position) * MOVE_FLAG_E_ACTIVE);
    move.reference_cnt += 1;
}

void pressure_advance_state_init(pressure_advance_state_t &state, const pressure_advance_params_t &params, const move_t &move, const uint8_t axis) {
    assert(is_beginning_empty_move(move));

    state.buffer_length = params.filter.length;
    state.buffer.start_idx = 0;
    state.buffer.same_samples_cnt = 0;
    state.current_move = &move;
    state.current_print_time = 0.;
    state.current_sample_idx = 0;
    state.current_start_position = 0.;
    state.previous_interpolated_position = 0.;
    state.current_interpolated_position = 0.;
    state.step_dir = get_move_step_dir(move, axis);
    state.offset = (double)((int)((state.buffer_length + 1) / 2)) * params.sampling_rate;
#ifndef NDEBUG
    state.position_has_to_be_in_range = false;
#endif
    pressure_advance_precalculate_parameters(state, params);
}

FORCE_INLINE double pressure_advance_apply_linear_advance_filter(pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    if (state.buffer.same_samples_cnt >= params.filter.length)
        return state.current_start_position;

    double filtered_value = 0.;
    const uint16_t index_diff = (params.filter.length - state.buffer.start_idx);
    for (int window_idx = 0; window_idx < index_diff; ++window_idx)
        filtered_value += params.filter.window[window_idx] * state.buffer.data[state.buffer.start_idx + window_idx];

    for (int window_idx = index_diff; window_idx < params.filter.length; ++window_idx)
        filtered_value += params.filter.window[window_idx] * state.buffer.data[window_idx - index_diff];

    return filtered_value;
}

// Based on whether the extruder is active, update the counter for the same samples in the buffer, which is used
// for skipping most of the pressure advance computation for move segments without the active extruder.
FORCE_INLINE void pressure_advance_update_same_samples_count(pressure_advance_state_t &state) {
    if (const bool is_e_active = is_active_e_axis(*state.current_move); !is_e_active && state.buffer.same_samples_cnt < state.buffer_length)
        ++state.buffer.same_samples_cnt;
    else if (is_e_active && state.buffer.same_samples_cnt > 0)
        state.buffer.same_samples_cnt = 0;
}

// Returns true when we were able to get the next sample, false if the buffer wasn't filled up,
// or state->current_move was whole processed, so we need to update to the next move.
FORCE_INLINE bool pressure_advance_sample_next(pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    if (state.current_sample_idx < state.buffer_length) {
        // Buffer is empty or just partly filled. We need to fill it to the filter's length.
        for (size_t idx = state.current_sample_idx; idx < state.buffer_length && state.current_print_time <= (state.current_move->print_time + state.current_move->move_t); ++idx) {
            assert(state.current_print_time + EPSILON >= state.current_move->print_time);
            const double relative_time_of_sample = std::max(state.current_print_time - state.current_move->print_time, 0.);
            const double extruder_position = state.current_start_position + calc_distance_for_time_with_pressure_advance_move(state, relative_time_of_sample);
            state.buffer.data[state.current_sample_idx++] = extruder_position;
            state.current_print_time = state.current_sample_idx * params.sampling_rate;
            pressure_advance_update_same_samples_count(state);
        }

        // False means that the buffer wasn't filled because the move is shorter. So we need to update to the next move.
        return state.current_sample_idx == state.buffer_length;
    } else if (state.current_print_time > (state.current_move->print_time + state.current_move->move_t)) {
        // Buffer is full, but we need one new sample, and state->current_move is whole processed, so we need to update to next move.
        return false;
    } else if (!is_active_e_axis(*state.current_move) && !is_ending_empty_move(*state.current_move) && state.buffer.same_samples_cnt >= state.buffer_length) {
        assert((state.current_move->print_time + state.current_move->move_t + EPSILON) >= state.current_print_time);
        const double remaining_time = std::max(state.current_move->print_time + state.current_move->move_t - state.current_print_time + EPSILON, 0.); // Clamp possible negative value.
        const uint32_t samples_cnt = (uint32_t)std::ceil(remaining_time / params.sampling_rate);

        state.current_sample_idx += samples_cnt;
        state.current_print_time = state.current_sample_idx * params.sampling_rate;

        // Buffer is full, and we just skip the whole move segment because the extruder isn't active.
        return false;
    } else {
        // Buffer is full, and we could sample next new sample from pa_state->current_m
        assert(state.current_print_time + EPSILON >= state.current_move->print_time);
        const double relative_time_of_sample = std::max(state.current_print_time - state.current_move->print_time, 0.);
        const double extruder_position = is_active_e_axis(*state.current_move) ? (state.current_start_position + calc_distance_for_time_with_pressure_advance_move(state, relative_time_of_sample)) : state.current_start_position;
        state.buffer.data[state.buffer.start_idx] = extruder_position;
        state.buffer.start_idx = (state.buffer.start_idx + 1) % state.buffer_length;
        state.current_sample_idx += 1;
        state.current_print_time = state.current_sample_idx * params.sampling_rate;
        pressure_advance_update_same_samples_count(state);
    }

    return true;
}

double pressure_advance_get_next_position(pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    if (!pressure_advance_sample_next(state, params))
        return INFINITY;

    state.previous_interpolated_position = state.current_interpolated_position;
    state.current_interpolated_position = pressure_advance_apply_linear_advance_filter(state, params);
    return state.current_interpolated_position;
}

FORCE_INLINE double pressure_advance_interpolate_step_time(const double prev_position, const double position_diff, const double next_step_position, const pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    if (std::abs(position_diff) >= PRESSURE_ADVANCE_MIN_POSITION_DIFF) {
        const double position_ratio = (next_step_position - prev_position) / position_diff;
        const double step_time_part = params.sampling_rate * position_ratio;
        const double prev_sample_time = params.sampling_rate * (state.current_sample_idx - 1);

        return prev_sample_time + step_time_part - state.offset;
    } else {
        // We round step time into the time of the previous sample.
        const double prev_sample_time = params.sampling_rate * (state.current_sample_idx - 1);
        return prev_sample_time - state.offset;
    }
}

double calc_time_for_distance_pressure_advance(const double distance, pressure_advance_step_generator_t &step_generator, const pressure_advance_params_t &params, step_generator_state_t &step_generator_state) {
    const uint8_t axis = step_generator.axis;
    pressure_advance_state_t &state = *step_generator.pa_state;

    double current_position = state.previous_interpolated_position;
    double next_position = state.current_interpolated_position;

#ifndef NDEBUG
    {
        const double next_step_positive = float(step_generator_state.current_distance[axis]) * Planner::mm_per_step[axis] + distance;
        const double next_step_negative = float(step_generator_state.current_distance[axis] - 1) * Planner::mm_per_step[axis] + distance;
        // When state.position_has_to_be_in_range is true, then have to be next_step_positive or next_step_negative
        // inside the interval (current_position, next_position).
        if (state.position_has_to_be_in_range)
            assert((!state.step_dir || (current_position <= next_step_negative && next_step_negative <= next_position)) && (state.step_dir || (current_position >= next_step_positive && next_step_positive >= next_position)));
    }
#endif

    // Iterate until the ending empty move segment isn't reached and two succeeding positions are the same, which means that the extruder stopped rotating.
    while (!is_ending_empty_move(*state.current_move) || current_position != next_position) {
        // Interpolate step times only when the buffer is completely full.
        if (state.current_sample_idx >= state.buffer_length) {
            const double position_diff = next_position - current_position;
            // If position_diff is equal to zero, then we use the same step_dir as for the previous step time.
            const bool step_dir = (position_diff == 0.) ? state.step_dir : position_diff > 0.; // True - positive direction, False - negative direction

            if (const double next_step_positive = float(step_generator_state.current_distance[axis]) * Planner::mm_per_step[axis] + distance; step_dir && current_position <= next_step_positive && next_step_positive <= next_position) {
#ifndef NDEBUG
                state.position_has_to_be_in_range = true;
#endif
                state.step_dir = step_dir;
                return pressure_advance_interpolate_step_time(current_position, position_diff, next_step_positive, state, params);
            } else if (const double next_step_negative = float(step_generator_state.current_distance[axis] - 1) * Planner::mm_per_step[axis] + distance; !step_dir && current_position >= next_step_negative && next_step_negative >= next_position) {
#ifndef NDEBUG
                state.position_has_to_be_in_range = true;
#endif
                state.step_dir = step_dir;
                return pressure_advance_interpolate_step_time(current_position, position_diff, next_step_negative, state, params);
            }
        }

        if ((next_position = pressure_advance_get_next_position(state, params)) == INFINITY) {
#ifndef NDEBUG
            state.position_has_to_be_in_range = false;
#endif
            return INFINITY;
        }

        current_position = state.previous_interpolated_position;

#ifndef NDEBUG
        state.position_has_to_be_in_range = false;
#endif
    }

    if (next_position == current_position) {
        // Two succeeding positions are the same, which means that the extruder isn't rotating, so we reset the E-axis active flag.
        const uint16_t current_axis_active_flag = (STEP_EVENT_FLAG_X_ACTIVE << axis);
        step_generator_state.flags &= ~current_axis_active_flag;
    }

#ifndef NDEBUG
    state.position_has_to_be_in_range = false;
#endif
    return INFINITY;
}

step_event_info_t pressure_advance_step_generator_next_step_event(pressure_advance_step_generator_t &step_generator, step_generator_state_t &step_generator_state, const double flush_time) {
    assert(step_generator.pa_state != nullptr);
    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0 };
    const move_t *next_move = nullptr;
    do {
        const bool prev_step_dir = step_generator.pa_state->step_dir;
        const float half_step_dist = Planner::mm_per_half_step[step_generator.axis];
        const double step_time = calc_time_for_distance_pressure_advance(half_step_dist, step_generator, PressureAdvance::pressure_advance_params, step_generator_state);

        if (prev_step_dir != step_generator.pa_state->step_dir) {
            // Update step direction flag, which is cached until this move segment is processed.
            const uint16_t current_axis_dir_flag = (STEP_EVENT_FLAG_X_DIR << step_generator.axis);
            step_generator_state.flags &= ~current_axis_dir_flag;
            step_generator_state.flags |= (!step_generator.pa_state->step_dir) * current_axis_dir_flag;
        }

        // When step_time is NaN, it means that next_distance will never be reached.
        // This happens when next_target exceeds end_position, and deceleration decelerates velocity to zero or negative value.
        // Also, we need to stop when step_time exceeds local_end.
        // Be aware that testing, if flush_time was exceeded, has to be after testing for the possibility of updating the pressure advance state.
        const double elapsed_time = step_time;
        if (isnan(step_time) || step_time >= MAX_PRINT_TIME) {
            if (next_move = PreciseStepping::move_segment_queue_next_move(*step_generator.pa_state->current_move); next_move != nullptr) {
                // We are ensuring that the ending empty move segment is always the last move segment in the queue.
                // So we never step into this branch when current_move is pointing to the ending empty move segment.
                assert(!is_ending_empty_move(*step_generator.pa_state->current_move));

                --step_generator.pa_state->current_move->reference_cnt;
                step_generator.pa_state->current_move = next_move;
                ++step_generator.pa_state->current_move->reference_cnt;

                if (is_pressure_advance_active(*next_move))
                    step_generator.pa_state->current_start_position = next_move->start_pos.e + (next_move->start_v * next_move->axes_r.e * PressureAdvance::pressure_advance_params.pressure_advance_value);
                else
                    step_generator.pa_state->current_start_position = next_move->start_pos.e;

                pressure_advance_precalculate_parameters(PressureAdvance::pressure_advance_state, PressureAdvance::pressure_advance_params);

                PreciseStepping::move_segment_processed_handler();
            } else {
                step_generator.reached_end_of_move_queue = true;
#ifndef NDEBUG
                step_generator.pa_state->position_has_to_be_in_range = false;
#endif
            }
        } else if (elapsed_time > flush_time) {
            step_generator.reached_end_of_move_queue = true;
#ifndef NDEBUG
            step_generator.pa_state->position_has_to_be_in_range = false;
#endif
            break;
        } else {
            // If the condition above is met, then definitely this axis is active.
            // And because we always set the bit to a high value, we don't need to clear it.
            step_generator_state.flags |= (STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis);

            next_step_event.time = elapsed_time;
            next_step_event.flags = STEP_EVENT_FLAG_STEP_X << step_generator.axis;
            next_step_event.flags |= step_generator_state.flags;
            step_generator_state.current_distance[step_generator.axis] += (step_generator.pa_state->step_dir ? 1 : -1);
            break;
        }
    } while (next_move != nullptr);

    return next_step_event;
}
