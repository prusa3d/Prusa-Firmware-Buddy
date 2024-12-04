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
    double sum_a = 0.;
    for (int i = 0; i < num_pulses; ++i) {
        sum_a += a[i];
    }

    // Reverse pulses vs their traditional definition
    const double inv_sum_a = 1. / sum_a;
    for (int i = 0; i < num_pulses; ++i) {
        is_pulses->pulses[num_pulses - i - 1].a = a[i] * inv_sum_a;
        is_pulses->pulses[num_pulses - i - 1].t = -t[i];
    }

    double time_shift = 0.;
    for (int i = 0; i < num_pulses; ++i) {
        time_shift += is_pulses->pulses[i].a * is_pulses->pulses[i].t;
    }

    // Shift pulses around mid-point.
    for (int i = 0; i < num_pulses; ++i) {
        is_pulses->pulses[i].t -= time_shift;
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

input_shaper_pulses_t create_zv_input_shaper_pulses(const float shaper_freq, const float damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0.f, input_shaper::Type::zv);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_zvd_input_shaper_pulses(const float shaper_freq, const float damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0.f, input_shaper::Type::zvd);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_mzv_input_shaper_pulses(const float shaper_freq, const float damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0.f, input_shaper::Type::mzv);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_ei_input_shaper_pulses(const float shaper_freq, const float damping_ratio, const float vibration_reduction) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, vibration_reduction, input_shaper::Type::ei);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_2hump_ei_input_shaper_pulses(const float shaper_freq, const float damping_ratio, const float vibration_reduction) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, vibration_reduction, input_shaper::Type::ei_2hump);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_3hump_ei_input_shaper_pulses(const float shaper_freq, const float damping_ratio, const float vibration_reduction) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, vibration_reduction, input_shaper::Type::ei_3hump);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_null_input_shaper_pulses() {
    input_shaper::Shaper shaper = input_shaper::get(NAN, NAN, NAN, input_shaper::Type::null);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

void input_shaper_step_generator_init(const move_t &move, input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    const uint8_t axis = step_generator.axis;
    assert(axis == X_AXIS || axis == Y_AXIS || axis == Z_AXIS);
    input_shaper_state_t *const is_state = step_generator.is_state;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)input_shaper_step_generator_next_step_event;

    // Set the initial direction and activity flags for the entire next move
    step_generator.move_step_flags = 0;
    step_generator.move_step_flags |= (!is_state->step_dir) * (STEP_EVENT_FLAG_X_DIR << axis);
    step_generator.move_step_flags |= (is_state->start_v != 0. || is_state->half_accel != 0.) * (STEP_EVENT_FLAG_X_ACTIVE << axis);

#ifdef COREXY
    if (axis == X_AXIS || axis == Y_AXIS) {
        is_state->m_axis_shaper[0].m_pulses = &InputShaper::logical_axis_pulses[X_AXIS];
        is_state->m_axis_shaper[1].m_pulses = &InputShaper::logical_axis_pulses[Y_AXIS];
    } else {
        bsod("Unsupported axis");
    }
#else
    if (axis == X_AXIS || axis == Y_AXIS || axis == Z_AXIS) {
        is_state->m_axis_shaper[0].m_pulses = &InputShaper::logical_axis_pulses[axis];
    } else {
        bsod("Unsupported axis");
    }
#endif

    for (const logical_axis_input_shaper_t &axis_shaper : is_state->m_axis_shaper) {
        move.reference_cnt += axis_shaper.m_pulses->num_pulses;
    }

    input_shaper_state_init(*is_state, move, axis);
    input_shaper_step_generator_update(step_generator);
}

