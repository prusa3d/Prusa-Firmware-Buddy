/**
 * Based on the implementation in Klipper [https://github.com/Klipper3d/klipper].
 * Copyright (C) Kevin O'Connor <kevin@koconnor.net>
 *
 * Our implementation takes inspiration from the work of Kevin O'Connor <kevin@koconnor.net> for Klipper
 * in used data structures, and some computations.
 */
#pragma once
#include "../../inc/MarlinConfig.h"
#include "../../core/types.h"
#include "cmath_ext.h"

//#define FAIL_ON_NEGATIVE_STEP_TIME
//#define ISR_DEADLINE_DEBUGGING // Enable audible warnings on step deadline misses
//#define ISR_DEADLINE_TRACKING // Accurate (but expensive) deadline miss tracking
//#define ISR_EVENT_DEBUGGING // Enable audible warnings on event queue misses

constexpr const double EPSILON = 0.000000001;
constexpr const float EPSILON_FLOAT = 0.0000001f;

constexpr const double MAX_PRINT_TIME = 10000000.;

#define MOVE_SEGMENT_QUEUE_MOD(n) ((n) & (MOVE_SEGMENT_QUEUE_SIZE - 1))
#define STEP_EVENT_QUEUE_MOD(n)   ((n) & (STEP_EVENT_QUEUE_SIZE - 1))

#define MOVE_FLAG_DIR_MASK         (0x00F0)
#define MOVE_FLAG_AXIS_ACTIVE_MASK (0x0F00)

#define MOVE_FLAG_DIR_SHIFT         (4)
#define MOVE_FLAG_AXIS_ACTIVE_SHIFT (8)

struct pressure_advance_state_t;
struct input_shaper_state_t;
struct input_shaper_pulses_t;

typedef uint16_t MoveFlag_t;
enum MoveFlag : MoveFlag_t {
    MOVE_FLAG_ACCELERATION_PHASE = _BV(0),
    MOVE_FLAG_DECELERATION_PHASE = _BV(1),
    MOVE_FLAG_CRUISE_PHASE = _BV(2),

    MOVE_FLAG_X_DIR = _BV(4),
    MOVE_FLAG_Y_DIR = _BV(5),
    MOVE_FLAG_Z_DIR = _BV(6),
    MOVE_FLAG_E_DIR = _BV(7),

    MOVE_FLAG_X_ACTIVE = _BV(8),
    MOVE_FLAG_Y_ACTIVE = _BV(9),
    MOVE_FLAG_Z_ACTIVE = _BV(10),
    MOVE_FLAG_E_ACTIVE = _BV(11),

    MOVE_FLAG_FIRST_MOVE_SEGMENT_OF_BLOCK = _BV(12), // Indicates if this move is the first move of the associated trapezoid block.
    MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK = _BV(13),  // Indicates if this move is the last move of the associated trapezoid block.

    MOVE_FLAG_BEGINNING_EMPTY_MOVE = _BV(14), // Indicates if this move is the beginning empty move.
    MOVE_FLAG_ENDING_EMPTY_MOVE = _BV(15)     // Indicates if this move is the ending empty move.
};

typedef struct move_t {
    double start_v;
    double half_accel;
    double move_t;
    double print_time;

    xyze_double_t axes_r;
    xyze_double_t start_pos;

    MoveFlag_t flags;
    // Number of step event generators that are using/referencing this move segment.
    int16_t reference_cnt = 0;
} move_t;

#define STEP_EVENT_FLAG_AXIS_MASK        (0x000Fu)
#define STEP_EVENT_FLAG_DIR_MASK         (0x00F0u)
#define STEP_EVENT_FLAG_AXIS_ACTIVE_MASK (0x0F00u)
#define STEP_EVENT_FLAG_AXIS_OTHER_MASK  (0xF000u)

