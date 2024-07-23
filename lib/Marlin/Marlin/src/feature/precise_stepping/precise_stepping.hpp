/**
 * Based on the implementation in Klipper [https://github.com/Klipper3d/klipper].
 * Copyright (C) Kevin O'Connor <kevin@koconnor.net>
 *
 * Our implementation takes inspiration from the work of Kevin O'Connor <kevin@koconnor.net> for Klipper
 * in used data structures, and some computations.
 */
#pragma once
#include "common.hpp"
#include <atomic>

#ifdef COREXY
    #define COREXY_CONVERT_LIMITS
#endif

// Minimum number of free slots in the move segment queue must be available in the queue in all circumstances.
// 1 free slot is required to ensure that we can add the empty ending move anytime.
constexpr const uint8_t MOVE_SEGMENT_QUEUE_MIN_FREE_SLOTS = 1;

// Maximum number of step events produced in on move interrupt to limit time spent by move interrupt handler
// when the step event queue is empty.
constexpr const uint16_t MAX_STEP_EVENTS_PRODUCED_PER_ONE_CALL = 256;

// Minimum length of move segment. Move segments shorted then this value will be rounded.
constexpr const double EPSILON_DISTANCE = 0.000001;

// Maximum difference in ticks between step events that fit into time_ticks without splitting.
// Currently, it is equal to 65.536ms.
constexpr const int32_t STEP_TIMER_MAX_TICKS_LIMIT = int32_t(std::numeric_limits<decltype(step_event_u16_t::time_ticks)>::max());

// Precomputed period of calling PreciseStepping::isr() when there is no queued step event (1ms).
constexpr const uint16_t STEPPER_ISR_PERIOD_IN_TICKS = (STEPPER_TIMER_RATE / 1000);

// Precomputed conversion rate from seconds to timer ticks.
constexpr const double STEPPER_TICKS_PER_SEC = double(STEPPER_TIMER_RATE);

struct move_t;
struct step_generator_state_t;

typedef uint16_t PreciseSteppingFlag_t;
enum PreciseSteppingFlag : PreciseSteppingFlag_t {
    // Indicated that position of the axis should be reset to zero.
    PRECISE_STEPPING_FLAG_RESET_POSITION_X = _BV(0),
    PRECISE_STEPPING_FLAG_RESET_POSITION_Y = _BV(1),
    PRECISE_STEPPING_FLAG_RESET_POSITION_Z = _BV(2),
    PRECISE_STEPPING_FLAG_RESET_POSITION_E = _BV(3),

    // Indicating logical axis usage until reset
    PRECISE_STEPPING_FLAG_X_USED = _BV(8),
    PRECISE_STEPPING_FLAG_Y_USED = _BV(9),
    PRECISE_STEPPING_FLAG_Z_USED = _BV(10),
    PRECISE_STEPPING_FLAG_E_USED = _BV(11),
};

// Ensure XYZE bits are always adjacent and ordered.
static_assert(PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_Y == (PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_X << 1));
static_assert(PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_Z == (PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_X << 2));
static_assert(PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_E == (PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_X << 3));

// Verify mapping between PreciseSteppingFlag and MoveFlag.
static_assert(MoveFlag::MOVE_FLAG_RESET_POSITION_X == (PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_X << MOVE_FLAG_RESET_POSITION_SHIFT));
static_assert(MoveFlag::MOVE_FLAG_RESET_POSITION_Y == (PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_Y << MOVE_FLAG_RESET_POSITION_SHIFT));
static_assert(MoveFlag::MOVE_FLAG_RESET_POSITION_Z == (PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_Z << MOVE_FLAG_RESET_POSITION_SHIFT));
static_assert(MoveFlag::MOVE_FLAG_RESET_POSITION_E == (PreciseSteppingFlag::PRECISE_STEPPING_FLAG_RESET_POSITION_E << MOVE_FLAG_RESET_POSITION_SHIFT));

