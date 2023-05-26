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
#include "input_shaper.h"

#include "../precise_stepping/precise_stepping.h"
#include "../../module/planner.h"

#include <cmath>
#include <cfloat>

input_shaper_state_t InputShaper::is_state_x;
input_shaper_pulses_t InputShaper::is_pulses_x;
input_shaper_state_t InputShaper::is_state_y;
input_shaper_pulses_t InputShaper::is_pulses_y;
float InputShaper::x_frequency = 0.0;
float InputShaper::y_frequency = 0.0;
input_shaper::Type InputShaper::x_type = input_shaper::Type::zv;
input_shaper::Type InputShaper::y_type = input_shaper::Type::zv;

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

void input_shaper_state_init(input_shaper_state_t *is_state, const input_shaper_pulses_t *is_pulses, move_t *m, int axis_idx) {
    is_state->print_time = m->print_time;

    for (size_t pulse_idx = 0; pulse_idx < size_t(is_pulses->num_pulses); ++pulse_idx) {
        is_state->m[pulse_idx] = m;
        is_state->next_change[pulse_idx] = m->print_time + m->move_t - is_pulses->pulses[pulse_idx].t;
    }

    is_state->nearest_next_change = DBL_MAX;
    for (size_t idx = 0; idx < size_t(is_pulses->num_pulses); ++idx)
        if (is_state->next_change[idx] < is_state->nearest_next_change)
            is_state->nearest_next_change = is_state->next_change[idx];

    is_state->half_accel = get_move_half_accel(m, axis_idx);
    is_state->start_v = get_move_start_v(m, axis_idx);
    is_state->start_pos = get_move_start_position(m, axis_idx);
    is_state->step_dir = get_move_step_direction(m, axis_idx);

    is_state->zero_crossing_v = false;
}

static double input_shaper_state_half_accel(input_shaper_state_t *is_state, const input_shaper_pulses_t *is_pulses, const int axis_idx) {
    double half_accel = 0.;
    for (size_t idx = 0; idx < size_t(is_pulses->num_pulses); ++idx)
        half_accel += is_pulses->pulses[idx].a * get_move_half_accel(is_state->m[idx], axis_idx);
    return half_accel;
}

static double input_shaper_state_calc_nearest_next_change(const input_shaper_state_t *is_state, const input_shaper_pulses_t *is_pulses) {
    double min_next_change = is_state->next_change[0];
    for (size_t idx = 1; idx < size_t(is_pulses->num_pulses); ++idx)
        if (min_next_change > is_state->next_change[idx])
            min_next_change = is_state->next_change[idx];

    return min_next_change;
}

static int input_shaper_state_step_dir(input_shaper_state_t *is_state) {
    // start_v and half_accel are already recomputed based on proportion, so there is no needs to handle it there.
    if (is_state->start_v == 0. && is_state->half_accel == 0.) {
        return is_state->step_dir;
    } else if (is_state->start_v < 0. || (is_state->start_v == 0. && is_state->half_accel < 0.)) {
        return 0;
    } else if (is_state->start_v > 0. || (is_state->start_v == 0. && is_state->half_accel > 0.)) {
        return 1;
    } else {
        fatal_error("Unexpected state", "input_shaper_state_step_dir");
        return is_state->step_dir;
    }
}