#define STEP_EVENT_FLAG_AXIS_SHIFT        (0)
#define STEP_EVENT_FLAG_DIR_SHIFT         (4)
#define STEP_EVENT_FLAG_AXIS_ACTIVE_SHIFT (8)
#define STEP_EVENT_FLAG_AXIS_OTHER_SHIFT  (12)

typedef uint16_t StepEventFlag_t;
enum StepEventFlag : StepEventFlag_t {
    STEP_EVENT_FLAG_STEP_X = _BV(0),
    STEP_EVENT_FLAG_STEP_Y = _BV(1),
    STEP_EVENT_FLAG_STEP_Z = _BV(2),
    STEP_EVENT_FLAG_STEP_E = _BV(3),

    STEP_EVENT_FLAG_X_DIR = _BV(4),
    STEP_EVENT_FLAG_Y_DIR = _BV(5),
    STEP_EVENT_FLAG_Z_DIR = _BV(6),
    STEP_EVENT_FLAG_E_DIR = _BV(7),

    STEP_EVENT_FLAG_X_ACTIVE = _BV(8),
    STEP_EVENT_FLAG_Y_ACTIVE = _BV(9),
    STEP_EVENT_FLAG_Z_ACTIVE = _BV(10),
    STEP_EVENT_FLAG_E_ACTIVE = _BV(11),

    STEP_EVENT_FLAG_BEGINNING_OF_MOVE_SEGMENT = _BV(12), // Indicate that this step event is the first step event from a move segment.
    STEP_EVENT_END_OF_MOTION = _BV(13),                  // Last event before coming to a halt
};

typedef struct step_event_t {
    int32_t time_ticks;
    StepEventFlag_t flags;
} step_event_t;

// Circular queue for move segments.
// head == tail              : the queue is empty
// head != tail              : moves segments are in the queue
// head == (tail -1 ) % size : the queue is full
typedef struct move_segment_queue_t {
    move_t data[MOVE_SEGMENT_QUEUE_SIZE];
    volatile uint8_t tail = 0;
    volatile uint8_t head = 0;
    volatile uint8_t unprocessed = 0; // Index of the first unprocessed move segment by PreciseStepping::process_queue_of_move_segments().
} move_segment_queue_t;

// Circular queue for step events.
// head == tail              : the queue is empty
// head != tail              : step events are in the queue
// head == (tail - 1) % size : the queue is full
typedef struct step_event_queue_t {
    step_event_t data[STEP_EVENT_QUEUE_SIZE];
    volatile uint16_t tail = 0;
    volatile uint16_t head = 0;
} step_event_queue_t;

FORCE_INLINE double calc_distance(const double start_v, const double half_accel, const double move_time) {
    return (start_v + half_accel * move_time) * move_time;
}

FORCE_INLINE xyze_double_t calc_position(const double start_v, const double half_accel, const double move_time, const xyze_double_t start_pos, const xyze_double_t axes_r) {
    const double distance = calc_distance(start_v, half_accel, move_time);
    const xyze_double_t end_pos = start_pos + xyze_double_t({ axes_r.x * distance, axes_r.y * distance, axes_r.z * distance, axes_r.e * distance });
    return end_pos;
}