static_assert(MoveFlag::MOVE_FLAG_X_ACTIVE == (MoveFlag)PreciseSteppingFlag::PRECISE_STEPPING_FLAG_X_USED);
static_assert(MoveFlag::MOVE_FLAG_Y_ACTIVE == (MoveFlag)PreciseSteppingFlag::PRECISE_STEPPING_FLAG_Y_USED);
static_assert(MoveFlag::MOVE_FLAG_Z_ACTIVE == (MoveFlag)PreciseSteppingFlag::PRECISE_STEPPING_FLAG_Z_USED);
static_assert(MoveFlag::MOVE_FLAG_E_ACTIVE == (MoveFlag)PreciseSteppingFlag::PRECISE_STEPPING_FLAG_E_USED);

class PreciseStepping {

public:
    static step_event_queue_t __attribute__((section(".ccmram"))) step_event_queue;
    static move_segment_queue_t move_segment_queue;
    static step_generator_state_t step_generator_state;

    // Preallocated collection of all step event generators for all axis and all generator types (classic, input shaper, pressure advance).
    static step_generators_pool_t step_generators_pool;
    // Indicate which type of step event generator is enabled on which physical axis.
    static uint8_t physical_axis_step_generator_types;

    // Total number of ticks until the next step event will be processed.
    // Or number of ticks to next call of stepper ISR when step event queue is empty.
    static uint16_t left_ticks_to_next_step_event;

    // Indicate which direction bits are inverted.
    static uint16_t inverted_dirs;

    // It represents the maximum value of how far in the time can some step event generators point.
    // Used for computing flush time that ensures that none of the step event generators will produce step
    // event beyond the flush time. Because for the same state of the move segment queue, some step
    // event generator could generate step events far away from others, which could let to incorrect
    // ordering of step events.
    static double max_lookback_time;

    static xyze_double_t initial_start_pos; // Initial absolute position (mm, cartesian)
    static double total_print_time; // Cumulative time since beginning of motion (s)
    static xyze_double_t total_start_pos; // Current absolute position (mm, cartesian)
    static xyze_long_t total_start_pos_msteps; // Current absolute position in mini-steps (msteps, cartesian)

    // Flags that affect the whole precise stepping. Those flags are reset when all queues are empty.
    // For now, used only for resetting the positions of axes.
    static PreciseSteppingFlag_t flags;

#if !BOARD_IS_DWARF()
    /// This completely arbitrary number is increased every time a stall happens (we run out of motion data)
    /// The idea is that the "subscriber" compares the value to the previous value it has seen and if they differ, the planner has stalled during the time
    /// This doesn't even have to be volatile, even though we're accessing from other threads, as we're only checking for difference and it's only informative.
    static std::atomic<uint32_t> stall_count;
#endif

    PreciseStepping() = default;

    static void init();

    // Reset the motion/stepper generator state from halt
    static void reset_from_halt(bool preserve_step_fraction = true);

    // The step ISR scheduler
    static void step_isr();

    // The move ISR scheduler
    static void move_isr();

    // Main thread loop handler
    static void loop();

    // Process one planner block into move segments
    static void process_queue_of_blocks();

    // Process the move segment queue
    static void process_queue_of_move_segments();

    // Generate step events from the move queue
    static StepGeneratorStatus process_one_move_segment_from_queue();

    // Generate step pulses for the stepper motors.
    // Returns time to the next step event or ISR call.
    static uint16_t process_one_step_event_from_queue();

    // Returns the index of the next move segment in the queue.
    static constexpr uint8_t move_segment_queue_next_index(const uint8_t move_segment_index) { return MOVE_SEGMENT_QUEUE_MOD(move_segment_index + 1); }

    // Returns the index of the previous move segment in the queue.
    static constexpr uint8_t move_segment_queue_prev_index(const uint8_t move_segment_index) { return MOVE_SEGMENT_QUEUE_MOD(move_segment_index - 1); }

    // Remove all move segments from the queue.
    FORCE_INLINE static void move_segment_queue_clear() { move_segment_queue.head = move_segment_queue.tail = move_segment_queue.unprocessed = 0; }

    // Check if the queue has any move segments queued.
    FORCE_INLINE static bool has_move_segments_queued() { return (move_segment_queue.head != move_segment_queue.tail); }

