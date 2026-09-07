/**
 * Based on the implementation of the input shaper in Klipper [https://github.com/Klipper3d/klipper].
 * Copyright (C) Dmitry Butyugin <dmbutyugin@google.com>
 * Copyright (C) Kevin O'Connor <kevin@koconnor.net>
 *
 * Our implementation takes inspiration from the work of Dmitry Butyugin <dmbutyugin@google.com>
 * and Kevin O'Connor <kevin@koconnor.net> for Klipper in used data structures, input shaper filters,
 * and some computations.
 *
 * We chose a different approach for implementing the input shaper that is less computationally demanding
 * and can be fully run on a single 32-bit MCU with identical results as the implementation in Klipper.
 */
#include "input_shaper.hpp"

#include "../precise_stepping/precise_stepping.hpp"
#include "../precise_stepping/internal.hpp"

#include "../../module/planner.h"

#include "bsod.h"
#include "input_shaper_config.hpp"
#include <config_store/store_instance.hpp>

#include <cmath>
#include <cfloat>

input_shaper_state_t InputShaper::is_state[3];
input_shaper_pulses_t InputShaper::logical_axis_pulses[3];

static void init_input_shaper_pulses(const float a[], const float t[], const int num_pulses, input_shaper_pulses_t *is_pulses) {
    float sum_a = 0.f;
    for (int i = 0; i < num_pulses; ++i) {
        sum_a += a[i];
    }

    // Reverse pulses vs. their traditional definition
    // and normalize their amplitudes
    //
    // In traditional definition time is increasing
    // as it is running forward.
    // If we assume time axis to have negative values on the left
    // and positive values on the right signal is moving from
    // left to the right as time goes forward.
    // In tradition definition pulses with lower index have lower
    // time and are encountered first by the signal.
    //
    // In our definition of time meaning of the time is time
    // remaining to some event. In our definition time
    // is decreasing as it is running forward.
    // If we assume time axis to have negative values on the left
    // and positive values on the right signal is moving from
    // right to the left as time goes forward.
    // In our definition pulses with higher index have higher
    // time and are encountered first by the signal.
    // As the time is moving in opposite direction,
    // we need to reverse pulses to ensure first pulse
    // from traditional definition is encountered first
    // by the signal in our definition.
    // In our definition we use same index vs. time
    // ordering as in traditional definition
    // (lowest index has lowest time and vice versa)
    // So purely reordering would break ordering.
    // Therefore we also flip pulse time sign to preserve
    // ordering.
    //
    // Amplitude normalization ensures that sum
    // of all pulse amplitudes is 1. This ensures that
    // sparse moves reaches their original target destinations.
    // If the sum would be greater than 1, printed object would be
    // enlarged in that axis, if lower than 1 it would be reduced.

    // All intermediate time arithmetic stays in double to avoid precision loss
    // during centering; only the final centered values are converted to TimeTicks.
    double intermediate_t[INPUT_SHAPER_MAX_PULSES];

    const float inv_sum_a = 1.f / sum_a;
    for (int i = 0; i < num_pulses; ++i) {
        is_pulses->pulses[num_pulses - i - 1].a = a[i] * inv_sum_a;
        intermediate_t[num_pulses - i - 1] = -double(t[i]);
    }

    double pulses_weighted_average_time = 0.;
    for (int i = 0; i < num_pulses; ++i) {
        // not sure if pulse centering algorithm works for negative amplitude
        // check if needed
        debug_assert((is_pulses->pulses[i].a) >= 0.f);
        static_assert(0.f == -0.f);
        // no other operation needed as sum of amplitudes is already 1 at this point
        pulses_weighted_average_time += double(is_pulses->pulses[i].a) * intermediate_t[i];
    }

    // Center pulses around zero time
    //
    // This ensures at least some synchronization between
    // different axes with different or no input shapers
    for (int i = 0; i < num_pulses; ++i) {
        intermediate_t[i] -= pulses_weighted_average_time;
        is_pulses->pulses[i].t = TimeTicks::from_seconds(intermediate_t[i]);
    }

    is_pulses->num_pulses = num_pulses;
}