FORCE_INLINE xyze_double_t calc_end_position_move(const move_t *move) {
    return calc_position(move->start_v, move->half_accel, move->move_t, move->start_pos, move->axes_r);
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
FORCE_INLINE float calc_time_for_distance(const float start_velocity, const float acceleration, const float distance) {
    if (acceleration == 0.f)
        return distance / start_velocity;

    // Preventing computing sqrt from negative numbers caused by numerical issues.
    const float sqr = 2.f * acceleration * distance + SQR(start_velocity);
    if (sqr < 0.f && sqr >= -EPSILON_FLOAT)
        return -(start_velocity / acceleration);

    return (fast_sqrt(sqr) - start_velocity) / acceleration;
}

// Calculates time that will take to travel the specified distance.
FORCE_INLINE float calc_time_for_distance_block(const float start_velocity, const float acceleration, const float distance) {
    if (acceleration == 0.f)
        return distance / start_velocity;

    // Preventing computing sqrt from negative numbers caused by numerical issues.
    if (const float sqr = 2.f * acceleration * distance + SQR(start_velocity); sqr < 0.f)
        return -start_velocity / acceleration;
    else
        return (fast_sqrt(sqr) - start_velocity) / acceleration;
}

FORCE_INLINE float calc_time_for_distance_move(const move_t *move, const float distance, const int axis) {
    // Special cases
    if (float(move->axes_r[axis]) == 0.f || (float(move->half_accel) == 0.f && float(move->start_v) == 0.f))
        return NAN;

    // We don't need to know direction in axis just its proportion.
    const float axes_r = float(move->axes_r[axis]);
    const float abs_axes_r = axes_r >= 0.f ? axes_r : -axes_r;
    const float abs_distance = distance >= 0.f ? distance : -distance;
    const float start_v = float(move->start_v) * abs_axes_r;
    const float accel = 2.f * float(move->half_accel) * abs_axes_r;
    return calc_time_for_distance(start_v, accel, abs_distance);
}

FORCE_INLINE double get_move_half_accel(const move_t *move, const int axis_idx) {
    return move->half_accel * move->axes_r[axis_idx];
}

FORCE_INLINE double get_move_start_v(const move_t *move, const int axis_idx) {
    return move->start_v * move->axes_r[axis_idx];
}

FORCE_INLINE double get_move_start_position(const move_t *move, const int axis_idx) {
    return move->start_pos[axis_idx];
}

// 1 - For positive direction
// 0 - For negative direction
FORCE_INLINE int get_move_step_direction(const move_t *move, const int axis_idx) {
    return move->start_v * move->axes_r[axis_idx] >= 0.;
}

enum StepGeneratorType : uint8_t {
    CLASSIC_STEP_GENERATOR_X = 0,
    CLASSIC_STEP_GENERATOR_Y = 0,
    CLASSIC_STEP_GENERATOR_Z = 0,
    CLASSIC_STEP_GENERATOR_E = 0,

    INPUT_SHAPER_STEP_GENERATOR_X = _BV(0),
    INPUT_SHAPER_STEP_GENERATOR_Y = _BV(1),

    PRESSURE_ADVANCE_STEP_GENERATOR_E = _BV(3)
};

typedef struct step_event_info_t {
    double time;
    StepEventFlag_t flags;
} step_event_info_t;

enum StepGeneratorStatus : uint8_t {
    STEP_GENERATOR_STATUS_OK = 0,
    STEP_GENERATOR_STATUS_NO_STEP_EVENT_PRODUCED = 1,
    STEP_GENERATOR_STATUS_FULL_STEP_EVENT_QUEUE = 2
};

typedef struct basic_step_generator_t {
    int axis = -1;
    bool reached_end_of_move_queue = false;
} basic_step_generator_t;

typedef struct step_generator_state_t {
    basic_step_generator_t *step_generator[4];
    step_event_info_t step_events[4];
    size_t nearest_step_event_idx;
    double previous_step_time;

    StepEventFlag_t flags;

    xyze_double_t current_distance;

    // It represents the maximum value of how far in the time can some step event generators point.
    // Used for computing flush time that ensures that none of the step event generators will produce step
    // event beyond the flush time. Because for the same state of the move segment queue, some step
    // event generator could generate step events far away from others, which could let to incorrect
    // ordering of step events.
    double max_lookback_time;

    // Number of markers indicating the start of move segments that need to be inserted into step events.
    // Be aware that very short move segments could produce just one single step event or none step event
    // for some generators. And that is why we have to use integer instead of bool because we could need
    // to produce more step events with a mark right after each other.
    int8_t left_insert_start_of_move_segment;

    bool initialized;
} step_generator_state_t;

typedef struct classic_step_generator_t : basic_step_generator_t {
    move_t *current_move = nullptr;
} classic_step_generator_t;

typedef struct input_shaper_step_generator_t : basic_step_generator_t {
    input_shaper_state_t *is_state = nullptr;
    input_shaper_pulses_t *is_pulses = nullptr;
} input_shaper_step_generator_t;

typedef struct pressure_advance_step_generator_t : basic_step_generator_t {
    pressure_advance_state_t *pa_state = nullptr;
} pressure_advance_step_generator_t;

typedef struct step_generators_pool_t {
    classic_step_generator_t classic_step_generator_x;
    classic_step_generator_t classic_step_generator_y;
    classic_step_generator_t classic_step_generator_z;
    classic_step_generator_t classic_step_generator_e;

    input_shaper_step_generator_t input_shaper_step_generator_x;
    input_shaper_step_generator_t input_shaper_step_generator_y;

    pressure_advance_step_generator_t pressure_advance_step_generator_e;
} step_generators_pool_t;

// Reset all step_event that are equal to std::numeric_limits<float>::max() to zero.
// std::numeric_limits<float>::max() indicates that all step events for the given move and its axis were already generated.
FORCE_INLINE void step_generator_state_restart(step_generator_state_t *step_generator_state) {
    for (step_event_info_t &step_event_info : step_generator_state->step_events)
        if (step_event_info.time == std::numeric_limits<double>::max())
            step_event_info.time = 0;
}

// Find the next the nearest step event index.
// Current implementation is iteration through all axis, so it could be better to have a tiny linked list of sorted step events
// and iterate through this linked list. It could be much faster in most cases.
FORCE_INLINE void step_generator_state_update_nearest_idx(step_generator_state_t *step_generator_state, double next_step_event_time) {
    for (int idx = 0; idx < 4; ++idx)
        if (step_generator_state->step_events[idx].time < next_step_event_time) {
            next_step_event_time = step_generator_state->step_events[idx].time;
            step_generator_state->nearest_step_event_idx = idx;
        }
}

// Returns if all generators reach the end of the move queue and are unable to generate next-step events
// that means that the step queue will start draining.
FORCE_INLINE bool has_all_generators_reached_end_of_move_queue(const step_generator_state_t &step_generator_state) {
    bool all_generators_reached_end_of_move_queue = true;
    for (const basic_step_generator_t *step_generator : step_generator_state.step_generator)
        all_generators_reached_end_of_move_queue &= step_generator->reached_end_of_move_queue;

    return all_generators_reached_end_of_move_queue;
}

// Reset reached_end_of_move_queue flag for all step event generators.
FORCE_INLINE void reset_reached_end_of_move_queue_flag(const step_generator_state_t &step_generator_state) {
    for (basic_step_generator_t *step_generator : step_generator_state.step_generator)
        step_generator->reached_end_of_move_queue = false;
}

FORCE_INLINE constexpr bool is_active_x_axis(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_X_ACTIVE);
}

FORCE_INLINE constexpr bool is_active_y_axis(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_Y_ACTIVE);
}

FORCE_INLINE constexpr bool is_active_z_axis(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_Z_ACTIVE);
}

FORCE_INLINE constexpr bool is_active_e_axis(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_E_ACTIVE);
}

FORCE_INLINE constexpr bool get_dir_x_axis(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_X_DIR);
}
FORCE_INLINE constexpr bool get_dir_y_axis(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_Y_DIR);
}
FORCE_INLINE constexpr bool get_dir_z_axis(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_Z_DIR);
}
FORCE_INLINE constexpr bool get_dir_e_axis(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_E_DIR);
}

FORCE_INLINE constexpr bool is_beginning_empty_move(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_BEGINNING_EMPTY_MOVE);
}

FORCE_INLINE constexpr bool is_ending_empty_move(const move_t &m) {
    return bool(m.flags & MOVE_FLAG_ENDING_EMPTY_MOVE);
}
