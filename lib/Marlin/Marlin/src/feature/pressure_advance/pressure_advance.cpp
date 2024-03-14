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

#include "bsod.h"

#include <cmath>
#include "core/macros.h"
#include "module/planner.h"

pressure_advance_params_t PressureAdvance::pressure_advance_params;
pressure_advance_state_t PressureAdvance::pressure_advance_state;

FORCE_INLINE bool is_pressure_advance_active(const move_t &move) {
    // Pressure advance will be active only when the E axis is extruding (positive direction) and moves simultaneously with at least the X or Y axes.
    // When the material is retracted, the pressure advance is never active.
    // The same way for activating pressure advance is used in Klipper.
    // Klipper, even when the pressure advance is inactive (for retraction and deretraction), it still uses smoothing. So we also do it.
    return is_active_e_axis(move) && !get_dir_e_axis(move) && (is_active_x_axis(move) || is_active_y_axis(move));
}

FORCE_INLINE void pressure_advance_precalculate_parameters(pressure_advance_step_generator_t &step_generator, const pressure_advance_params_t &params) {
    pressure_advance_state_t &state = *step_generator.pa_state;
    const move_t &current_move = *state.current_move;
    if (is_active_e_axis(current_move)) {
        state.start_v = float(get_move_start_v(current_move, step_generator.axis));
        state.half_accel = float(get_move_half_accel(current_move, step_generator.axis));

        if (is_pressure_advance_active(current_move)) {
            state.start_v += (2.f * state.half_accel * params.pressure_advance_value);
        }
    } else {
        state.start_v = 0.f;
        state.half_accel = 0.f;
    }

    if (!is_ending_empty_move(current_move)) {
        const double current_move_end_time = current_move.print_time + current_move.move_t;
        state.current_move_last_total_sample_idx = uint32_t(current_move_end_time / params.sampling_rate);
    } else {
        state.current_move_last_total_sample_idx = std::numeric_limits<uint32_t>::max();
    }
}

FORCE_INLINE float calc_distance_for_time_with_pressure_advance_move(const pressure_advance_state_t &state, const float time) {
    assert(time >= 0.);
    return calc_distance<float>(state.start_v, state.half_accel, time);
}

pressure_advance_window_filter_t create_normalized_bartlett_window_filter(const uint16_t filter_length) {
    if (filter_length > PRESSURE_ADVANCE_MAX_FILTER_LENGTH) {
        fatal_error("Filter length is above maximum.", "create_normalized_bartlett_window_filter");
    }

    pressure_advance_window_filter_t filter;
    if (filter_length > 1) {
        filter.length = filter_length;
        const double window_area = (double)(filter_length - 1) / 2.;
        const double length_minus_one = (double)(filter_length - 1);
        for (int idx = 0; idx < filter_length; ++idx) {
            const double idx_d = (double)idx;
            filter.window[idx] = float((1. - ABS(2. * (idx_d - 0.5 * length_minus_one) / length_minus_one)) / window_area);
        }
    } else {
        filter.length = 1;
        filter.window[0] = 1.f;
    }

    return filter;
}

pressure_advance_window_filter_t create_simple_window_filter(const uint16_t filter_length) {
    if (filter_length > PRESSURE_ADVANCE_MAX_FILTER_LENGTH) {
        bsod("Filter length is above maximum.");
    }

    pressure_advance_window_filter_t filter;
    if (filter_length > 1) {
        filter.length = filter_length;
        for (int idx = 0; idx < filter_length; ++idx) {
            filter.window[idx] = 0.f;
        }

        int idx = (filter_length - 1) / 2;
        filter.window[idx] = 1.f;
        if (filter_length % 2 == 0) {
            filter.window[idx] = 0.5f;
            filter.window[idx + 1] = 0.5f;
        }
    } else {
        filter.length = 1;
        filter.window[0] = 1.f;
    }

    return filter;
}

static constexpr double round(const double value, const uint32_t number_of_decimal_places) {
    const double precision = double(std::pow(10, number_of_decimal_places));
    return std::round(value * precision) / precision;
}