input_shaper::Shaper input_shaper::get(const float damping_ratio, const float shaper_freq, const float vibration_reduction, const input_shaper::Type type) {
    if (shaper_freq <= 0.f) {
        bsod("Zero or negative frequency of input shaper.");
    } else if (damping_ratio >= 1.f) {
        bsod("Damping ration must always be less than 1.");
    }

    switch (type) {
    case Type::zv: {
        constexpr int num_pulses = 2;
        const float df = std::sqrt(1.f - SQR(damping_ratio));
        const float K = std::exp(-damping_ratio * float(M_PI) / df);
        const float t_d = 1.f / (shaper_freq * df);

        const float a[num_pulses] = { 1.f, K };
        const float t[num_pulses] = { 0.f, .5f * t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::zvd: {
        constexpr int num_pulses = 3;
        const float df = std::sqrt(1.f - SQR(damping_ratio));
        const float K = std::exp(-damping_ratio * float(M_PI) / df);
        const float t_d = 1.f / (shaper_freq * df);

        const float a[num_pulses] = { 1.f, 2.f * K, SQR(K) };
        const float t[num_pulses] = { 0.f, .5f * t_d, t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::mzv: {
        constexpr int num_pulses = 3;
        const float df = std::sqrt(1.f - SQR(damping_ratio));
        const float K = std::exp(-.75f * damping_ratio * float(M_PI) / df);
        const float t_d = 1.f / (shaper_freq * df);

        const float a1 = 1.f - 1.f / std::sqrt(2.f);
        const float a2 = (std::sqrt(2.f) - 1.f) * K;
        const float a3 = a1 * K * K;

        const float a[num_pulses] = { a1, a2, a3 };
        const float t[num_pulses] = { 0.f, .375f * t_d, .75f * t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::ei: {
        constexpr int num_pulses = 3;
        const float v_tol = 1.f / vibration_reduction; // vibration tolerance
        const float df = std::sqrt(1.f - SQR(damping_ratio));
        const float K = std::exp(-damping_ratio * float(M_PI) / df);
        const float t_d = 1.f / (shaper_freq * df);

        const float a1 = .25f * (1.f + v_tol);
        const float a2 = .5f * (1.f - v_tol) * K;
        const float a3 = a1 * K * K;

        float a[num_pulses] = { a1, a2, a3 };
        float t[num_pulses] = { 0.f, .5f * t_d, t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::ei_2hump: {
        constexpr int num_pulses = 4;
        const float v_tol = 1.f / vibration_reduction; // vibration tolerance
        const float df = std::sqrt(1.f - SQR(damping_ratio));
        const float K = std::exp(-damping_ratio * float(M_PI) / df);
        const float t_d = 1.f / (shaper_freq * df);
        const float V2 = SQR(v_tol);
        const float X = std::pow(V2 * (std::sqrt(1.f - V2) + 1.f), 1.f / 3.f);

        const float a1 = (3.f * X * X + 2.f * X + 3.f * V2) / (16.f * X);
        const float a2 = (.5f - a1) * K;
        const float a3 = a2 * K;
        const float a4 = a1 * K * K * K;

        float a[num_pulses] = { a1, a2, a3, a4 };
        float t[num_pulses] = { 0.f, .5f * t_d, t_d, 1.5f * t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::ei_3hump: {
        constexpr int num_pulses = 5;
        const float v_tol = 1.f / vibration_reduction; // vibration tolerance
        const float df = std::sqrt(1.f - SQR(damping_ratio));
        const float K = std::exp(-damping_ratio * float(M_PI) / df);
        const float t_d = 1.f / (shaper_freq * df);
        const float K2 = K * K;

        const float a1 = 0.0625f * (1.f + 3.f * v_tol + 2.f * std::sqrt(2.f * (v_tol + 1.f) * v_tol));
        const float a2 = 0.25f * (1.f - v_tol) * K;
        const float a3 = (0.5f * (1.f + v_tol) - 2.f * a1) * K2;
        const float a4 = a2 * K2;
        const float a5 = a1 * K2 * K2;

        float a[num_pulses] = { a1, a2, a3, a4, a5 };
        float t[num_pulses] = { 0.f, .5f * t_d, t_d, 1.5f * t_d, 2.f * t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::cnt: // Fallback to null filter
    case Type::null: {
        constexpr int num_pulses = 1;
        float a[num_pulses] = { 1.f };
        float t[num_pulses] = { 0.f };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    }
    bsod("input_shaper::Type out of range");
}

void create_zv_input_shaper_pulses(input_shaper_pulses_t &is_pulses, const float shaper_freq, const float damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0.f, input_shaper::Type::zv);
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
}

void create_zvd_input_shaper_pulses(input_shaper_pulses_t &is_pulses, const float shaper_freq, const float damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0.f, input_shaper::Type::zvd);
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
}

void create_mzv_input_shaper_pulses(input_shaper_pulses_t &is_pulses, const float shaper_freq, const float damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0.f, input_shaper::Type::mzv);
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
}

void create_ei_input_shaper_pulses(input_shaper_pulses_t &is_pulses, const float shaper_freq, const float damping_ratio, const float vibration_reduction) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, vibration_reduction, input_shaper::Type::ei);
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
}

void create_2hump_ei_input_shaper_pulses(input_shaper_pulses_t &is_pulses, const float shaper_freq, const float damping_ratio, const float vibration_reduction) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, vibration_reduction, input_shaper::Type::ei_2hump);
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
}

void create_3hump_ei_input_shaper_pulses(input_shaper_pulses_t &is_pulses, const float shaper_freq, const float damping_ratio, const float vibration_reduction) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, vibration_reduction, input_shaper::Type::ei_3hump);
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
}

void create_null_input_shaper_pulses(input_shaper_pulses_t &is_pulses) {
    input_shaper::Shaper shaper = input_shaper::get(NAN, NAN, NAN, input_shaper::Type::null);
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
}

void input_shaper_step_generator_init(const move_t &move, input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    const uint8_t axis = step_generator.axis;
    debug_assert(axis == X_AXIS || axis == Y_AXIS || axis == Z_AXIS);
    input_shaper_state_t *const is_state = step_generator.is_state;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)input_shaper_step_generator_next_step_event;

    // Set the initial step flags from the last cached values
    step_generator.step_flags = 0;
    step_generator.step_flags |= step_generator_state.current_flags & (STEP_EVENT_FLAG_X_DIR << axis);
    step_generator.step_flags |= step_generator_state.current_flags & (STEP_EVENT_FLAG_X_ACTIVE << axis);

    // Set the initial direction and activity flags for the entire next move
    step_generator.move_step_flags = 0;
    step_generator.move_step_flags |= (!is_state->step_dir) * (STEP_EVENT_FLAG_X_DIR << axis);
    step_generator.move_step_flags |= (is_state->start_v != 0.f || is_state->half_accel != 0.f) * (STEP_EVENT_FLAG_X_ACTIVE << axis);

#ifdef COREXY
    if (axis == X_AXIS || axis == Y_AXIS) {
        debug_assert(is_state->m_logical_axis_pulses[0]->num_pulses == is_state->m_logical_axis_pulses[1]->num_pulses);
        is_state->m_logical_axis_pulses_cnt = 2;
        is_state->m_logical_axis_pulses[0] = &InputShaper::logical_axis_pulses[X_AXIS];
        is_state->m_logical_axis_pulses[1] = &InputShaper::logical_axis_pulses[Y_AXIS];
    } else if (axis == Z_AXIS) {
        is_state->m_logical_axis_pulses_cnt = 1;
        is_state->m_logical_axis_pulses[0] = &InputShaper::logical_axis_pulses[axis];
    } else {
        bsod("Input shaper isn't supported on this axis for CoreXY kinematics.");
    }
#else
    if (axis == X_AXIS || axis == Y_AXIS || axis == Z_AXIS) {
        is_state->m_logical_axis_pulses_cnt = 1;
        is_state->m_logical_axis_pulses[0] = &InputShaper::logical_axis_pulses[axis];
    } else {
        bsod("Input shaper isn't supported on this axis for Cartesian kinematics.");
    }
#endif

    move.reference_cnt += is_state->m_logical_axis_pulses[0]->num_pulses;

    input_shaper_state_init(*is_state, move, axis);
    input_shaper_step_generator_update(step_generator);
}

void input_shaper_state_init(input_shaper_state_t &is_state, const move_t &move, const uint8_t physical_axis) {
    debug_assert(is_beginning_empty_move(move));
    debug_assert(move.print_time == TimeTicks::zero());
    debug_assert(is_state.m_logical_axis_pulses_cnt == 1 || is_state.m_logical_axis_pulses_cnt == 2);
    debug_assert(is_state.m_logical_axis_pulses[0]->num_pulses > 0);
    debug_assert(is_state.m_logical_axis_pulses_cnt != 2 || is_state.m_logical_axis_pulses[0]->num_pulses == is_state.m_logical_axis_pulses[1]->num_pulses);

    // We assume that for CoreXY, both m_logical_axis_pulses have the same time coefficient, so we can always choose the first one.
    const input_shaper_pulses_t &logical_axis_pulses = *is_state.m_logical_axis_pulses[0];
    for (uint8_t pulse_idx = 0; pulse_idx < logical_axis_pulses.num_pulses; ++pulse_idx) {
        debug_assert(is_state.m_logical_axis_pulses_cnt != 2 || logical_axis_pulses.pulses[pulse_idx].t == is_state.m_logical_axis_pulses[1]->pulses[pulse_idx].t);
        is_state.m_next_change[pulse_idx] = move.print_time + move.move_time - logical_axis_pulses.pulses[pulse_idx].t;
        is_state.m_move[pulse_idx] = &move;
    }

    if (is_state.m_logical_axis_pulses_cnt == 1) {
        is_state.start_pos = float(get_move_start_pos(move, physical_axis));
    } else if (is_state.m_logical_axis_pulses_cnt == 2) {
#ifdef COREXY
        if (physical_axis == A_AXIS) {
            is_state.start_pos = float(move.start_pos.x + move.start_pos.y);
        } else if (physical_axis == B_AXIS) {
            is_state.start_pos = float(move.start_pos.x - move.start_pos.y);
        } else {
            bsod("Invalid combination of axis and number of logical input shapers for CoreXY kinematics.");
        }
#else
        bsod("Unsupported number of logical axes for used kinematics.");
#endif
    } else {
        bsod("Unsupported number of logical input shapers.");
    }

    is_state.half_accel = float(get_move_half_accel(move, physical_axis));
    is_state.start_v = float(get_move_start_v(move, physical_axis));
    is_state.step_dir = get_move_step_dir(move, physical_axis);

    is_state.m_physical_axis = physical_axis;
    is_state.m_is_crossing_zero_velocity = false;
    is_state.m_nearest_next_change_idx = is_state.calc_nearest_next_change_idx();

    is_state.print_time = move.print_time;
    is_state.nearest_next_change = is_state.get_nearest_next_change();
}

static bool input_shaper_state_step_dir(input_shaper_state_t &is_state) {
    // Previously we ensured that none of micro move segments would cross zero velocity.
    // So this function can correctly determine step_dir only when this assumption is met.

    float start_v = is_state.start_v;
    float half_accel = is_state.half_accel;

    if (start_v < 0.f || (start_v == 0.f && half_accel < 0.f)) {
        return false;
    } else if (start_v > 0.f || (start_v == 0.f && half_accel > 0.f)) {
        return true;
    } else {
        debug_assert(start_v == 0.f && half_accel == 0.f);
        return is_state.step_dir;
    }
}

micro_move_segment_t input_shaper_pulses_t::calc_micro_move_segment(const std::array<const move_t *, INPUT_SHAPER_MAX_PULSES> &moves, const TimeTicks nearest_next_change, const uint8_t logical_axis) const {
    micro_move_segment_t segment = { 0.f, 0.f, 0.f };
    for (uint8_t pulse_idx = 0; pulse_idx < num_pulses; ++pulse_idx) {
        if (const pulse_t &pulse = pulses[pulse_idx]; pulse.a != 0.f) {
            const move_t &move = *moves[pulse_idx];
            const float start_v = float(get_move_start_v(move, logical_axis));
            const float start_pos = float(get_move_start_pos(move, logical_axis));
            const float half_accel = float(get_move_half_accel(move, logical_axis));

            // Elapsed time is relative time within the current move segment, so its values are relatively small.
            const float move_elapsed_time = (nearest_next_change - (move.print_time - pulse.t)).to_seconds_float();

            const float half_velocity_diff = half_accel * move_elapsed_time; // (1/2) * a * t
            segment.start_v += (2.f * half_velocity_diff + start_v) * pulse.a; // v0 + a * t
            segment.start_pos += ((start_v + half_velocity_diff) * move_elapsed_time + start_pos) * pulse.a; // s0 + (v0 + (1/2) * a * t) * t
            segment.half_accel += half_accel * pulse.a;
        }
    }

    return segment;
}

// If elapsed_time is too big that it cause crossing multiple time events, this function update input_shaper_state
// just for the first crossed-time event. To process all crossed-time events is required to do multiple calls.
// Reason is that too big elapsed_time is caused by the current status of input_shaper_state, so it is possible
// that processing multiple time events at once could cause that very close time event could be skipped. That
// could lead to incorrect step timing.
// Returns true if input_shaper_state was updated and false otherwise.
bool input_shaper_state_update(input_shaper_state_t &is_state, const uint8_t physical_axis) {
    debug_assert(is_state.m_logical_axis_pulses_cnt == 1 || is_state.m_logical_axis_pulses_cnt == 2);

    const uint8_t current_move_idx = is_state.m_nearest_next_change_idx;
    if (!is_state.m_is_crossing_zero_velocity) {
        if (const move_t *next_move = is_state.load_next_move_segment(current_move_idx); next_move == nullptr) {
            return false;
        }
    }

    const move_t &current_move = *is_state.m_move[current_move_idx];
    const TimeTicks nearest_next_change = is_state.m_next_change[current_move_idx];

    const input_shaper_pulses_t &first_pulses = *is_state.m_logical_axis_pulses[0];
    is_state.m_next_change[current_move_idx] = current_move.print_time + current_move.move_time - first_pulses.pulses[current_move_idx].t;

    if (is_state.m_logical_axis_pulses_cnt == 2) {
#ifdef COREXY
        const input_shaper_pulses_t &second_pulses = *is_state.m_logical_axis_pulses[1];
        debug_assert(first_pulses.pulses[current_move_idx].t == second_pulses.pulses[current_move_idx].t);
        const micro_move_segment_t &first_segment = first_pulses.calc_micro_move_segment(is_state.m_move, nearest_next_change, X_AXIS);
        const micro_move_segment_t &second_segment = second_pulses.calc_micro_move_segment(is_state.m_move, nearest_next_change, Y_AXIS);

        if (physical_axis == X_AXIS) {
            is_state.start_v = first_segment.start_v + second_segment.start_v;
            is_state.start_pos = first_segment.start_pos + second_segment.start_pos;
            is_state.half_accel = first_segment.half_accel + second_segment.half_accel;
        } else if (physical_axis == Y_AXIS) {
            is_state.start_v = first_segment.start_v - second_segment.start_v;
            is_state.start_pos = first_segment.start_pos - second_segment.start_pos;
            is_state.half_accel = first_segment.half_accel - second_segment.half_accel;
        } else {
            bsod("Invalid combination of axis and number of logical input shapers for CoreXY kinematics.");
        }
#else
        bsod("Unsupported number of logical axes for used kinematics.");
#endif

    } else {
        debug_assert(is_state.m_logical_axis_pulses_cnt == 1);

        const micro_move_segment_t &segment = is_state.m_logical_axis_pulses[0]->calc_micro_move_segment(is_state.m_move, nearest_next_change, physical_axis);
        is_state.start_v = segment.start_v;
        is_state.start_pos = segment.start_pos;
        is_state.half_accel = segment.half_accel;
    }

    is_state.m_nearest_next_change_idx = is_state.calc_nearest_next_change_idx();

    is_state.print_time = nearest_next_change;
    is_state.nearest_next_change = is_state.get_nearest_next_change();

    // Change small accelerations to zero as prevention for numeric issues.
    if (std::abs(is_state.half_accel) <= INPUT_SHAPER_ACCELERATION_EPSILON) {
        const bool start_v_prev_sign = std::signbit(is_state.start_v);
        const float move_time = (is_state.nearest_next_change - is_state.print_time).to_seconds_float();

        // Adjust start_v to compensate for zeroed acceleration.
        is_state.start_v += is_state.half_accel * move_time;
        if (std::signbit(is_state.start_v) != start_v_prev_sign) {
            // When we cross zero velocity, set the start velocity to zero.
            // Typically, when zero velocity is crossed, it will be a very small value.
            // So setting the start velocity to zero should be ok.
            is_state.start_v = 0.f;
        }

        is_state.half_accel = 0.f;
    }

    // Change small velocities to zero as prevention for numeric issues.
    if (std::abs(is_state.start_v) <= INPUT_SHAPER_VELOCITY_EPSILON) {
        is_state.start_v = 0.f;
    }

    is_state.step_dir = input_shaper_state_step_dir(is_state);
    is_state.m_is_crossing_zero_velocity = false;

    // Determine if the current micro move segment is crossing zero velocity because when zero velocity is crossed, we need to flip the step direction.
    // Zero velocity could be crossed only when start_v and half_accel have different signs and start_v isn't zero.
    if (std::signbit(is_state.start_v) != std::signbit(is_state.half_accel) && is_state.start_v != 0.f) {
        // Micro move segment is crossing zero velocity only when start_v and end_v are different.
        // Elapsed time is relative time within the current move segment, so its values are relatively small.
        const float move_time = (is_state.nearest_next_change - is_state.print_time).to_seconds_float();
        const float end_v = is_state.start_v + 2.f * is_state.half_accel * move_time;
        if (std::signbit(is_state.start_v) != std::signbit(end_v)) {
            // half_accel == ±0 cannot reach here (then end_v == start_v, failing the signbit check above).
            // Near-zero accelerations are zeroed by INPUT_SHAPER_ACCELERATION_EPSILON, so |2*half_accel| >= 0.2,
            // making the quotient bounded (~1e4 s max). from_seconds(float) isfinite assert is the backstop.
            const TimeTicks crossing = is_state.print_time + TimeTicks::from_seconds(is_state.start_v / (-2.f * is_state.half_accel));

            // We need to ensure that we set is_crossing_zero_velocity to true only when crossing
            // is bigger than is_state.print_time, otherwise, we can get stuck in the infinite loop.
            if (is_state.print_time < crossing && crossing < is_state.nearest_next_change) {
                is_state.set_nearest_next_change(crossing);

                is_state.nearest_next_change = crossing;
                is_state.m_is_crossing_zero_velocity = true;
            }
        }
    }

    return true;
}

FORCE_INLINE void input_shaper_step_generator_update(input_shaper_step_generator_t &step_generator) {
    step_generator.start_v = step_generator.is_state->start_v;
    step_generator.accel = 2.f * step_generator.is_state->half_accel;
    step_generator.start_pos = step_generator.is_state->start_pos;
    step_generator.step_dir = step_generator.is_state->step_dir;
}

step_event_info_t input_shaper_step_generator_next_step_event(input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    debug_assert(step_generator.is_state != nullptr);
    step_event_info_t next_step_event = { TimeTicks::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };

    const float half_step_dist = Planner::mm_per_half_step[step_generator.axis];
    const float next_target = float(step_generator_state.current_distance[step_generator.axis] + (step_generator.step_dir ? 0 : -1)) * Planner::mm_per_step[step_generator.axis] + half_step_dist;
    const float next_distance = next_target - step_generator.start_pos;
    const float next_distance_clamped = step_generator.step_dir ? std::max(next_distance, 0.f) : std::min(next_distance, 0.f);
    const float step_time = calc_time_for_distance(step_generator, next_distance_clamped);

    // When step_time is infinity, it means that next_distance will never be reached.
    // This happens when next_target exceeds end_position, and deceleration decelerates velocity to zero or negative value.
    // Also, we need to stop when step_time exceeds local_end.
    // isinf must short-circuit before from_seconds(float) — int64_t(inf) is UB.
    // Epsilon on the left (known-finite) side: nearest_next_change can carry huge values; never add to it.
    const TimeTicks nearest_next_change = step_generator.is_state->nearest_next_change;
    const TimeTicks print_time = step_generator.is_state->print_time;
    if (std::isinf(step_time) || (print_time + TimeTicks::from_seconds(step_time)) - EPSILON_TICKS > nearest_next_change) {
        next_step_event.time = nearest_next_change;

        if (input_shaper_state_update(*step_generator.is_state, step_generator.axis) && step_generator.is_state->nearest_next_change < MAX_PRINT_TIME_TICKS) {
            next_step_event.flags |= STEP_EVENT_FLAG_KEEP_ALIVE;
            next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_KEEP_ALIVE;
        } else {
            // We reached the ending move segment, so we will never produce any valid step event from this micro move segment.
            // When we return GENERATED_INVALID, we always have to return the value of nearest_next_change for this new micro
            // move segment and not for the previous one.
            next_step_event.time = step_generator.is_state->nearest_next_change;
        }

        input_shaper_step_generator_update(step_generator);

        // Update the direction and activity flags for the entire next move
        step_generator.move_step_flags = 0;
        step_generator.move_step_flags |= (!step_generator.step_dir) * (STEP_EVENT_FLAG_X_DIR << step_generator.axis);
        step_generator.move_step_flags |= (step_generator.start_v != 0.f || step_generator.accel != 0.f) * (STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis);

        PreciseStepping::move_segment_processed_handler();
    } else {
        next_step_event.time = print_time + TimeTicks::from_seconds(step_time);
        next_step_event.flags = STEP_EVENT_FLAG_STEP_X << step_generator.axis;
        next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_VALID;
        step_generator_state.current_distance[step_generator.axis] += (step_generator.step_dir ? 1 : -1);
    }

    return next_step_event;
}

uint8_t input_shaper_state_t::calc_nearest_next_change_idx() const {
    uint8_t min_next_change_idx = 0;
    TimeTicks min_next_change = m_next_change[0];
    for (uint8_t idx = 1; idx < m_logical_axis_pulses.front()->num_pulses; ++idx) {
        if (m_next_change[idx] < min_next_change) {
            min_next_change = m_next_change[idx];
            min_next_change_idx = idx;
        }
    }

    return min_next_change_idx;
}

void input_shaper_state_t::set_nearest_next_change(const TimeTicks new_nearest_next_change) {
    m_next_change[m_nearest_next_change_idx] = new_nearest_next_change;
}

const move_t *input_shaper_state_t::load_next_move_segment(const uint8_t m_idx) {
    const move_t *next_move = PreciseStepping::move_segment_queue_next_move(*m_move[m_idx]);
    if (next_move == nullptr) {
        return nullptr;
    }

    --m_move[m_idx]->reference_cnt;
    ++next_move->reference_cnt;

    m_move[m_idx] = next_move;

    return next_move;
}
