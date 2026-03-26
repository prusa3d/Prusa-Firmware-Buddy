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

#ifdef _DEBUG
    // There seems to be a bug in the GCC where always_inline in debug causes trendemous stack usage
    // Do just normal inline to prevent the ISR stack overflow (BFW-7496)
    #define STEPPING_INLINE inline
#else
    #define STEPPING_INLINE __attribute__((always_inline)) inline
#endif

constexpr const double EPSILON = 0.000000001;
constexpr const float EPSILON_FLOAT = 0.0000001f;

constexpr const double MAX_PRINT_TIME = 10000000.;

template <typename Type>
STEPPING_INLINE Type calc_distance(const Type start_v, const Type half_accel, const Type move_time) {
    return (start_v + half_accel * move_time) * move_time;
}

STEPPING_INLINE xyze_double_t calc_end_position(const xyze_double_t start_pos, const xyze_double_t axes_r, const double dist) {
    return start_pos + (axes_r * dist);
}

STEPPING_INLINE xyze_double_t calc_end_position(const double start_v, const double half_accel, const double move_time, const xyze_double_t start_pos, const xyze_double_t axes_r) {
    const double dist = calc_distance<double>(start_v, half_accel, move_time);
    return calc_end_position(start_pos, axes_r, dist);
}

STEPPING_INLINE xyze_double_t calc_end_position_move(const move_t *move) {
    return calc_end_position(move->start_v, move->half_accel, move->move_time, move->start_pos, move->axes_r);
}

STEPPING_INLINE float fast_sqrt(float in) {
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
STEPPING_INLINE float calc_time_for_distance(const float start_velocity, const float acceleration, const float distance, const bool step_dir) {
    if (acceleration == 0.f) {
        if (start_velocity != 0.f) {
            return distance / start_velocity;
        } else {
            return std::numeric_limits<float>::infinity();
        }
    } else if (distance == 0.f) {
        return 0.f;
    } else if (const float disc_4ac = 2.f * acceleration * distance, disc = disc_4ac + SQR(start_velocity); disc >= 0.f) {
        const float disc_sqrt = fast_sqrt(disc);
        const float term = step_dir ? (start_velocity + disc_sqrt) : (start_velocity - disc_sqrt);
        if (term != 0.f) {
            return disc_4ac / (acceleration * term);
        } else {
            // Because term == 0.f, we can exclude it from the following expression.
            return (-2.f * start_velocity) / acceleration;
        }
    } else if (disc < 0.f && disc >= -EPSILON_FLOAT) {
        return -(start_velocity / acceleration);
    } else {
        return std::numeric_limits<float>::infinity();
    }
}

STEPPING_INLINE float calc_time_for_distance(const move_segment_step_generator_t &step_generator, const float distance) {
    return std::max(calc_time_for_distance(step_generator.start_v, step_generator.accel, distance, step_generator.step_dir), 0.f);
}

STEPPING_INLINE double get_move_half_accel(const move_t &move, const int axis) {
    return move.half_accel * move.axes_r[axis];
}

STEPPING_INLINE double get_move_start_v(const move_t &move, const int axis) {
    return move.start_v * move.axes_r[axis];
}

STEPPING_INLINE double get_move_end_v(const move_t &move, const int axis) {
    return (move.start_v + 2. * move.half_accel * move.move_time) * move.axes_r[axis];
}

STEPPING_INLINE double get_move_start_pos(const move_t &move, const int axis) {
    return move.start_pos[axis];
}

STEPPING_INLINE double get_move_end_pos(const move_t &move, const int axis) {
    const double axis_r = move.axes_r[axis];
    return move.start_pos[axis] + calc_distance<double>(move.start_v * axis_r, move.half_accel * axis_r, move.move_time);
}

// True - Positive direction
// False - Negative direction
STEPPING_INLINE bool get_move_step_dir(const move_t &move, const int axis) {
    return !(move.flags & (MOVE_FLAG_X_DIR << axis));
}

// Re-insert step_event_index[0] into the remaining sorted positions [1..3].
// Unrolled for PS_AXIS_COUNT=4: eliminates lower_bound/lambda dispatch and
// the separate shift loop (which previously needed asm volatile to prevent
// a memmove call for 3 elements).
STEPPING_INLINE void step_generator_state_update_nearest_idx(step_generator_state_t &step_generator_state) {
    const step_index_t first_index = step_generator_state.step_event_index[0];
    const auto new_time = step_generator_state.step_events[first_index].time;
    step_index_t *idx = step_generator_state.step_event_index.data();

    step_index_t pos = 0;
    if (step_generator_state.step_events[idx[1]].time < new_time) {
        idx[0] = idx[1];
        pos = 1;
        if (step_generator_state.step_events[idx[2]].time < new_time) {
            idx[1] = idx[2];
            pos = 2;
            if (step_generator_state.step_events[idx[3]].time < new_time) {
                idx[2] = idx[3];
                pos = 3;
            }
        }
    }
    idx[pos] = first_index;
}

STEPPING_INLINE constexpr bool is_active_x_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_X_ACTIVE);
}

STEPPING_INLINE constexpr bool is_active_y_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_Y_ACTIVE);
}

STEPPING_INLINE constexpr bool is_active_z_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_Z_ACTIVE);
}

STEPPING_INLINE constexpr bool is_active_e_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_E_ACTIVE);
}

STEPPING_INLINE constexpr bool get_dir_x_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_X_DIR);
}

STEPPING_INLINE constexpr bool get_dir_y_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_Y_DIR);
}

STEPPING_INLINE constexpr bool get_dir_z_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_Z_DIR);
}

STEPPING_INLINE constexpr bool get_dir_e_axis(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_E_DIR);
}

STEPPING_INLINE constexpr bool is_beginning_empty_move(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_BEGINNING_EMPTY_MOVE);
}

STEPPING_INLINE constexpr bool is_ending_empty_move(const move_t &move) {
    return bool(move.flags & MOVE_FLAG_ENDING_EMPTY_MOVE);
}
