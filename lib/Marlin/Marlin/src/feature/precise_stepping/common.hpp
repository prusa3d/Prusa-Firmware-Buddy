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

#define MOVE_SEGMENT_QUEUE_MOD(n) ((n) & (MOVE_SEGMENT_QUEUE_SIZE - 1))
#define STEP_EVENT_QUEUE_MOD(n)   ((n) & (STEP_EVENT_QUEUE_SIZE - 1))

constexpr const uint32_t MOVE_FLAG_DIR_MASK = 0x00F0;
constexpr const uint32_t MOVE_FLAG_AXIS_ACTIVE_MASK = 0x0F00;
constexpr const uint32_t MOVE_FLAG_RESET_POSITION_MASK = 0x000F0000;

constexpr const uint32_t MOVE_FLAG_DIR_SHIFT = 4;
constexpr const uint32_t MOVE_FLAG_AXIS_ACTIVE_SHIFT = 8;
constexpr const uint32_t MOVE_FLAG_RESET_POSITION_SHIFT = 16;

struct pressure_advance_state_t;
struct input_shaper_state_t;
struct input_shaper_pulses_t;

typedef uint32_t MoveFlag_t;
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
    MOVE_FLAG_LAST_MOVE_SEGMENT_OF_BLOCK = _BV(13), // Indicates if this move is the last move of the associated trapezoid block.

    MOVE_FLAG_BEGINNING_EMPTY_MOVE = _BV(14), // Indicates if this move is the beginning empty move.
    MOVE_FLAG_ENDING_EMPTY_MOVE = _BV(15), // Indicates if this move is the ending empty move.

    // Indicated that position of the axis should be reset to zero.
    MOVE_FLAG_RESET_POSITION_X = _BV(16),
    MOVE_FLAG_RESET_POSITION_Y = _BV(17),
    MOVE_FLAG_RESET_POSITION_Z = _BV(18),
    MOVE_FLAG_RESET_POSITION_E = _BV(19),
};

// Ensure XYZE bits are always adjacent and ordered.
static_assert(MoveFlag::MOVE_FLAG_RESET_POSITION_Y == (MoveFlag::MOVE_FLAG_RESET_POSITION_X << 1));
static_assert(MoveFlag::MOVE_FLAG_RESET_POSITION_Z == (MoveFlag::MOVE_FLAG_RESET_POSITION_X << 2));
static_assert(MoveFlag::MOVE_FLAG_RESET_POSITION_E == (MoveFlag::MOVE_FLAG_RESET_POSITION_X << 3));

// Verify shifts.
static_assert(MoveFlag::MOVE_FLAG_X_DIR == (1ul << MOVE_FLAG_DIR_SHIFT));
static_assert(MoveFlag::MOVE_FLAG_X_ACTIVE == (1ul << MOVE_FLAG_AXIS_ACTIVE_SHIFT));
static_assert(MoveFlag::MOVE_FLAG_RESET_POSITION_X == (1ul << MOVE_FLAG_RESET_POSITION_SHIFT));

typedef struct move_t {
    double start_v;
    double half_accel;
    double move_t;
    double print_time;

    xyze_double_t axes_r;
    xyze_double_t start_pos;

    MoveFlag_t flags;
    // Number of step event generators that are using/referencing this move segment.
    mutable uint8_t reference_cnt = 0;
} move_t;

constexpr const uint16_t STEP_EVENT_FLAG_AXIS_MASK = 0x000Fu;
constexpr const uint16_t STEP_EVENT_FLAG_DIR_MASK = 0x00F0u;
constexpr const uint16_t STEP_EVENT_FLAG_AXIS_ACTIVE_MASK = 0x0F00u;
constexpr const uint16_t STEP_EVENT_FLAG_AXIS_OTHER_MASK = 0xF000u;

constexpr const uint16_t STEP_EVENT_FLAG_AXIS_SHIFT = 0;
constexpr const uint16_t STEP_EVENT_FLAG_DIR_SHIFT = 4;
constexpr const uint16_t STEP_EVENT_FLAG_AXIS_ACTIVE_SHIFT = 8;
constexpr const uint16_t STEP_EVENT_FLAG_AXIS_OTHER_SHIFT = 12;

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
    STEP_EVENT_END_OF_MOTION = _BV(13), // Last event before coming to a halt
};

// Ensure XYZE bits are always adjacent and ordered as required in most loops
static_assert(StepEventFlag::STEP_EVENT_FLAG_STEP_Y == (StepEventFlag::STEP_EVENT_FLAG_STEP_X << 1));
static_assert(StepEventFlag::STEP_EVENT_FLAG_STEP_Z == (StepEventFlag::STEP_EVENT_FLAG_STEP_X << 2));
static_assert(StepEventFlag::STEP_EVENT_FLAG_STEP_E == (StepEventFlag::STEP_EVENT_FLAG_STEP_X << 3));
static_assert(StepEventFlag::STEP_EVENT_FLAG_Y_DIR == (StepEventFlag::STEP_EVENT_FLAG_X_DIR << 1));
static_assert(StepEventFlag::STEP_EVENT_FLAG_Z_DIR == (StepEventFlag::STEP_EVENT_FLAG_X_DIR << 2));
static_assert(StepEventFlag::STEP_EVENT_FLAG_E_DIR == (StepEventFlag::STEP_EVENT_FLAG_X_DIR << 3));
static_assert(StepEventFlag::STEP_EVENT_FLAG_Y_ACTIVE == (StepEventFlag::STEP_EVENT_FLAG_X_ACTIVE << 1));
static_assert(StepEventFlag::STEP_EVENT_FLAG_Z_ACTIVE == (StepEventFlag::STEP_EVENT_FLAG_X_ACTIVE << 2));
static_assert(StepEventFlag::STEP_EVENT_FLAG_E_ACTIVE == (StepEventFlag::STEP_EVENT_FLAG_X_ACTIVE << 3));

