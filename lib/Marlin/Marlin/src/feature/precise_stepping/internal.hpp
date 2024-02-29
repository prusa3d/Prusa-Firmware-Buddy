/**
 * Based on the implementation in Klipper [https://github.com/Klipper3d/klipper].
 * Copyright (C) Kevin O'Connor <kevin@koconnor.net>
 *
 * Our implementation takes inspiration from the work of Kevin O'Connor <kevin@koconnor.net> for Klipper
 * in used data structures, and some computations.
 */
#pragma once
#include "common.hpp"
#include "cmath_ext.h"

// #define ISR_DEADLINE_DEBUGGING // Enable audible warnings on step deadline misses
// #define ISR_DEADLINE_TRACKING // Accurate (but expensive) deadline miss tracking
// #define ISR_EVENT_DEBUGGING // Enable audible warnings on event queue misses
// #define ISR_DEBUG_NOOPT // Do not optimize ISR functions for debugging

#ifdef ISR_DEADLINE_DEBUGGING
    #warning "Dedline detection isn't working as intended after PreciseStepping::step_isr() was rewroted."
#endif

#ifndef ISR_DEBUG_NOOPT
    #define FORCE_OFAST __attribute__((optimize("-Ofast")))
#else
    #define FORCE_OFAST // no-op
#endif

constexpr const double EPSILON = 0.000000001;
constexpr const float EPSILON_FLOAT = 0.0000001f;

constexpr const double MAX_PRINT_TIME = 10000000.;

template <typename Type>
FORCE_INLINE Type calc_distance(const Type start_v, const Type half_accel, const Type move_time) {
    return (start_v + half_accel * move_time) * move_time;
}

FORCE_INLINE xyze_double_t calc_end_position(const xyze_double_t start_pos, const xyze_double_t axes_r, const double dist) {
    return start_pos + (axes_r * dist);
}

FORCE_INLINE xyze_double_t calc_end_position(const double start_v, const double half_accel, const double move_time, const xyze_double_t start_pos, const xyze_double_t axes_r) {
    const double dist = calc_distance<double>(start_v, half_accel, move_time);
    return calc_end_position(start_pos, axes_r, dist);
}

FORCE_INLINE xyze_double_t calc_end_position_move(const move_t *move) {
    return calc_end_position(move->start_v, move->half_accel, move->move_time, move->start_pos, move->axes_r);
}

FORCE_INLINE float fast_sqrt(float in) {
    float out;
#if defined(__GNUC__) && defined(__VFP_FP__) && !defined(__SOFTFP__)
    asm("vsqrt.f32 %0,%1"
        : "=t"(out)
        : "t"(in));
#else
    out = sqrtf(in);
#endif
    return out;
}

// Calculates time that will take to travel the specified distance.
// Step_dir determines which solution of the quadratic equation we will choose.
FORCE_INLINE float calc_time_for_distance(const float start_velocity, const float acceleration, const float distance, const bool step_dir) {
    if (acceleration == 0.f) {
        if (start_velocity != 0.f) {
            return distance / start_velocity;
        } else {
            return std::numeric_limits<float>::infinity();
        }
    } else if (const float sqr = 2.f * acceleration * distance + SQR(start_velocity); sqr >= 0.f) {
        if (step_dir) {
            return (fast_sqrt(sqr) - start_velocity) / acceleration;
        } else {
            return (-fast_sqrt(sqr) - start_velocity) / acceleration;
        }
    } else if (sqr < 0.f && sqr >= -EPSILON_FLOAT) {
        return -(start_velocity / acceleration);
    } else {
        return std::numeric_limits<float>::infinity();
    }
}

FORCE_INLINE double get_move_half_accel(const move_t &move, const int axis) {
    return move.half_accel * move.axes_r[axis];
}

FORCE_INLINE double get_move_start_v(const move_t &move, const int axis) {
    return move.start_v * move.axes_r[axis];
}

FORCE_INLINE double get_move_end_v(const move_t &move, const int axis) {
    return (move.start_v + 2. * move.half_accel * move.move_time) * move.axes_r[axis];
}

FORCE_INLINE double get_move_start_pos(const move_t &move, const int axis) {
    return move.start_pos[axis];
}

FORCE_INLINE double get_move_end_pos(const move_t &move, const int axis) {
    const double axis_r = move.axes_r[axis];
    return move.start_pos[axis] + calc_distance<double>(move.start_v * axis_r, move.half_accel * axis_r, move.move_time);
}

// True - Positive direction
// False - Negative direction
FORCE_INLINE bool get_move_step_dir(const move_t &move, const int axis) {
    return !(move.flags & (MOVE_FLAG_X_DIR << axis));
}

// Update the step event index after updating the first entry (oldest) to keep it sorted.
FORCE_INLINE void step_generator_state_update_nearest_idx(step_generator_state_t &step_generator_state) {
    // index and time of the new event
    auto first_index = step_generator_state.step_event_index[0];
    const auto new_time = step_generator_state.step_events[first_index].time;

    // find insertion position
    auto lb = std::lower_bound(step_generator_state.step_event_index.begin() + 1, step_generator_state.step_event_index.end(),
        new_time, [&](auto a, auto b) { return step_generator_state.step_events[a].time < b; });
    step_index_t insert_pos = lb - step_generator_state.step_event_index.begin() - 1;

    // move previous positions
    for (step_index_t n = 0; n != insert_pos; ++n) {
        asm volatile(""); // prevent call to memmove
        step_generator_state.step_event_index[n] = step_generator_state.step_event_index[n + 1];
    }

    // update
    step_generator_state.step_event_index[insert_pos] = first_index;
}

FORCE_INLINE constexpr bool is_active_x_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_X_ACTIVE);
}

FORCE_INLINE constexpr bool is_active_y_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_Y_ACTIVE);
}

FORCE_INLINE constexpr bool is_active_z_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_Z_ACTIVE);
}

FORCE_INLINE constexpr bool is_active_e_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_E_ACTIVE);
}

FORCE_INLINE constexpr bool get_dir_x_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_X_DIR);
}

FORCE_INLINE constexpr bool get_dir_y_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_Y_DIR);
}

FORCE_INLINE constexpr bool get_dir_z_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_Z_DIR);
}

FORCE_INLINE constexpr bool get_dir_e_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_E_DIR);
}

FORCE_INLINE constexpr bool is_beginning_empty_move(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_BEGINNING_EMPTY_MOVE);
}

FORCE_INLINE constexpr bool is_ending_empty_move(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_ENDING_EMPTY_MOVE);
}