pressure_advance_params_t create_pressure_advance_params(const pressure_advance::Config &config) {
    pressure_advance_params_t params;
    params.pressure_advance_value = config.pressure_advance;

    constexpr const uint16_t filter_length = PRESSURE_ADVANCE_MAX_FILTER_LENGTH;
    params.sampling_rate = (filter_length > 1) ? round(double(config.smooth_time) / (filter_length - 1), 6) : 0.001;
    params.sampling_rate_float = float(params.sampling_rate);
#ifndef PRESSURE_ADVANCE_SIMPLE_WINDOW_FILTER
    params.filter = create_normalized_bartlett_window_filter(filter_length);
#else
    params.filter = create_simple_window_filter(filter_length);
#endif
    params.filter_total_time = params.sampling_rate * double(filter_length);
    params.filter_delay = double(filter_length / 2) * params.sampling_rate;

    return params;
}

void pressure_advance_step_generator_init(const move_t &move, pressure_advance_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    const uint8_t axis = step_generator.axis;
    pressure_advance_state_t *const pa_state = step_generator.pa_state;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)pressure_advance_step_generator_next_step_event;

    pressure_advance_state_init(step_generator, PressureAdvance::pressure_advance_params, move, axis);

    step_generator_state.flags |= (!pa_state->step_dir) * STEP_EVENT_FLAG_E_DIR;
    step_generator_state.flags |= ((pa_state->next_position != pa_state->prev_position) * MOVE_FLAG_E_ACTIVE);
    move.reference_cnt += 1;
}

void pressure_advance_state_init(pressure_advance_step_generator_t &step_generator, const pressure_advance_params_t &params, const move_t &move, const uint8_t axis) {
    assert(is_beginning_empty_move(move));
    pressure_advance_state_t &state = *step_generator.pa_state;

    state.buffer.length = params.filter.length;
    state.buffer.start_idx = 0;
    state.buffer.same_samples_cnt = 0;

    state.current_move = &move;
    state.start_v = 0.f;
    state.half_accel = 0.f;
    state.start_pos = 0.f;
    state.step_dir = get_move_step_dir(move, axis);

    state.total_sample_idx = 0;
    state.local_sample_idx = 0;
    state.local_sample_time_left = 0.f;
    state.current_move_last_total_sample_idx = 0;

    state.prev_position = 0.f;
    state.next_position = 0.f;

#ifndef NDEBUG
    state.position_has_to_be_in_range = false;
#endif
    pressure_advance_precalculate_parameters(step_generator, params);
}

FORCE_INLINE float pressure_advance_apply_filter(pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    assert(params.filter.length == state.buffer.length);
    if (state.buffer.same_samples_cnt >= params.filter.length) {
        return state.start_pos;
    }

    float filtered_value = 0.;
    const uint16_t index_diff = (params.filter.length - state.buffer.start_idx);
    for (int window_idx = 0; window_idx < index_diff; ++window_idx) {
        filtered_value += params.filter.window[window_idx] * state.buffer.data[state.buffer.start_idx + window_idx];
    }

    for (int window_idx = index_diff; window_idx < params.filter.length; ++window_idx) {
        filtered_value += params.filter.window[window_idx] * state.buffer.data[window_idx - index_diff];
    }

    return filtered_value;
}

// Based on whether the extruder is active, update the counter for the same samples in the buffer, which is used
// for skipping most of the pressure advance computation for move segments without the active extruder.
FORCE_INLINE void pressure_advance_update_same_samples_count(pressure_advance_state_t &state) {
    if (const bool is_e_active = is_active_e_axis(*state.current_move); !is_e_active && state.buffer.same_samples_cnt < state.buffer.length) {
        ++state.buffer.same_samples_cnt;
    } else if (is_e_active && state.buffer.same_samples_cnt > 0) {
        state.buffer.same_samples_cnt = 0;
    }
}