void input_shaper_state_init(input_shaper_state_t &is_state, const move_t &move, uint8_t axis) {
    assert(is_beginning_empty_move(move));
    assert(move.print_time == 0.);

    is_state.print_time = move.print_time;

#ifdef COREXY
    for (uint8_t logical_axis = X_AXIS; logical_axis <= Y_AXIS; ++logical_axis) {
        is_state.m_axis_shaper[logical_axis].init(move, logical_axis);
    }
#else
    is_state.m_axis_shaper[0].init(move, axis);
#endif

#ifdef COREXY
    is_state.nearest_next_change = std::min(is_state.m_axis_shaper[0].get_nearest_next_change(), is_state.m_axis_shaper[1].get_nearest_next_change());
#else
    is_state.nearest_next_change = is_state.m_axis_shaper[0].get_nearest_next_change();
#endif

    is_state.half_accel = get_move_half_accel(move, axis);
    is_state.start_v = get_move_start_v(move, axis);

#ifdef COREXY
    if (axis == A_AXIS) {
        is_state.start_pos = move.start_pos.x + move.start_pos.y;
    } else if (axis == B_AXIS) {
        is_state.start_pos = move.start_pos.x - move.start_pos.y;
    } else {
        fatal_error("Invalid axis", "input_shaper_state_init");
    }
#else
    is_state.start_pos = get_move_start_pos(move, axis);
#endif

    is_state.step_dir = get_move_step_dir(move, axis);

    is_state.is_crossing_zero_velocity = false;
}

static bool input_shaper_state_step_dir(input_shaper_state_t &is_state) {
    // Previously we ensured that none of micro move segments would cross zero velocity.
    // So this function can correctly determine step_dir only when this assumption is met.

    // Convert doubles to floats to perform comparisons in float instead doubles.
    float start_v = float(is_state.start_v);
    float half_accel = float(is_state.half_accel);

    if (start_v < 0.f || (start_v == 0.f && half_accel < 0.f)) {
        return false;
    } else if (start_v > 0.f || (start_v == 0.f && half_accel > 0.f)) {
        return true;
    } else {
        assert(start_v == 0.f && is_state.half_accel == 0.f);
        return is_state.step_dir;
    }
}

bool logical_axis_input_shaper_t::update(const input_shaper_state_t &axis_is) {
    const uint8_t curr_idx = m_nearest_next_change_idx;
    const double nearest_next_change = m_next_change[curr_idx];

    // Weighted (by input shaper coefficient) discontinuity in ending and starting velocity between two succeeding move segments.
    double weighted_velocity_discontinuity = 0.;
    if (!axis_is.is_crossing_zero_velocity) {
        const move_t *curr_move = m_move[curr_idx];
        const move_t *next_move = this->load_next_move_segment(curr_idx);
        if (next_move == nullptr) {
            return false;
        }

        weighted_velocity_discontinuity += (get_move_start_v(*next_move, m_axis) - get_move_end_v(*curr_move, m_axis)) * m_pulses->pulses[curr_idx].a;
    }

    const move_t *curr_move = m_move[curr_idx];
    m_next_change[curr_idx] = curr_move->print_time + curr_move->move_time - m_pulses->pulses[curr_idx].t;

    const double move_elapsed_time = (nearest_next_change - m_print_time);
    if (const move_t *first_move = m_move[0], *last_move = m_move[m_pulses->num_pulses - 1]; first_move == last_move) {
        // When all pointers point on the same move segment (iff the first and the last pointers point on the same move segment),
        // then we can compute start_v, start_pos, and half_accel without accumulation, so we can suppress accumulated errors.
        const double move_time = (nearest_next_change - first_move->print_time); // Mapped nearest_next_change into current move segment.
        const double start_v = get_move_start_v(*first_move, m_axis);
        const double half_accel = get_move_half_accel(*first_move, m_axis);
        assert(move_time >= 0. && move_time < first_move->move_time);

        // After applying the input shape filter, the velocity during the acceleration or deceleration phase doesn't equal the velocity of the input move segment at the same time.
        // Because of that also, the position will not equal, so during the acceleration or deceleration phase, we cannot compute start_pos just from the input move segment.
        // We can compute start_pos from the input move segment (without accumulated error) just for the cruise phase where velocity is constant.
        if (std::abs(half_accel) <= EPSILON) {
            m_start_pos = get_move_start_pos(*first_move, m_axis) + start_v * move_time;
        } else {
            m_start_pos += (m_start_v + m_half_accel * move_elapsed_time) * move_elapsed_time;
        }

        m_start_v = start_v + 2. * half_accel * move_time;
        m_half_accel = half_accel;
    } else {
        const double half_velocity_diff = m_half_accel * move_elapsed_time; // (1/2) * a * t
        m_start_pos += (m_start_v + half_velocity_diff) * move_elapsed_time; // (v0 + (1/2) * a * t) * t
        m_start_v += 2. * half_velocity_diff + weighted_velocity_discontinuity; // a * t
        m_half_accel = this->calc_half_accel();

        // Change small accelerations to zero as prevention for numeric issues.
        if (std::abs(m_half_accel) <= INPUT_SHAPER_ACCELERATION_EPSILON) {
            const bool start_v_prev_sign = std::signbit(m_start_v);

            // Adjust start_v to compensate for zeroed acceleration.
            m_start_v += m_half_accel * move_elapsed_time;
            if (std::signbit(m_start_v) != start_v_prev_sign) {
                // When we cross zero velocity, set the start velocity to zero.
                // Typically, when zero velocity is crossed, it will be a very small value.
                // So setting the start velocity to zero should be ok.
                m_start_v = 0.f;
            }

            m_half_accel = 0.;
        }

        // Change small velocities to zero as prevention for numeric issues.
        if (std::abs(m_start_v) <= INPUT_SHAPER_VELOCITY_EPSILON) {
            m_start_v = 0.;
        }
    }

    m_print_time = nearest_next_change;
    m_nearest_next_change_idx = this->calc_nearest_next_change_idx();
    return true;
}