    // Check if the queue has any unprocessed move segments queued.
    FORCE_INLINE static bool has_unprocessed_move_segments_queued() { return (move_segment_queue.head != move_segment_queue.unprocessed); }

    // Check if the queue of move segment is full.
    FORCE_INLINE static bool is_move_segment_queue_full() { return move_segment_queue.tail == move_segment_queue_next_index(move_segment_queue.head); }

    // Number of move segments in the queue.
    FORCE_INLINE static uint8_t move_segment_queue_size() { return MOVE_SEGMENT_QUEUE_MOD(move_segment_queue.head - move_segment_queue.tail); }

    // Returns number of free slots in the move segment queue.
    FORCE_INLINE static uint8_t move_segment_queue_free_slots() { return MOVE_SEGMENT_QUEUE_SIZE - 1 - move_segment_queue_size(); }

    // Returns the current move segment, nullptr if the queue is empty.
    FORCE_INLINE static move_t *get_current_move_segment() {
        if (has_move_segments_queued()) {
            return &move_segment_queue.data[move_segment_queue.tail];
        }

        return nullptr;
    }

    // Returns the current move segment that isn't processed by PreciseStepping::process_queue_of_move_segments(), nullptr if the queue is empty.
    FORCE_INLINE static move_t *get_current_unprocessed_move_segment() {
        if (has_unprocessed_move_segments_queued()) {
            return &move_segment_queue.data[move_segment_queue.unprocessed];
        }

        return nullptr;
    }

    // Returns the last move segment that has been processed by
    // PreciseStepping::process_queue_of_move_segments(), nullptr if the queue is empty or not
    // processed.
    FORCE_INLINE static move_t *get_last_processed_move_segment() {
        if (move_segment_queue.unprocessed != move_segment_queue.tail) {
            return &move_segment_queue.data[move_segment_queue_prev_index(move_segment_queue.unprocessed)];
        }

        return nullptr;
    }

    // Returns the last move segment inside the queue (at the bottom of the queue), nullptr if the queue is empty.
    FORCE_INLINE static move_t *get_last_move_segment() {
        if (has_move_segments_queued()) {
            return &move_segment_queue.data[move_segment_queue_prev_index(move_segment_queue.head)];
        }

        return nullptr;
    }

    // Returns the first head move segment, nullptr if the queue is full.
    // Also, it returns the next move segment queue head index (passed by reference).
    FORCE_INLINE static move_t *get_next_free_move_segment(uint8_t &next_move_segment_queue_head) {
        if (is_move_segment_queue_full()) {
            return nullptr;
        }

        // Return the first available move segment.
        next_move_segment_queue_head = move_segment_queue_next_index(move_segment_queue.head);
        return &move_segment_queue.data[move_segment_queue.head];
    }

    // Discard the current move segment.
    // Caller must ensure that there is something to discard.
    FORCE_INLINE static void discard_current_move_segment() {
        assert(has_move_segments_queued());
        move_segment_queue.tail = move_segment_queue_next_index(move_segment_queue.tail);
    }

    // Discard the current unprocessed move segment.
    // Caller must ensure that there is something to discard.
    FORCE_INLINE static void discard_current_unprocessed_move_segment() {
        assert(has_unprocessed_move_segments_queued());
        move_segment_queue.unprocessed = move_segment_queue_next_index(move_segment_queue.unprocessed);
    }

    // Returns the index of the next step event in the queue.
    static constexpr uint16_t step_event_queue_next_index(const uint16_t step_event_index) { return STEP_EVENT_QUEUE_MOD(step_event_index + 1); }

    // Returns the index of the previous step event in the queue.
    static constexpr uint16_t step_event_queue_prev_index(const uint16_t step_event_index) { return STEP_EVENT_QUEUE_MOD(step_event_index - 1); }

    // Remove all step events from the queue.
    FORCE_INLINE static void step_event_queue_clear() { step_event_queue.head = step_event_queue.tail = 0; }

    // Check if the queue have any step events queued.
    FORCE_INLINE static bool has_step_events_queued() { return (step_event_queue.head != step_event_queue.tail); }

    // Check if the queue of step events is full.
    FORCE_INLINE static bool is_step_event_queue_full() { return step_event_queue.tail == step_event_queue_next_index(step_event_queue.head); }