// Verify shifts
static_assert(StepEventFlag::STEP_EVENT_FLAG_STEP_X == (1u << STEP_EVENT_FLAG_AXIS_SHIFT));
static_assert(StepEventFlag::STEP_EVENT_FLAG_X_DIR == (1u << STEP_EVENT_FLAG_DIR_SHIFT));
static_assert(StepEventFlag::STEP_EVENT_FLAG_X_ACTIVE == (1u << STEP_EVENT_FLAG_AXIS_ACTIVE_SHIFT));

struct step_event_i32_t {
    int32_t time_ticks;
    StepEventFlag_t flags;
};

// Used by step event queue. So, the maximum time difference between step events is 2^16 / STEPPER_TIMER_RATE,
// which is currently 65.536ms.
// Probably, we will almost never have a step event with such a time difference. But when those step events occur,
// they are split into several step events.
struct step_event_u16_t {
    uint16_t time_ticks;
    StepEventFlag_t flags;
};

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
    step_event_u16_t data[STEP_EVENT_QUEUE_SIZE];
    volatile uint16_t tail = 0;
    volatile uint16_t head = 0;
} step_event_queue_t;

enum StepGeneratorType : uint8_t {
    CLASSIC_STEP_GENERATOR_X = 0,
    CLASSIC_STEP_GENERATOR_Y = 0,
    CLASSIC_STEP_GENERATOR_Z = 0,
    CLASSIC_STEP_GENERATOR_E = 0,

    INPUT_SHAPER_STEP_GENERATOR_X = _BV(0),
    INPUT_SHAPER_STEP_GENERATOR_Y = _BV(1),
    INPUT_SHAPER_STEP_GENERATOR_Z = _BV(2),

    PRESSURE_ADVANCE_STEP_GENERATOR_E = _BV(3)
};

enum StepEventInfoStatus : uint8_t {
    STEP_EVENT_INFO_STATUS_NOT_GENERATED = 0, // Step event isn't produced by any step event generator. Such a step event cannot be inserted into the step event queue.
    STEP_EVENT_INFO_STATUS_GENERATED_INVALID = 1, // Step event is produced by a step event generator but cannot be inserted into the step event queue.
    STEP_EVENT_INFO_STATUS_GENERATED_VALID = 2, // Step event is produced by a step event generator and can be inserted into the step event queue.
};

typedef struct step_event_info_t {
    double time;
    StepEventFlag_t flags;
    StepEventInfoStatus status;
} step_event_info_t;

enum StepGeneratorStatus : uint8_t {
    STEP_GENERATOR_STATUS_OK = 0,
    STEP_GENERATOR_STATUS_NO_STEP_EVENT_PRODUCED = 1,
    STEP_GENERATOR_STATUS_FULL_STEP_EVENT_QUEUE = 2
};

typedef struct basic_step_generator_t {
    const uint8_t axis;
} basic_step_generator_t;

// Groups variables for step event generators that work with move segments or micro move segments (input shaper).
typedef struct move_segment_step_generator_t : basic_step_generator_t {
    float start_v = 0.f;
    float accel = 0.f;
    float start_pos = 0.f;
    bool step_dir = false;
} move_segment_step_generator_t;

struct step_generator_state_t;
typedef step_event_info_t (*generator_next_step_f)(move_segment_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

typedef uint8_t step_index_t;

struct step_generator_state_t {
    basic_step_generator_t *step_generator[4];
    generator_next_step_f next_step_func[4];
    step_event_info_t step_events[4];
    std::array<step_index_t, 4> step_event_index;
    double previous_step_time;

    StepEventFlag_t flags; // current axis/direction flags
    step_event_i32_t buffered_step; // accumulator for multi-axis step fusion

    xyze_long_t current_distance;

    // Number of markers indicating the start of move segments that need to be inserted into step events.
    // Be aware that very short move segments could produce just one single step event or none step event
    // for some generators. And that is why we have to use integer instead of bool because we could need
    // to produce more step events with a mark right after each other.
    int8_t left_insert_start_of_move_segment;

    bool initialized;
};

typedef struct classic_step_generator_t : move_segment_step_generator_t {
    const move_t *current_move = nullptr;
} classic_step_generator_t;

typedef struct input_shaper_step_generator_t : move_segment_step_generator_t {
    input_shaper_state_t *is_state = nullptr;
} input_shaper_step_generator_t;

typedef struct pressure_advance_step_generator_t : basic_step_generator_t {
    pressure_advance_state_t *pa_state = nullptr;
} pressure_advance_step_generator_t;

typedef struct step_generators_pool_t {
    classic_step_generator_t classic_step_generator[4] = { { { { .axis = X_AXIS } } }, { { { .axis = Y_AXIS } } }, { { { .axis = Z_AXIS } } }, { { { .axis = E_AXIS } } } };
#ifdef ADVANCED_STEP_GENERATORS
    input_shaper_step_generator_t input_shaper_step_generator[3] = { { { { .axis = X_AXIS } } }, { { { .axis = Y_AXIS } } }, { { { .axis = Z_AXIS } } } };
    pressure_advance_step_generator_t pressure_advance_step_generator_e = { { .axis = E_AXIS } };
#endif
} step_generators_pool_t;