// If elapsed_time is too big that it cause crossing multiple time events, this function update input_shaper_state
// just for the first crossed-time event. To process all crossed-time events is required to do multiple calls.
// Reason is that too big elapsed_time is caused by the current status of input_shaper_state, so it is possible
// that processing multiple time events at once could cause that very close time event could be skipped. That
// could lead to incorrect step timing.
// Returns true if input_shaper_state was updated and false otherwise.
bool input_shaper_state_update(input_shaper_state_t &is_state, const int axis) {
#ifdef COREXY
    bool x_updated = false;
    bool y_updated = false;
    if (is_state.is_crossing_zero_velocity || is_state.nearest_next_change == is_state.m_axis_shaper[0].get_nearest_next_change()) {
        if (!is_state.m_axis_shaper[0].update(is_state)) {
            return false;
        } else {
            x_updated = true;
        }
    }

    if (is_state.is_crossing_zero_velocity || is_state.nearest_next_change == is_state.m_axis_shaper[1].get_nearest_next_change()) {
        if (!is_state.m_axis_shaper[1].update(is_state)) {
            return false;
        } else {
            y_updated = true;
        }
    }

#else
    if (!is_state.m_axis_shaper[0].update(is_state)) {
        return false;
    }
#endif

    if (is_state.is_crossing_zero_velocity) {
        is_state.is_crossing_zero_velocity = false;
    }

#ifdef COREXY
    double x_start_v = is_state.m_axis_shaper[0].m_start_v;
    double y_start_v = is_state.m_axis_shaper[1].m_start_v;
    double x_start_pos = is_state.m_axis_shaper[0].m_start_pos;
    double y_start_pos = is_state.m_axis_shaper[1].m_start_pos;

    const double x_move_time = is_state.nearest_next_change - is_state.m_axis_shaper[0].m_print_time;
    const double y_move_time = is_state.nearest_next_change - is_state.m_axis_shaper[1].m_print_time;

    // If just one logical axis input shaper was updated, then we need to calculate new start_pos and start_v for the other logical axis input shaper.
    if (x_updated && !y_updated) {
        y_start_pos = y_start_pos + (y_start_v + is_state.m_axis_shaper[1].m_half_accel * y_move_time) * y_move_time;
        y_start_v = y_start_v + 2. * is_state.m_axis_shaper[1].m_half_accel * y_move_time;
    } else if (y_updated && !x_updated) {
        x_start_pos = x_start_pos + (x_start_v + is_state.m_axis_shaper[0].m_half_accel * x_move_time) * x_move_time;
        x_start_v = x_start_v + 2. * is_state.m_axis_shaper[0].m_half_accel * x_move_time;
    }

    if (axis == A_AXIS) {
        is_state.start_v = x_start_v + y_start_v;
        is_state.start_pos = x_start_pos + y_start_pos;
        is_state.half_accel = is_state.m_axis_shaper[0].m_half_accel + is_state.m_axis_shaper[1].m_half_accel;
    } else if (axis == B_AXIS) {
        is_state.start_v = x_start_v - y_start_v;
        is_state.start_pos = x_start_pos - y_start_pos;
        is_state.half_accel = is_state.m_axis_shaper[0].m_half_accel - is_state.m_axis_shaper[1].m_half_accel;
    } else {
        fatal_error("Invalid axis", "input_shaper_state_update");
    }

    // Change small accelerations to zero as prevention for numeric issues.
    if (std::abs(is_state.half_accel) <= INPUT_SHAPER_ACCELERATION_EPSILON) {
        const bool start_v_prev_sign = std::signbit(is_state.start_v);

        // Adjust start_v to compensate for zeroed acceleration.
        is_state.start_v += is_state.half_accel * (is_state.nearest_next_change - is_state.print_time);
        if (std::signbit(is_state.start_v) != start_v_prev_sign) {
            // When we cross zero velocity, set the start velocity to zero.
            // Typically, when zero velocity is crossed, it will be a very small value.
            // So setting the start velocity to zero should be ok.
            is_state.start_v = 0.f;
        }

        is_state.half_accel = 0.;
    }

    // Change small velocities to zero as prevention for numeric issues.
    if (std::abs(is_state.start_v) <= INPUT_SHAPER_VELOCITY_EPSILON) {
        is_state.start_v = 0.;
    }

    is_state.step_dir = input_shaper_state_step_dir(is_state);
    is_state.print_time = is_state.nearest_next_change;
    is_state.nearest_next_change = std::min(is_state.m_axis_shaper[0].m_next_change[is_state.m_axis_shaper[0].m_nearest_next_change_idx], is_state.m_axis_shaper[1].m_next_change[is_state.m_axis_shaper[1].m_nearest_next_change_idx]);
#else
    is_state.start_v = is_state.m_axis_shaper[0].m_start_v;
    is_state.half_accel = is_state.m_axis_shaper[0].m_half_accel;
    is_state.start_pos = is_state.m_axis_shaper[0].m_start_pos;
    is_state.step_dir = input_shaper_state_step_dir(is_state);
    is_state.print_time = is_state.m_axis_shaper[0].m_print_time;
    is_state.nearest_next_change = is_state.m_axis_shaper[0].m_next_change[is_state.m_axis_shaper[0].m_nearest_next_change_idx];
#endif

    // Determine if the current micro move segment is crossing zero velocity because when zero velocity is crossed, we need to flip the step direction.
    // Zero velocity could be crossed only when start_v and half_accel have different signs and start_v isn't zero.
    if (std::signbit(is_state.start_v) != std::signbit(is_state.half_accel) && is_state.start_v != 0.) {
        // Micro move segment is crossing zero velocity only when start_v and end_v are different.
        // Division in doubles is quite an expensive operation, so it is much cheaper to make these pre-checks instead of checks based on the computed time of crossing the zero velocity.
        const double move_time = is_state.nearest_next_change - is_state.print_time;
        const double end_v = is_state.start_v + 2. * is_state.half_accel * move_time;
        if (std::signbit(is_state.start_v) != std::signbit(end_v)) {
            const double zero_velocity_crossing_time_absolute = is_state.start_v / (-2. * is_state.half_accel) + is_state.print_time;

            if (zero_velocity_crossing_time_absolute < (is_state.nearest_next_change - EPSILON)) {
                for (logical_axis_input_shaper_t &axis_shaper : is_state.m_axis_shaper) {
                    axis_shaper.set_nearest_next_change(zero_velocity_crossing_time_absolute);
                }

                is_state.nearest_next_change = zero_velocity_crossing_time_absolute;
                is_state.is_crossing_zero_velocity = true;
            }
        }
    }

    return true;
}