    // Number of step events in the queue.
    FORCE_INLINE static uint16_t step_event_queue_size() { return STEP_EVENT_QUEUE_MOD(step_event_queue.head - step_event_queue.tail); }

    // Returns number of free slots in the step event queue.
    FORCE_INLINE static uint16_t step_event_queue_free_slots() { return STEP_EVENT_QUEUE_SIZE - 1 - step_event_queue_size(); }

    // Returns the current step event, nullptr if the queue is empty.
    static step_event_u16_t *get_current_step_event() {
        if (has_step_events_queued()) {
            return &step_event_queue.data[step_event_queue.tail];
        }

        return nullptr;
    }

    // Returns the first head step event, nullptr if the queue is full.
    // Also, it returns the next step event queue head index (passed by reference).
    FORCE_INLINE static step_event_u16_t *get_next_free_step_event(uint16_t &next_step_event_queue_head) {
        if (is_step_event_queue_full()) {
            return nullptr;
        }

        // Return the first available step event.
        next_step_event_queue_head = step_event_queue_next_index(step_event_queue.head);
        return &step_event_queue.data[step_event_queue.head];
    }

    // Discard the current step event.
    FORCE_INLINE static void discard_current_step_event() {
        if (has_step_events_queued()) {
            step_event_queue.tail = step_event_queue_next_index(step_event_queue.tail);
        }
    }

    FORCE_INLINE static void step_generator_state_clear() {
        step_generator_state.initialized = false;

        // always drop buffered step/s as those can be processed also with an empty move queue
        step_generator_state.buffered_step.flags = 0;
    }

    FORCE_INLINE static move_t *move_segment_queue_next_move(const move_t &move) {
        int32_t move_idx = &move - &move_segment_queue.data[0];
        assert(move_idx >= 0 && move_idx < MOVE_SEGMENT_QUEUE_SIZE); // move_idx out of bounds of the move queue.
        assert(move_idx != move_segment_queue.head); // Input move segment is out of the move queue.

        if (uint8_t next_move_idx = move_segment_queue_next_index(uint8_t(move_idx)); next_move_idx == move_segment_queue.head) {
            return nullptr;
        } else {
            return &PreciseStepping::move_segment_queue.data[next_move_idx];
        }
    }

    FORCE_INLINE static StepEventInfoStatus get_nearest_step_event_status() {
        return step_generator_state.step_events[step_generator_state.step_event_index[0]].status;
    }

    FORCE_INLINE static void reset_nearest_step_event_status() {
        step_generator_state.step_events[step_generator_state.step_event_index[0]].status = STEP_EVENT_INFO_STATUS_NOT_GENERATED;
    }

    static void update_maximum_lookback_time();

    // This function must be called after the whole actual move segment is processed or the artificially
    // created move segment is processed, as in the input shaper case.
    static void move_segment_processed_handler();

    // Reset the step/move queue
    static void quick_stop() { stop_pending = true; }

    // Return true if the motion is being stopped
    static bool stopping() { return stop_pending; }

    // Return if any of queues have blocks pending
    static bool has_blocks_queued() { return has_move_segments_queued() || has_step_events_queued(); }

    // Return if some processing is still pending before all queues are flushed
    static bool processing() { return busy || stop_pending; }

    static volatile uint8_t step_dl_miss; // stepper deadline misses
    static volatile uint8_t step_ev_miss; // stepper event misses

private:
    static uint32_t waiting_before_delivering_start_time;
    static uint32_t last_step_isr_delay;

    static void step_generator_state_init(const move_t &move);

    static std::atomic<bool> busy;
    static std::atomic<bool> stop_pending;
    static void reset_queues();
    static bool is_waiting_before_delivering();
};

void classic_step_generator_init(const move_t &move, classic_step_generator_t &step_generator, step_generator_state_t &step_generator_state);

FORCE_INLINE void classic_step_generator_update(classic_step_generator_t &step_generator);

// Common functions:
float get_move_axis_r(const move_t &move, const int axis);
void mark_ownership(move_t &move);
void discard_ownership(move_t &move);