// If elapsed_time is too big that it cause crossing multiple time events, this function update input_shaper_state
// just for the first crossed-time event. To process all crossed-time events is required to do multiple calls.
// Reason is that too big elapsed_time is caused by the current status of input_shaper_state, so it is possible
// that processing multiple time events at once could cause that very close time event could be skipped. That
// could lead to incorrect step timing.
// Returns true if input_shaper_state was updated and false otherwise.
static bool input_shaper_state_update(input_shaper_state_t *is_state, const input_shaper_pulses_t *is_pulses, const double elapsed_time, const int axis_idx) {
    if (elapsed_time < EPSILON)
        fatal_error("Unexpected state 1", "input_shaper_state_update");

    if (elapsed_time >= is_state->nearest_next_change - EPSILON) {
        for (size_t idx = 0; idx < size_t(is_pulses->num_pulses); ++idx)
            if (is_state->nearest_next_change >= is_state->next_change[idx] - EPSILON)
                if (move_t *next_move = PreciseStepping::move_segment_queue_next_move(*is_state->m[idx]); next_move == nullptr)
                    return false;
    }

    double current_print_time = 0.;
    double velocity_discontinuity = 0.;

    if (elapsed_time >= is_state->nearest_next_change - EPSILON) {
        for (size_t idx = 0; idx < size_t(is_pulses->num_pulses); ++idx) {
            if (is_state->nearest_next_change >= is_state->next_change[idx] - EPSILON) {
                current_print_time = is_state->next_change[idx];

                if (!is_state->zero_crossing_v) {
                    const double end_v = get_move_start_v(is_state->m[idx], axis_idx) + 2 * get_move_half_accel(is_state->m[idx], axis_idx) * is_state->m[idx]->move_t;

                    move_t *next_move = PreciseStepping::move_segment_queue_next_move(*is_state->m[idx]);
                    if (idx == 0) {
                        --is_state->m[idx]->reference_cnt;
                        ++next_move->reference_cnt;
                    }
                    is_state->m[idx] = next_move;

                    const double diff_v = get_move_start_v(is_state->m[idx], axis_idx) - end_v;
                    if (diff_v < -EPSILON || diff_v > EPSILON)
                        velocity_discontinuity += diff_v * is_pulses->pulses[idx].a;
                } else
                    is_state->zero_crossing_v = false;

                is_state->next_change[idx] = is_state->m[idx]->print_time + is_state->m[idx]->move_t - is_pulses->pulses[idx].t;
            }
        }
    }

    if (current_print_time > 0.) {
        const double move_elapsed_time = (current_print_time - is_state->print_time);

        if (is_state->m[0] == is_state->m[is_pulses->num_pulses - 1]) {
            // At the end of the input shaping segment, recompute start_v, start_pos, and half_accel to suppress accumulated errors.
            const double move_time_pos = (current_print_time - is_state->m[0]->print_time); // Time withing the current move.

            // TODO Lukas H.: Check if this is correct.
            //            // If acceleration is zero, then ve could compute position exactly without accumulated error.
            //            if (get_move_half_accel(is_state->m[0], axis_idx) >= -EPSILON && get_move_half_accel(is_state->m[0], axis_idx) <= EPSILON)
            //                is_state->start_pos = get_move_start_position(is_state->m[0], axis_idx) + get_move_start_v(is_state->m[0], axis_idx) * move_time_pos;
            //            else
            is_state->start_pos += (is_state->start_v + is_state->half_accel * move_elapsed_time) * move_elapsed_time;

            is_state->start_v = get_move_start_v(is_state->m[0], axis_idx) + 2. * get_move_half_accel(is_state->m[0], axis_idx) * move_time_pos;
            is_state->half_accel = get_move_half_accel(is_state->m[0], axis_idx);
        } else {
            const double velocity_tmp = is_state->half_accel * move_elapsed_time;
            is_state->start_pos += (is_state->start_v + velocity_tmp) * move_elapsed_time;
            is_state->start_v += 2. * velocity_tmp + velocity_discontinuity;
            is_state->half_accel = input_shaper_state_half_accel(is_state, is_pulses, axis_idx);

            // Change small velocities to zero as prevention for numeric issues.
            if (is_state->start_v >= -INPUT_SHAPER_VELOCITY_EPSILON && is_state->start_v <= INPUT_SHAPER_VELOCITY_EPSILON)
                is_state->start_v = 0.;

            // Change small accelerations to zero as prevention for numeric issues.
            if (is_state->half_accel >= -INPUT_SHAPER_ACCELERATION_EPSILON && is_state->half_accel <= INPUT_SHAPER_ACCELERATION_EPSILON)
                is_state->half_accel = 0.;
        }

        is_state->step_dir = input_shaper_state_step_dir(is_state);
        is_state->print_time = current_print_time;
        is_state->nearest_next_change = input_shaper_state_calc_nearest_next_change(is_state, is_pulses);

        // Determine if the current move segment after applying the input shaper is crossing through zero velocity (changing the direction of stepper motors).
        {
            const double move_t = is_state->nearest_next_change - is_state->print_time;
            const double end_v = is_state->start_v + 2. * is_state->half_accel * move_t;
            if ((is_state->start_v > 0. && end_v < 0.) || (is_state->start_v < 0. && end_v > 0.)) {
                const double zero_crossing_t = is_state->start_v / (-2 * is_state->half_accel) + is_state->print_time;

                size_t min_idx = 0;
                double min_value = is_state->next_change[0];

                for (size_t idx = 1; idx < size_t(is_pulses->num_pulses); ++idx) {
                    if (is_state->next_change[idx] < min_value) {
                        min_value = is_state->next_change[idx];
                        min_idx = idx;
                    }
                }

                if (zero_crossing_t < is_state->next_change[min_idx]) {
                    is_state->next_change[min_idx] = zero_crossing_t;
                    is_state->nearest_next_change = zero_crossing_t;
                    is_state->zero_crossing_v = true;
                }
            }
        }

        return true;
    }

    return false;
}