FORCE_INLINE void input_shaper_step_generator_update(input_shaper_step_generator_t &step_generator) {
    step_generator.start_v = float(step_generator.is_state->start_v);
    step_generator.accel = 2.f * float(step_generator.is_state->half_accel);
    step_generator.start_pos = float(step_generator.is_state->start_pos);
    step_generator.step_dir = step_generator.is_state->step_dir;
}

step_event_info_t input_shaper_step_generator_next_step_event(input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state) {
    assert(step_generator.is_state != nullptr);
    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };

    const float half_step_dist = Planner::mm_per_half_step[step_generator.axis];
    const float next_target = float(step_generator_state.current_distance[step_generator.axis] + (step_generator.step_dir ? 0 : -1)) * Planner::mm_per_step[step_generator.axis] + half_step_dist;
    const float next_distance = next_target - step_generator.start_pos;
    const float step_time = calc_time_for_distance(step_generator, next_distance);

    // When step_time is infinity, it means that next_distance will never be reached.
    // This happens when next_target exceeds end_position, and deceleration decelerates velocity to zero or negative value.
    // Also, we need to stop when step_time exceeds local_end.
    if (const double elapsed_time = double(step_time) + step_generator.is_state->print_time; elapsed_time > (step_generator.is_state->nearest_next_change + EPSILON)) {
        next_step_event.time = step_generator.is_state->nearest_next_change;

        if (input_shaper_state_update(*step_generator.is_state, step_generator.axis) && step_generator.is_state->nearest_next_change < MAX_PRINT_TIME) {
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
        next_step_event.time = elapsed_time;
        next_step_event.flags = STEP_EVENT_FLAG_STEP_X << step_generator.axis;
        next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_VALID;
        step_generator_state.current_distance[step_generator.axis] += (step_generator.step_dir ? 1 : -1);
    }

    return next_step_event;
}

