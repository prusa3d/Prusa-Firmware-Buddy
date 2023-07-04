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
#include "input_shaper_config.hpp"
#include "eeprom_journal/store_instance.hpp"

#include <cmath>
#include <cfloat>

input_shaper_state_t InputShaper::is_state[3];
input_shaper_pulses_t InputShaper::is_pulses[3];

static void init_input_shaper_pulses(const double a[], const double t[], const int num_pulses, input_shaper_pulses_t *is_pulses) {
    double sum_a = 0.;
    for (int i = 0; i < num_pulses; ++i)
        sum_a += a[i];

    // Reverse pulses vs their traditional definition
    const double inv_sum_a = 1. / sum_a;
    for (int i = 0; i < num_pulses; ++i) {
        is_pulses->pulses[num_pulses - i - 1].a = a[i] * inv_sum_a;
        is_pulses->pulses[num_pulses - i - 1].t = -t[i];
    }

    double time_shift = 0.;
    for (int i = 0; i < num_pulses; ++i)
        time_shift += is_pulses->pulses[i].a * is_pulses->pulses[i].t;

    // Shift pulses around mid-point.
    for (int i = 0; i < num_pulses; ++i)
        is_pulses->pulses[i].t -= time_shift;

    is_pulses->num_pulses = num_pulses;
}