// Returns true when we were able to get the next sample, false if the buffer wasn't filled up,
// or state->current_move was whole processed, so we need to update to the next move.
FORCE_INLINE bool pressure_advance_sample_next(pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    if (state.total_sample_idx < state.buffer.length) {
        // Buffer is empty or just partly filled. We need to fill it to the filter's length.
        assert(state.local_sample_idx == 0);
        for (; state.total_sample_idx < state.buffer.length && state.total_sample_idx <= state.current_move_last_total_sample_idx; ++state.total_sample_idx, ++state.local_sample_idx) {
            const float next_local_sample_time = state.local_sample_time_left + state.local_sample_idx * params.sampling_rate_float;

            assert(next_local_sample_time + EPSILON >= state.current_move->print_time);
            const float extruder_position = state.start_pos + calc_distance_for_time_with_pressure_advance_move(state, next_local_sample_time);
            state.buffer.data[state.total_sample_idx] = extruder_position;
            pressure_advance_update_same_samples_count(state);
        }

        // False means that the buffer wasn't filled because the move is shorter. So we need to update to the next move.
        return state.total_sample_idx == state.buffer.length;
    } else if (state.total_sample_idx > state.current_move_last_total_sample_idx) {
        // Buffer is full, but we need one new sample, and state->current_move is whole processed, so we need to update to next move.
        return false;
    } else if (!is_active_e_axis(*state.current_move) && !is_ending_empty_move(*state.current_move) && state.buffer.same_samples_cnt >= state.buffer.length) {
        const uint32_t samples_cnt = state.current_move_last_total_sample_idx - state.total_sample_idx + 1;
        state.total_sample_idx += samples_cnt;
        state.local_sample_idx += samples_cnt;

        // Buffer is full, and we just skip the whole move segment because the extruder isn't active.
        return false;
    } else {
        // Buffer is full, and we could sample next new sample from pa_state->current_m
        const float next_local_sample_time = state.local_sample_time_left + state.local_sample_idx * params.sampling_rate_float;
        const float extruder_position = is_active_e_axis(*state.current_move) ? (state.start_pos + calc_distance_for_time_with_pressure_advance_move(state, next_local_sample_time)) : state.start_pos;
        state.buffer.data[state.buffer.start_idx] = extruder_position;
        state.buffer.start_idx = (state.buffer.start_idx + 1) % state.buffer.length;
        state.total_sample_idx++;
        state.local_sample_idx++;
        pressure_advance_update_same_samples_count(state);
    }

    return true;
}

FORCE_INLINE float pressure_advance_get_next_position(pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    if (!pressure_advance_sample_next(state, params)) {
        return std::numeric_limits<float>::infinity();
    }

    return pressure_advance_apply_filter(state, params);
}

FORCE_INLINE double pressure_advance_step_time_of_prev_sample(const pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    return params.sampling_rate * (state.total_sample_idx - 2) - params.filter_delay;
}

FORCE_INLINE double pressure_advance_step_time_of_next_sample(const pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    return params.sampling_rate * (state.total_sample_idx - 1) - params.filter_delay;
}

FORCE_INLINE double pressure_advance_interpolate_step_time(const float prev_position, const float position_diff, const float next_step_position, const pressure_advance_state_t &state, const pressure_advance_params_t &params) {
    assert(state.total_sample_idx >= 2);
    if (std::abs(position_diff) >= PRESSURE_ADVANCE_MIN_POSITION_DIFF) {
        const float position_ratio = std::clamp((next_step_position - prev_position) / position_diff, 0.f, 1.f);
        const double step_time = params.sampling_rate * (double(state.total_sample_idx - 2) + double(position_ratio));

        return step_time - params.filter_delay;
    } else {
        // We round step time into the time of the previous sample.
        return pressure_advance_step_time_of_prev_sample(state, params);
    }
}

// The pressure advance reaches the end of processing when current_move is equal to the ending empty move segment and two succeeding positions are the same.
FORCE_INLINE bool is_pressure_advance_reached_end(const pressure_advance_state_t &state) {
    return is_ending_empty_move(*state.current_move) && state.prev_position == state.next_position;
}