void logical_axis_input_shaper_t::init(const move_t &move, uint8_t axis) {
    assert(m_pulses->num_pulses > 0);

    for (uint8_t pulse_idx = 0; pulse_idx < m_pulses->num_pulses; ++pulse_idx) {
        m_move[pulse_idx] = &move;
        m_next_change[pulse_idx] = move.print_time + move.move_time - m_pulses->pulses[pulse_idx].t;
    }

    m_half_accel = get_move_half_accel(move, axis);
    m_start_v = get_move_start_v(move, axis);
    m_start_pos = get_move_start_pos(move, axis);
    m_axis = axis;
    m_print_time = move.print_time;
    m_nearest_next_change_idx = this->calc_nearest_next_change_idx();
}

uint8_t logical_axis_input_shaper_t::calc_nearest_next_change_idx() const {
    uint8_t min_next_change_idx = 0;
    double min_next_change = m_next_change[0];
    for (uint8_t idx = 1; idx < m_pulses->num_pulses; ++idx) {
        if (m_next_change[idx] < min_next_change) {
            min_next_change = m_next_change[idx];
            min_next_change_idx = idx;
        }
    }

    return min_next_change_idx;
}

double logical_axis_input_shaper_t::calc_half_accel() const {
    double half_accel = 0.;
    for (uint8_t idx = 0; idx < m_pulses->num_pulses; ++idx) {
        half_accel += m_pulses->pulses[idx].a * get_move_half_accel(*m_move[idx], m_axis);
    }
    return half_accel;
}

void logical_axis_input_shaper_t::set_nearest_next_change(const double new_nearest_next_change) {
    m_next_change[m_nearest_next_change_idx] = new_nearest_next_change;
}

const move_t *logical_axis_input_shaper_t::load_next_move_segment(const uint8_t m_idx) {
    const move_t *next_move = PreciseStepping::move_segment_queue_next_move(*m_move[m_idx]);
    if (next_move == nullptr) {
        return nullptr;
    }

    --m_move[m_idx]->reference_cnt;
    ++next_move->reference_cnt;

    m_move[m_idx] = next_move;

    return next_move;
}