input_shaper::Shaper input_shaper::get(const double damping_ratio, const double shaper_freq, const double vibration_reduction, const input_shaper::Type type) {
    switch (type) {
    case Type::zv: {
        constexpr int num_pulses = 2;
        const double df = std::sqrt(1. - std::pow(damping_ratio, 2.));
        const double K = std::exp(-damping_ratio * M_PI / df);
        const double t_d = 1. / (shaper_freq * df);

        const double a[num_pulses] = { 1., K };
        const double t[num_pulses] = { 0., .5 * t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::zvd: {
        constexpr int num_pulses = 3;
        const double df = std::sqrt(1. - std::pow(damping_ratio, 2.));
        const double K = std::exp(-damping_ratio * M_PI / df);
        const double t_d = 1. / (shaper_freq * df);

        const double a[num_pulses] = { 1., 2. * K, std::pow(K, 2) };
        const double t[num_pulses] = { 0., .5 * t_d, t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::mzv: {
        constexpr int num_pulses = 3;
        const double df = std::sqrt(1. - std::pow(damping_ratio, 2.));
        const double K = std::exp(-.75 * damping_ratio * M_PI / df);
        const double t_d = 1. / (shaper_freq * df);

        const double a1 = 1. - 1. / std::sqrt(2.);
        const double a2 = (std::sqrt(2.) - 1.) * K;
        const double a3 = a1 * K * K;

        const double a[num_pulses] = { a1, a2, a3 };
        const double t[num_pulses] = { 0., .375 * t_d, .75 * t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::ei: {
        constexpr int num_pulses = 3;
        const double v_tol = 1. / vibration_reduction; // vibration tolerance
        const double df = std::sqrt(1. - std::pow(damping_ratio, 2.));
        const double K = std::exp(-damping_ratio * M_PI / df);
        const double t_d = 1. / (shaper_freq * df);

        const double a1 = .25 * (1. + v_tol);
        const double a2 = .5 * (1. - v_tol) * K;
        const double a3 = a1 * K * K;

        double a[num_pulses] = { a1, a2, a3 };
        double t[num_pulses] = { 0., .5 * t_d, t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::ei_2hump: {
        constexpr int num_pulses = 4;
        const double v_tol = 1. / vibration_reduction; // vibration tolerance
        const double df = std::sqrt(1. - std::pow(damping_ratio, 2.));
        const double K = std::exp(-damping_ratio * M_PI / df);
        const double t_d = 1. / (shaper_freq * df);
        const double V2 = std::pow(v_tol, 2.);
        const double X = std::pow(V2 * (std::sqrt(1. - V2) + 1.), 1. / 3.);

        const double a1 = (3. * X * X + 2. * X + 3. * V2) / (16. * X);
        const double a2 = (.5 - a1) * K;
        const double a3 = a2 * K;
        const double a4 = a1 * K * K * K;

        double a[num_pulses] = { a1, a2, a3, a4 };
        double t[num_pulses] = { 0., .5 * t_d, t_d, 1.5 * t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::ei_3hump: {
        constexpr int num_pulses = 5;
        const double v_tol = 1. / vibration_reduction; // vibration tolerance
        const double df = std::sqrt(1. - std::pow(damping_ratio, 2.));
        const double K = std::exp(-damping_ratio * M_PI / df);
        const double t_d = 1. / (shaper_freq * df);
        const double K2 = K * K;

        const double a1 = 0.0625 * (1. + 3. * v_tol + 2. * std::sqrt(2. * (v_tol + 1.) * v_tol));
        const double a2 = 0.25 * (1. - v_tol) * K;
        const double a3 = (0.5 * (1. + v_tol) - 2. * a1) * K2;
        const double a4 = a2 * K2;
        const double a5 = a1 * K2 * K2;

        double a[num_pulses] = { a1, a2, a3, a4, a5 };
        double t[num_pulses] = { 0., .5 * t_d, t_d, 1.5 * t_d, 2. * t_d };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    case Type::null: {
        constexpr int num_pulses = 1;
        double a[num_pulses] = { 1. };
        double t[num_pulses] = { 0. };

        input_shaper::Shaper shaper(a, t, num_pulses);
        return shaper;
    }
    }
    bsod("input_shaper::Type out of range");
}

input_shaper_pulses_t create_zv_input_shaper_pulses(const double shaper_freq, const double damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0., input_shaper::Type::zv);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_zvd_input_shaper_pulses(const double shaper_freq, const double damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0., input_shaper::Type::zvd);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_mzv_input_shaper_pulses(const double shaper_freq, const double damping_ratio) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, 0., input_shaper::Type::mzv);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_ei_input_shaper_pulses(const double shaper_freq, const double damping_ratio, const double vibration_reduction) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, vibration_reduction, input_shaper::Type::ei);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_2hump_ei_input_shaper_pulses(const double shaper_freq, const double damping_ratio, const double vibration_reduction) {
    input_shaper::Shaper shaper = input_shaper::get(damping_ratio, shaper_freq, vibration_reduction, input_shaper::Type::ei_2hump);
    input_shaper_pulses_t is_pulses;
    init_input_shaper_pulses(shaper.a, shaper.t, shaper.num_pulses, &is_pulses);
    return is_pulses;
}

input_shaper_pulses_t create_3hump_ei_input_shaper_pulses(const double shaper_freq, const double damping_ratio, const double vibration_reduction) {
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
    input_shaper_state_t *const is_state = step_generator.is_state;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)input_shaper_step_generator_next_step_event;

    input_shaper_state_init(*is_state, *step_generator.is_pulses, move, axis);

    step_generator_state.flags |= (!is_state->step_dir) * (STEP_EVENT_FLAG_X_DIR << axis);
    step_generator_state.flags |= (is_state->start_v != 0. || is_state->half_accel != 0.) * (STEP_EVENT_FLAG_X_ACTIVE << axis);
    move.reference_cnt += step_generator.is_pulses->num_pulses;

    input_shaper_step_generator_update(step_generator);
}

void input_shaper_state_init(input_shaper_state_t &is_state, const input_shaper_pulses_t &is_pulses, const move_t &move, uint8_t axis) {
    assert(is_beginning_empty_move(move));
    assert(is_pulses.num_pulses > 0);
    assert(move.print_time == 0.);

    is_state.print_time = move.print_time;

    for (uint8_t pulse_idx = 0; pulse_idx < is_pulses.num_pulses; ++pulse_idx) {
        is_state.m[pulse_idx] = &move;
        is_state.next_change[pulse_idx] = move.print_time + move.move_t - is_pulses.pulses[pulse_idx].t;
    }

    std::tie(is_state.nearest_next_change, is_state.nearest_next_change_idx) = input_shaper_state_calc_nearest_next_change(is_state, is_pulses);

    is_state.half_accel = get_move_half_accel(move, axis);
    is_state.start_v = get_move_start_v(move, axis);
    is_state.start_pos = get_move_start_pos(move, axis);
    is_state.step_dir = get_move_step_dir(move, axis);

    is_state.is_crossing_zero_velocity = false;
}

static double input_shaper_state_half_accel(const input_shaper_state_t &is_state, const input_shaper_pulses_t &is_pulses, const int axis) {
    double half_accel = 0.;
    for (uint8_t idx = 0; idx < is_pulses.num_pulses; ++idx)
        half_accel += is_pulses.pulses[idx].a * get_move_half_accel(*is_state.m[idx], axis);
    return half_accel;
}

FORCE_INLINE std::pair<double, uint8_t> input_shaper_state_calc_nearest_next_change(const input_shaper_state_t &is_state, const input_shaper_pulses_t &is_pulses) {
    uint8_t min_next_change_idx = 0;
    double min_next_change = is_state.next_change[0];
    for (uint8_t idx = 1; idx < is_pulses.num_pulses; ++idx) {
        if (is_state.next_change[idx] < min_next_change) {
            min_next_change = is_state.next_change[idx];
            min_next_change_idx = idx;
        }
    }

    return { min_next_change, min_next_change_idx };
}

static int input_shaper_state_step_dir(input_shaper_state_t &is_state) {
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

// If elapsed_time is too big that it cause crossing multiple time events, this function update input_shaper_state
// just for the first crossed-time event. To process all crossed-time events is required to do multiple calls.
// Reason is that too big elapsed_time is caused by the current status of input_shaper_state, so it is possible
// that processing multiple time events at once could cause that very close time event could be skipped. That
// could lead to incorrect step timing.
// Returns true if input_shaper_state was updated and false otherwise.
static bool input_shaper_state_update(input_shaper_state_t &is_state, const input_shaper_pulses_t &is_pulses, const int axis) {
    const uint8_t curr_idx = is_state.nearest_next_change_idx;

    // Weighted (by input shaper coefficient) discontinuity in ending and starting velocity between two succeeding move segments.
    double weighted_velocity_discontinuity = 0.;
    if (!is_state.is_crossing_zero_velocity) {
        const move_t *curr_move = is_state.m[curr_idx];
        const move_t *next_move = is_state.load_next_move_segment(curr_idx);
        if (next_move == nullptr)
            return false;

        weighted_velocity_discontinuity += (get_move_start_v(*next_move, axis) - get_move_end_v(*curr_move, axis)) * is_pulses.pulses[curr_idx].a;
    } else
        is_state.is_crossing_zero_velocity = false;

    const move_t *curr_move = is_state.m[curr_idx];
    is_state.next_change[curr_idx] = curr_move->print_time + curr_move->move_t - is_pulses.pulses[curr_idx].t;

    const double move_elapsed_time = (is_state.nearest_next_change - is_state.print_time);
    if (const move_t *first_move = is_state.m[0], *last_move = is_state.m[is_pulses.num_pulses - 1]; first_move == last_move) {
        // When all pointers point on the same move segment (iff the first and the last pointers point on the same move segment),
        // then we can compute start_v, start_pos, and half_accel without accumulation, so we can suppress accumulated errors.
        const double move_t = (is_state.nearest_next_change - first_move->print_time); // Mapped nearest_next_change into current move segment.
        const double start_v = get_move_start_v(*first_move, axis);
        const double half_accel = get_move_half_accel(*first_move, axis);
        assert(move_t >= 0. && move_t < first_move->move_t);

        // After applying the input shape filter, the velocity during the acceleration or deceleration phase doesn't equal the velocity of the input move segment at the same time.
        // Because of that also, the position will not equal, so during the acceleration or deceleration phase, we cannot compute start_pos just from the input move segment.
        // We can compute start_pos from the input move segment (without accumulated error) just for the cruise phase where velocity is constant.
        if (std::abs(half_accel) <= EPSILON)
            is_state.start_pos = get_move_start_pos(*first_move, axis) + start_v * move_t;
        else
            is_state.start_pos += (is_state.start_v + is_state.half_accel * move_elapsed_time) * move_elapsed_time;

        is_state.start_v = start_v + 2. * half_accel * move_t;
        is_state.half_accel = half_accel;
    } else {
        const double half_velocity_diff = is_state.half_accel * move_elapsed_time;         // (1/2) * a * t
        is_state.start_pos += (is_state.start_v + half_velocity_diff) * move_elapsed_time; // (v0 + (1/2) * a * t) * t
        is_state.start_v += 2. * half_velocity_diff + weighted_velocity_discontinuity;     // a * t
        is_state.half_accel = input_shaper_state_half_accel(is_state, is_pulses, axis);

        // Change small velocities to zero as prevention for numeric issues.
        if (std::abs(is_state.start_v) <= INPUT_SHAPER_VELOCITY_EPSILON)
            is_state.start_v = 0.;

        // Change small accelerations to zero as prevention for numeric issues.
        if (std::abs(is_state.half_accel) <= INPUT_SHAPER_ACCELERATION_EPSILON)
            is_state.half_accel = 0.;
    }

    is_state.step_dir = input_shaper_state_step_dir(is_state);
    is_state.print_time = is_state.nearest_next_change;
    std::tie(is_state.nearest_next_change, is_state.nearest_next_change_idx) = input_shaper_state_calc_nearest_next_change(is_state, is_pulses);

    // Determine if the current micro move segment is crossing zero velocity because when zero velocity is crossed, we need to flip the step direction.
    // Zero velocity could be crossed only when start_v and half_accel have different signs and start_v isn't zero.
    if (std::signbit(is_state.start_v) != std::signbit(is_state.half_accel) && is_state.start_v != 0.) {
        // Micro move segment is crossing zero velocity only when start_v and end_v are different.
        // Division in doubles is quite an expensive operation, so it is much cheaper to make these pre-checks instead of checks based on the computed time of crossing the zero velocity.
        const double move_t = is_state.nearest_next_change - is_state.print_time;
        const double end_v = is_state.start_v + 2. * is_state.half_accel * move_t;
        if (std::signbit(is_state.start_v) != std::signbit(end_v)) {
            const double zero_velocity_crossing_time_absolute = is_state.start_v / (-2. * is_state.half_accel) + is_state.print_time;

            if (zero_velocity_crossing_time_absolute < (is_state.nearest_next_change - EPSILON)) {
                is_state.next_change[is_state.nearest_next_change_idx] = zero_velocity_crossing_time_absolute;
                is_state.nearest_next_change = zero_velocity_crossing_time_absolute;
                is_state.is_crossing_zero_velocity = true;
            }
        }
    }

    return true;
}

FORCE_INLINE float calc_time_for_distance(const input_shaper_step_generator_t &step_generator, const float distance) {
    return calc_time_for_distance(step_generator.start_v, step_generator.accel, distance, step_generator.is_state->step_dir);
}

FORCE_INLINE void input_shaper_step_generator_update(input_shaper_step_generator_t &step_generator) {
    step_generator.start_v = float(step_generator.is_state->start_v);
    step_generator.accel = 2.f * float(step_generator.is_state->half_accel);
    step_generator.start_pos = float(step_generator.is_state->start_pos);
    step_generator.step_dir = step_generator.is_state->step_dir;
}

step_event_info_t input_shaper_step_generator_next_step_event(input_shaper_step_generator_t &step_generator, step_generator_state_t &step_generator_state, const double flush_time) {
    assert(step_generator.is_state != nullptr && step_generator.is_pulses != nullptr);
    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0 };
    int is_update_state = 0;
    do {
        const bool step_dir = step_generator.is_state->step_dir;
        const float half_step_dist = Planner::mm_per_half_step[step_generator.axis];
        const float current_distance = float(step_generator_state.current_distance[step_generator.axis]) * Planner::mm_per_step[step_generator.axis];
        const float next_target = current_distance + (step_dir ? half_step_dist : -half_step_dist);
        const float next_distance = next_target - step_generator.start_pos;
        const float step_time = calc_time_for_distance(step_generator, next_distance);

        // When step_time is NaN, it means that next_distance will never be reached.
        // This happens when next_target exceeds end_position, and deceleration decelerates velocity to zero or negative value.
        // Also, we need to stop when step_time exceeds local_end.
        // Be aware that testing, if flush_time was exceeded, has to be after testing for exceeding print_time.
        const double elapsed_time = double(step_time) + step_generator.is_state->print_time;
        if (isnan(step_time) || elapsed_time > (step_generator.is_state->nearest_next_change + EPSILON)) {
            is_update_state = input_shaper_state_update(*step_generator.is_state, *step_generator.is_pulses, step_generator.axis);

            if (!is_update_state)
                step_generator.reached_end_of_move_queue = true;

            // Update step direction flag, which is cached until this move segment is processed.
            const uint16_t current_axis_dir_flag = (STEP_EVENT_FLAG_X_DIR << step_generator.axis);
            step_generator_state.flags &= ~current_axis_dir_flag;
            step_generator_state.flags |= (!step_generator.is_state->step_dir) * current_axis_dir_flag;

            // Update active axis flag, which is cached until this move segment is processed.
            const uint16_t current_axis_active_flag = (STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis);
            step_generator_state.flags &= ~current_axis_active_flag;
            step_generator_state.flags |= (step_generator.is_state->start_v != 0. || step_generator.is_state->half_accel != 0.) * current_axis_active_flag;

            input_shaper_step_generator_update(step_generator);
            PreciseStepping::move_segment_processed_handler();
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
    } while (is_update_state != 0);

    return next_step_event;
}

const move_t *input_shaper_state_t::load_next_move_segment(const uint8_t m_idx) {
    const move_t *next_move = PreciseStepping::move_segment_queue_next_move(*m[m_idx]);
    if (next_move == nullptr)
        return nullptr;

    --m[m_idx]->reference_cnt;
    ++next_move->reference_cnt;

    m[m_idx] = next_move;

    return next_move;
}