double calc_time_for_distance_pressure_advance(const float distance, pressure_advance_step_generator_t &step_generator, const pressure_advance_params_t &params, step_generator_state_t &step_generator_state) {
    const uint8_t axis = step_generator.axis;
    pressure_advance_state_t &state = *step_generator.pa_state;
    // We need to ensure that when the filter is applied for the first time, then the value will be 0.
    // Because of that, we need the beginning empty move segment with duration at least equal to sampling_rate * filter.length.
    assert(!is_beginning_empty_move(*state.current_move) || state.current_move->move_t >= (params.sampling_rate * params.filter.length));

#ifndef NDEBUG
    {
        const double next_step_positive = float(step_generator_state.current_distance[axis]) * Planner::mm_per_step[axis] + distance;
        const double next_step_negative = float(step_generator_state.current_distance[axis] - 1) * Planner::mm_per_step[axis] + distance;
        // When state.position_has_to_be_in_range is true, then have to be next_step_positive or next_step_negative
        // inside the interval (state.prev_position, state.next_position).
        if (state.position_has_to_be_in_range) {
            assert((!state.step_dir || (state.prev_position <= next_step_negative && next_step_negative <= state.next_position)) && (state.step_dir || (state.prev_position >= next_step_positive && next_step_positive >= state.next_position)));
        }
    }
#endif

    // Iterate until the ending empty move segment isn't reached and two succeeding positions are the same, which means that the extruder stopped rotating.
    while (!is_pressure_advance_reached_end(state)) {
        // Interpolate step times only when the buffer is completely full and when both prev_position
        // and next_position contain values after applying the filter.
        // The second condition is met when the number of samples is at least one more than the filter length.
        if (state.total_sample_idx > state.buffer.length) {
            const float position_diff = state.next_position - state.prev_position;
            // If position_diff is equal to zero, then we use the same step_dir as for the previous step time.
            const bool step_dir = (position_diff == 0.f) ? state.step_dir : position_diff > 0.f; // True - positive direction, False - negative direction

            if (const float next_step_positive = float(step_generator_state.current_distance[axis]) * Planner::mm_per_step[axis] + distance; step_dir && state.prev_position <= next_step_positive && next_step_positive <= state.next_position) {
#ifndef NDEBUG
                state.position_has_to_be_in_range = true;
#endif
                state.step_dir = step_dir;
                return pressure_advance_interpolate_step_time(state.prev_position, position_diff, next_step_positive, state, params);
            } else if (const float next_step_negative = float(step_generator_state.current_distance[axis] - 1) * Planner::mm_per_step[axis] + distance; !step_dir && state.prev_position >= next_step_negative && next_step_negative >= state.next_position) {
#ifndef NDEBUG
                state.position_has_to_be_in_range = true;
#endif
                state.step_dir = step_dir;
                return pressure_advance_interpolate_step_time(state.prev_position, position_diff, next_step_negative, state, params);
            }
        }

        if (const float new_next_position = pressure_advance_get_next_position(state, params); new_next_position == std::numeric_limits<float>::infinity()) {
#ifndef NDEBUG
            state.position_has_to_be_in_range = false;
#endif
            return std::numeric_limits<double>::infinity();
        } else {
            state.prev_position = state.next_position;
            state.next_position = new_next_position;
        }

#ifndef NDEBUG
        state.position_has_to_be_in_range = false;
#endif
    }

    if (state.next_position == state.prev_position) {
        // Two succeeding positions are the same, which means that the extruder isn't rotating, so we reset the E-axis active flag.
        const uint16_t current_axis_active_flag = (STEP_EVENT_FLAG_X_ACTIVE << axis);
        step_generator_state.flags &= ~current_axis_active_flag;
    }

#ifndef NDEBUG
    state.position_has_to_be_in_range = false;
#endif
    return std::numeric_limits<double>::infinity();
}

// Apply reset of position on the pressure advance data structure and adjust position in steps (current_distance).
void pressure_advance_reset_position(pressure_advance_step_generator_t &step_generator, step_generator_state_t &step_generator_state, const move_t &next_move) {
    assert(step_generator.pa_state->total_sample_idx >= step_generator.pa_state->buffer.length);
    const uint8_t axis = step_generator.axis;
    const move_t &current_move = *step_generator.pa_state->current_move;
    const double c_end_pos = get_move_end_pos(current_move, axis);
    const double n_start_pos = get_move_start_pos(next_move, axis);

    // In every case, the difference in axis position between next_move and current_move is equal to a whole number of steps.
    const int32_t axis_diff_steps = int32_t(std::round(float(n_start_pos - c_end_pos) * Planner::settings.axis_steps_per_mm[axis]));
    const float axis_diff = float(axis_diff_steps) * Planner::mm_per_step[axis];

    // We have ensured that there is at least a number of samples equal to the buffer size. So, the whole buffer is filled with sampled positions.
    for (uint32_t buffer_idx = 0; buffer_idx < step_generator.pa_state->buffer.length; ++buffer_idx) {
        step_generator.pa_state->buffer.data[buffer_idx] += axis_diff;
    }

    // Because this function is called when the next step position isn't within the interval (prev_position, next_position),
    // we don't have to care about numeric issues. We have to only ensure that when prev_position and next_position are equal,
    // then after resetting, they will also equal.
    float new_next_position = pressure_advance_apply_filter(*step_generator.pa_state, PressureAdvance::pressure_advance_params);
    if (step_generator.pa_state->prev_position == step_generator.pa_state->next_position) {
        step_generator.pa_state->prev_position = new_next_position;
    } else {
        step_generator.pa_state->prev_position += axis_diff;
    }

    step_generator.pa_state->next_position = new_next_position;

    // Because the pressure advance adds additional steps and there is a delay between the current move segment
    // and current_distance, we need to recalculate current_distance instead of just resetting to zero.
    step_generator_state.current_distance[axis] += axis_diff_steps;
}