static float calc_time_for_distance_input_shaper(const input_shaper_state_t *is_state, const float distance) {
    if (float(is_state->half_accel) == 0.f && float(is_state->start_v) == 0.f)
        return NAN;

    const int step_dir = is_state->step_dir;
    if (step_dir)
        return calc_time_for_distance(float(is_state->start_v), 2.f * float(is_state->half_accel), distance);
    else
        return calc_time_for_distance(float(-is_state->start_v), 2.f * -float(is_state->half_accel), -distance);
}

step_event_info_t input_shaper_step_generator_next_step_event(input_shaper_step_generator_t *step_generator, step_generator_state_t &step_generator_state, const double flush_time) {
    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0 };
    int is_update_state = 0;
    do {
        const bool step_dir = step_generator->is_state->step_dir;
        const float half_step_dist = Planner::mm_per_step[step_generator->axis] / 2.f;
        const double next_target = step_generator_state.current_distance[step_generator->axis] + (step_dir ? half_step_dist : -half_step_dist);
        const double start_pos = step_generator->is_state->start_pos;
        const double next_distance = next_target - start_pos;
        const double step_time = calc_time_for_distance_input_shaper(step_generator->is_state, next_distance);

        // When step_time is NaN, it means that next_distance will never be reached.
        // This happens when next_target exceeds end_position, and deceleration decelerates velocity to zero or negative value.
        // Also, we need to stop when step_time exceeds local_end.
        // Be aware that testing, if flush_time was exceeded, has to be after testing for exceeding print_time.
        const double elapsed_time = step_time + step_generator->is_state->print_time;
        if (isnan(step_time) || elapsed_time > (step_generator->is_state->nearest_next_change + EPSILON)) {
            is_update_state = input_shaper_state_update(step_generator->is_state, step_generator->is_pulses, step_generator->is_state->nearest_next_change, step_generator->axis);

            if (!is_update_state)
                step_generator->reached_end_of_move_queue = true;

            // Update step direction flag, which is cached until this move segment is processed.
            const uint16_t current_axis_dir_flag = (STEP_EVENT_FLAG_X_DIR << step_generator->axis);
            step_generator_state.flags &= ~current_axis_dir_flag;
            step_generator_state.flags |= (!step_generator->is_state->step_dir) * current_axis_dir_flag;

            // Update active axis flag, which is cached until this move segment is processed.
            const uint16_t current_axis_active_flag = (STEP_EVENT_FLAG_X_ACTIVE << step_generator->axis);
            step_generator_state.flags &= ~current_axis_active_flag;
            step_generator_state.flags |= (step_generator->is_state->start_v != 0. || step_generator->is_state->half_accel != 0.) * current_axis_active_flag;

            PreciseStepping::move_segment_processed_handler();
        } else if (elapsed_time > flush_time) {
            step_generator->reached_end_of_move_queue = true;
            break;
        } else {
            next_step_event.time = elapsed_time;
            next_step_event.flags = STEP_EVENT_FLAG_STEP_X << step_generator->axis;
            next_step_event.flags |= step_generator_state.flags;
            step_generator_state.current_distance[step_generator->axis] = next_target + (step_dir ? half_step_dist : -half_step_dist);
            break;
        }
    } while (is_update_state != 0);

    return next_step_event;
}