step_event_info_t pressure_advance_step_generator_next_step_event(pressure_advance_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    assert(step_generator.pa_state != nullptr);
    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };
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

        // When step_time is infinity, it means that next_distance will never be reached.
        // This happens when next_target exceeds end_position, and deceleration decelerates velocity to zero or negative value.
        // Also, we need to stop when step_time exceeds local_end.
        const double elapsed_time = step_time;
        if (step_time >= MAX_PRINT_TIME) {
            if (is_pressure_advance_reached_end(*step_generator.pa_state)) {
                assert(step_generator.pa_state->current_move->move_t == MAX_PRINT_TIME);
                next_step_event.time = step_generator.pa_state->current_move->print_time + step_generator.pa_state->current_move->move_t;
            } else {
                next_step_event.time = pressure_advance_step_time_of_next_sample(*step_generator.pa_state, PressureAdvance::pressure_advance_params);
            }

            if (next_move = PreciseStepping::move_segment_queue_next_move(*step_generator.pa_state->current_move); next_move != nullptr) {
                // We are ensuring that the ending empty move segment is always the last move segment in the queue.
                // So we never step into this branch when current_move is pointing to the ending empty move segment.
                assert(!is_ending_empty_move(*step_generator.pa_state->current_move));

                // We have to update start_post before we reset the pressure advance position because
                // we are using it during the resetting position.
                if (is_pressure_advance_active(*next_move)) {
                    step_generator.pa_state->start_pos = float(get_move_start_pos(*next_move, step_generator.axis)) + float(get_move_start_v(*next_move, step_generator.axis)) * PressureAdvance::pressure_advance_params.pressure_advance_value;
                } else {
                    step_generator.pa_state->start_pos = float(get_move_start_pos(*next_move, step_generator.axis));
                }

                // Apply reset of position on the pressure advance data structure and adjust position in steps (current_distance).
                if (next_move->flags & (MOVE_FLAG_RESET_POSITION_X << step_generator.axis)) {
                    pressure_advance_reset_position(step_generator, step_generator_state, *next_move);
                }

                --step_generator.pa_state->current_move->reference_cnt;
                step_generator.pa_state->current_move = next_move;
                ++step_generator.pa_state->current_move->reference_cnt;

                step_generator.pa_state->local_sample_idx = 0;
                step_generator.pa_state->local_sample_time_left = std::max(float((step_generator.pa_state->total_sample_idx * PressureAdvance::pressure_advance_params.sampling_rate) - step_generator.pa_state->current_move->print_time), 0.f);

                pressure_advance_precalculate_parameters(step_generator, PressureAdvance::pressure_advance_params);

                PreciseStepping::move_segment_processed_handler();
            } else {
#ifndef NDEBUG
                step_generator.pa_state->position_has_to_be_in_range = false;
#endif
            }
        } else {
            // If the condition above is met, then definitely this axis is active.
            // And because we always set the bit to a high value, we don't need to clear it.
            step_generator_state.flags |= (STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis);

            next_step_event.time = elapsed_time;
            next_step_event.flags = STEP_EVENT_FLAG_STEP_X << step_generator.axis;
            next_step_event.flags |= step_generator_state.flags;
            next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_VALID;
            step_generator_state.current_distance[step_generator.axis] += (step_generator.pa_state->step_dir ? 1 : -1);
            break;
        }
    } while (next_move != nullptr);

    return next_step_event;
}
