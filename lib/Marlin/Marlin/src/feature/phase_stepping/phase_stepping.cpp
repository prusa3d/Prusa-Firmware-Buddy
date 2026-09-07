#include "phase_stepping.hpp"
#include "calibration_hooks.hpp"
#include "debug_util.hpp"

#if HAS_BURST_STEPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    #include "burst_stepper.hpp"
#else
    #include "quick_tmc_spi.hpp"
#endif

#include "../precise_stepping/precise_stepping.hpp"
#include "../precise_stepping/internal.hpp"
#include "../input_shaper/input_shaper.hpp"
#include "axes.hpp"
#include "debug_events.hpp"
#include <config_store/store_instance.hpp>

#include <device/peripherals.hpp>
#include <module/motion.h>
#include <module/stepper.h>
#include <feature/motordriver_util.h>

#include <Pin.hpp>
#include <logging/log.hpp>
#include <metric.h>

#include <cmath>
#include <sys/unistd.h>

#include <timing_precise.hpp>

LOG_COMPONENT_DEF(PhaseStepping, logging::Severity::debug);

using namespace phase_stepping;
using namespace phase_stepping::opts;
using namespace buddy::hw;

AtomicCircularQueue<ProblemEvents, uint16_t, 32> phase_stepping::debug_events_queue {};

// Max rounding "adjustment" for jump_to_position(origin=true).
//
// In perfect arithmetic, it should stay the same, but there is rounding
// through two int() calls, which can perturb it a little bit. Anything beyond
// this is likely a logic error.
static constexpr int MAX_JUMP_PHASE_DELTA = 4;

// Global definitions
std::array<AxisState, SUPPORTED_AXIS_COUNT> phase_stepping::axis_states = { X_AXIS, Y_AXIS };
static bool initialized = false;

// Module definitions
#if !HAS_BURST_STEPPING()
static uint_fast8_t axis_num_to_refresh = 0;
#endif
static uint32_t last_timer_tick = 0;

MoveTarget::MoveTarget(float initial_pos)
    : initial_pos(initial_pos)
    , half_accel(0)
    , start_v(0)
    , duration(0)
    , target_pos(initial_pos) {}

MoveTarget::MoveTarget(float initial_pos, const move_t &move, int axis, const uint64_t move_duration_ticks)
    : initial_pos(initial_pos)
    , duration(uint32_t(move_duration_ticks)) {
    debug_assert(move_duration_ticks <= std::numeric_limits<uint32_t>::max());
    float r = get_move_axis_r(move, axis);
    half_accel = r * float(move.half_accel);
    start_v = r * float(move.start_v);
    target_pos = target_position();
}

MoveTarget::MoveTarget(float initial_pos, const input_shaper_state_t &is_state, const uint64_t move_duration_ticks)
    : initial_pos(initial_pos)
    , half_accel(is_state.half_accel)
    , start_v(is_state.start_v)
    , duration(uint32_t(move_duration_ticks)) {
    debug_assert(move_duration_ticks <= std::numeric_limits<uint32_t>::max());
    target_pos = target_position();
}

float MoveTarget::target_position() const {
    float epoch = duration / static_cast<float>(TICK_FREQ);
    return initial_pos + start_v * epoch + half_accel * epoch * epoch;
}

void phase_stepping::init() {
    // Enforce the proper init sequence
    if (!PreciseStepping::inverted_dirs_set) {
        bsod("stepper directions not initialized");
    }

    phase_stepping::initialize_axis_motor_params();

    // Setup fixed axis parameters so that they can be used before the axis is initialized
    for (auto &axis_state : axis_states) {
        axis_state.inverted = Stepper::is_axis_inverted(AxisEnum(axis_state.axis_index));
    }

    initialized = true;
}

FORCE_INLINE TimeTicks calc_move_segment_end_time(const move_t &move) {
    return move.print_time + move.move_time;
}

FORCE_INLINE TimeTicks calc_move_segment_end_time(const input_shaper_state_t &is_state) {
    return is_state.nearest_next_change;
}

template <typename T>
FORCE_INLINE T resolve_axis_inversion(bool is_inverted_flag, T val) {
    // Since the TMC driver in XDirect mode swaps meaning of phases, the
    // non-inverted axis has to be inverted and vice-versa
    return is_inverted_flag
        ? val
        : -val;
}

FORCE_INLINE CorrectedCurrentLut &resolve_current_lut(AxisState &axis_state) {
    return axis_state.direction
        ? axis_state.forward_current
        : axis_state.backward_current;
}

static void init_step_generator_internal(
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    auto &axis_state = *step_generator.phase_step_state;
    const uint8_t axis = step_generator.axis;

    debug_assert(axis_state.pending_targets.isEmpty());

    axis_state.initial_time = step_generator_state.initial_time;

    axis_state.last_position = axis_state.next_target.peek().initial_pos;
    axis_state.current_target = MoveTarget(axis_state.last_position);
    axis_state.has_current_target = true;
    axis_state.next_target_end_time = MAX_PRINT_TIME_TICKS;

    int32_t initial_steps_made = pos_to_steps(AxisEnum(axis), axis_state.next_target.peek().initial_pos);
    axis_state.initial_count_position = Stepper::get_axis_steps(AxisEnum(axis)) - initial_steps_made;
    axis_state.initial_count_position_from_startup = Stepper::get_axis_steps_from_startup(AxisEnum(axis)) - initial_steps_made;

    axis_state.active.store(true, std::memory_order_seq_cst);
}

void phase_stepping::init_step_generator_classic(
    const move_t &move,
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    debug_assert(is_beginning_empty_move(move));

    auto &axis_state = *step_generator.phase_step_state;
    axis_state.active.store(false, std::memory_order_seq_cst);

    const uint8_t axis = step_generator.axis;
    axis_state.current_print_time_us = static_cast<uint64_t>(move.print_time.to_us_floor());

    axis_state.next_target_end_time = calc_move_segment_end_time(move);
    const uint64_t next_print_time_us = static_cast<uint64_t>(axis_state.next_target_end_time.to_us_floor());
    const uint64_t move_duration_ticks = next_print_time_us - axis_state.current_print_time_us;
    const float move_start_pos = extract_physical_position(AxisEnum(axis), move.start_pos);
    axis_state.next_target.set(MoveTarget(move_start_pos, move, axis, move_duration_ticks));
    axis_state.current_print_time_us = next_print_time_us;

    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)next_step_event_classic;

    // We are keeping a reference to a move segment in last_processed_move.
    // Otherwise, a move segment can be discarded while we are using it.
    move.reference_cnt += 1;
    axis_state.last_processed_move = &move;

    init_step_generator_internal(step_generator, step_generator_state);
}

void phase_stepping::init_step_generator_input_shaping(
    const move_t &move,
    input_shaper_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    debug_assert(is_beginning_empty_move(move));

    auto &axis_state = *step_generator.phase_step_state;
    axis_state.active.store(false, std::memory_order_seq_cst);

    // Inherit input shaper initialization...
    input_shaper_step_generator_init(move, step_generator, step_generator_state);

    axis_state.current_print_time_us = static_cast<uint64_t>(step_generator.is_state->print_time.to_us_floor());

    axis_state.next_target_end_time = calc_move_segment_end_time(*step_generator.is_state);
    const uint64_t next_print_time_us = static_cast<uint64_t>(axis_state.next_target_end_time.to_us_floor());
    const uint64_t move_duration_ticks = next_print_time_us - axis_state.current_print_time_us;
    axis_state.next_target.set(MoveTarget(step_generator.is_state->start_pos, *step_generator.is_state, move_duration_ticks));
    axis_state.current_print_time_us = next_print_time_us;

    // ...and then override next_step_func with phase stepping one
    const uint8_t axis = step_generator.axis;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)next_step_event_input_shaping;

    // we don't keep a reference to the last move with IS, reset it
    axis_state.last_processed_move = nullptr;

    init_step_generator_internal(step_generator, step_generator_state);
}

step_event_info_t phase_stepping::next_step_event_classic(
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    AxisState &axis_state = *step_generator.phase_step_state;

    debug_assert(axis_state.last_processed_move != nullptr);
    debug_assert(axis_state.last_processed_move->reference_cnt != 0);

    step_event_info_t next_step_event = { TimeTicks::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };
    if (axis_state.pending_targets.isFull()) {
        next_step_event.time = axis_state.next_target_end_time;
        next_step_event.status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_PENDING;
    } else if (const move_t *next_move = PreciseStepping::move_segment_queue_next_move(*axis_state.last_processed_move); next_move != nullptr) {
        next_step_event.time = next_move->print_time;

        const uint8_t axis = axis_state.axis_index;
        const float move_start_pos = extract_physical_position(AxisEnum(axis), next_move->start_pos);

        // push the buffered target
        if (auto next_target = axis_state.next_target.take(); next_target.has_value()) {
            next_target->target_pos = move_start_pos;
            [[maybe_unused]] bool enqueued = axis_state.pending_targets.enqueue(*next_target);
            debug_assert(enqueued);
        }

        // buffer the next
        if (!is_ending_empty_move(*next_move)) {
            axis_state.next_target_end_time = calc_move_segment_end_time(*next_move);
            // floor endpoints first, then difference (preserves Σ(durations)==floor(absolute) invariant)
            const uint64_t next_print_time_us = static_cast<uint64_t>(axis_state.next_target_end_time.to_us_floor());
            const uint64_t move_duration_ticks = next_print_time_us - axis_state.current_print_time_us;
            MoveTarget next(move_start_pos, *next_move, axis, move_duration_ticks);
            axis_state.next_target.set(next);
            axis_state.current_print_time_us = next_print_time_us;

            const int32_t target_steps = pos_to_steps(AxisEnum(axis), next.target_pos);
            step_generator_state.current_distance[axis] = target_steps;

            next_step_event.flags |= STEP_EVENT_FLAG_KEEP_ALIVE;
            next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_KEEP_ALIVE;
        }

        // The move segment is fully processed, and in the queue is another unprocessed move segment.
        // So we decrement reference count of the current move segment and increment reference count of next move segment.
        --axis_state.last_processed_move->reference_cnt;
        axis_state.last_processed_move = next_move;
        ++axis_state.last_processed_move->reference_cnt;

        PreciseStepping::move_segment_processed_handler();
    } else {
        next_step_event.time = axis_state.last_processed_move->print_time + axis_state.last_processed_move->move_time;
    }

    return next_step_event;
}

step_event_info_t phase_stepping::next_step_event_input_shaping(
    input_shaper_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    AxisState &axis_state = *step_generator.phase_step_state;

    debug_assert(step_generator.is_state != nullptr);

    step_event_info_t next_step_event = { TimeTicks::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };
    if (axis_state.pending_targets.isFull()) {
        next_step_event.time = axis_state.next_target_end_time;
        next_step_event.status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_PENDING;
    } else {
        next_step_event.time = step_generator.is_state->nearest_next_change;

        if (input_shaper_state_update(*step_generator.is_state, step_generator.axis)) {
            uint8_t axis = axis_state.axis_index;
            const float move_start_pos = step_generator.is_state->start_pos;

            // push the buffered target
            if (auto next_target = axis_state.next_target.take(); next_target.has_value()) {
                next_target->target_pos = move_start_pos;
                [[maybe_unused]] bool enqueued = axis_state.pending_targets.enqueue(*next_target);
                debug_assert(enqueued);
            }

            // buffer the next
            if (step_generator.is_state->nearest_next_change < MAX_PRINT_TIME_TICKS) {
                axis_state.next_target_end_time = calc_move_segment_end_time(*step_generator.is_state);
                // floor endpoints first, then difference (preserves Σ(durations)==floor(absolute) invariant)
                const uint64_t next_print_time_us = static_cast<uint64_t>(axis_state.next_target_end_time.to_us_floor());
                const uint64_t move_duration_ticks = next_print_time_us - axis_state.current_print_time_us;
                MoveTarget next(move_start_pos, *step_generator.is_state, move_duration_ticks);
                axis_state.next_target.set(next);
                axis_state.current_print_time_us = next_print_time_us;

                const int32_t target_steps = pos_to_steps(AxisEnum(axis), next.target_pos);
                step_generator_state.current_distance[axis] = target_steps;

                next_step_event.flags |= STEP_EVENT_FLAG_KEEP_ALIVE;
                next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_KEEP_ALIVE;
            }
        }

        PreciseStepping::move_segment_processed_handler();
    }

    return next_step_event;
}

bool phase_stepping::is_initialized() {
    return initialized;
}

#ifdef _DEBUG
void phase_stepping::assert_initialized() {
    // This is explicitly kept non-inline to serve as a single trap point
    debug_assert(initialized);
}

void phase_stepping::assert_disabled() {
    // This is explicitly kept non-inline to serve as a single trap point
    debug_assert(!any_axis_enabled());
}

void phase_stepping::check_state() {
    if (!initialized) {
        // this function can be called early during app_setup() as a side-effect of synchronize,
        // before init() actually gets called.
        return;
    }

    for (auto &state : axis_states) {
        if (!state.enabled) {
            continue;
        }

    #if HAS_BURST_STEPPING()
        // #error dead code found by automatic analyses (see BFW-5461)
        debug_assert(!burst_stepping::busy());

        // Ensure driver_phase is in sync with MSCNT
        int mscnt = stepper_axis((AxisEnum)state.axis_index).MSCNT();
        if (mscnt != state.driver_phase) {
            bsod("desync:axis=%i,mscnt=%hu,phase=%i", state.axis_index, mscnt, state.driver_phase);
        }
    #endif
    }
}
#endif // _DEBUG

void phase_stepping::synchronize() {
    planner.synchronize();
}

bool phase_stepping::motion_pending() {
    if (!initialized) {
        return false;
    }
    for (auto &state : axis_states) {
        if (!state.pending_targets.isEmpty() || state.has_current_target) {
            return true;
        }
    }
    return false;
}

bool phase_stepping::processing() {
    if (!initialized) {
        // processing() can be called early during app_setup(), before init() actually gets called.
        return false;
    }

    if (motion_pending()) {
        return true;
    }

    // ensure the last target has also been flushed
#if HAS_BURST_STEPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    if (burst_stepping::busy()) {
        return true;
    }
#else
    if (phase_stepping::spi::busy()) {
        return true;
    }
#endif

    return false;
}

// Fetch the current MSCNT position at standstill
static int get_current_phase(AxisEnum axis) {
    auto &stepper = stepper_axis(axis);

    // switch off interpolation first to ensure position is settled
    bool had_interpolation = stepper.intpol();
    stepper.intpol(false);

    // fetch MSCNT at full resolution
    int original_microsteps = stepper.microsteps();
    int current_phase = stepper.MSCNT();

    // restore original settings
    stepper.microsteps(original_microsteps);
    stepper.intpol(had_interpolation);

    return current_phase;
}

void phase_stepping::jump_to_position(AxisEnum axis, float pos, bool set_origin) {
    debug_assert(axis < SUPPORTED_AXIS_COUNT);
    assert_initialized();

    auto &axis_state = axis_states[axis];
    debug_assert(axis_state.pending_targets.isEmpty() && !axis_state.has_current_target);

    bool was_active = axis_state.active.exchange(false, std::memory_order_seq_cst);

    if (set_origin) {
        // If phase stepping isn't already active, we must fetch the current rotor position
        int current_phase = (was_active ? axis_state.last_phase : get_current_phase(axis));
        float physical_position = resolve_axis_inversion(axis_state.inverted, pos);
        axis_state.offset = phase_to_pos(axis, phase_difference(current_phase, pos_to_phase(axis, physical_position)));
    }

    axis_state.last_position = pos;
    if (set_origin && was_active) {
        // The intention here is to stay at the same physical position.
        // Compute the "new" values the refresh will get and check this is
        // true, allowing for a little bit of rounding error.
        float check_phys = resolve_axis_inversion(axis_state.inverted, axis_state.last_position)
            + axis_state.offset;
        int check_phase = pos_to_phase(axis, check_phys);
        int delta = phase_difference(axis_state.last_phase, check_phase);
        if (std::abs(delta) > MAX_JUMP_PHASE_DELTA) {
            bsod("phase-jump axis=%d delta=%d", static_cast<int>(axis), delta);
        }
    }
    axis_state.active.store(was_active, std::memory_order_seq_cst);
}

namespace phase_stepping {
/**
 * Enables phase stepping for axis. Reconfigures the motor driver. It is not
 * safe to invoke this procedure within interrupt context. No movement shall be
 * be in progress.
 **/
static void enable_phase_stepping(AxisEnum axis_num) {
    debug_assert(PreciseStepping::initialized());
    debug_assert(axis_num < SUPPORTED_AXIS_COUNT);
    debug_assert(!axis_states[axis_num].enabled);
    if (!planner.draining()) {
        // If we are within an aborted move, we must still allow to re-enable phase stepping:
        // we might be within an EnsureDisabled block. In such case motion is already stopped,
        // and it will re-initialize phase stepping on the next move. If we're not within an
        // aborted move though we must ensure there's no existing motion going on.
        debug_assert(!planner.has_blocks_queued() && !PreciseStepping::processing());
    }

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    TMCStepperType &stepper = stepper_axis(axis_num);
    auto &axis_state = axis_states[axis_num];
    debug_assert(!axis_state.enabled && !axis_state.active);
    debug_assert(!axis_state.has_current_target && axis_state.pending_targets.isEmpty());

    // Read axis configuration and cache it so we can access it fast
    axis_state.direction = (Stepper::motor_direction(axis_num) ^ axis_state.inverted);

    // switch off interpolation first to ensure position is settled
    axis_state.had_interpolation = stepper.intpol();
    stepper.intpol(false);

    // switch microsteps and fetch MSCNT at full resolution
    axis_state.original_microsteps = stepper.microsteps();
    stepper.microsteps(256);
    int current_phase = stepper.MSCNT();

    axis_state.last_phase = current_phase;
#if HAS_BURST_STEPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    axis_state.driver_phase = current_phase;
    burst_stepping::init();
#else
    // In order to start phase stepping, we have to set phase currents that are
    // in sync with current position, and then switch the driver to current
    // mode.
    axis_state.last_currents = resolve_current_lut(axis_state).get_current(current_phase);

    // Set IHOLD to be the same as IRUN (as IHOLD is always used in XDIRECT)
    axis_state.initial_hold_multiplier = stepper.hold_multiplier();
    stepper.rms_current(stepper.rms_current(), 1.);

    // Swapping coils isn't a mistake - TMC in Xdirect mode swaps coils
    stepper.coil_A(axis_state.last_currents.b);
    stepper.coil_B(axis_state.last_currents.a);
    stepper.direct_mode(true);
#endif

    // Sync the counters just before enabling the axis
    int32_t initial_steps_made = pos_to_steps(axis_num, axis_state.last_position);
    axis_state.initial_count_position = Stepper::get_axis_steps(axis_num) - initial_steps_made;
    axis_state.initial_count_position_from_startup = Stepper::get_axis_steps_from_startup(axis_num) - initial_steps_made;

    axis_state.missed_tx_cnt = 0;
    axis_state.enabled = true;
    axis_state.active.store(true, std::memory_order_seq_cst);
    axis_state.is_slowed_down = true;

    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types |= enable_mask;

    HAL_TIM_Base_Start_IT(tim_handle_phase_stepping);
}

#if !HAS_BURST_STEPPING()
static inline void single_step_axis(AxisEnum axis) {
    switch (axis) {
    case X_AXIS:
    #if ENABLED(SQUARE_WAVE_STEPPING)
        X_STEP_SET();
    #else
        // #error dead code found by automatic analyses (see BFW-5461)
        X_STEP_SET();
        delay_us_precise<MINIMUM_STEPPER_PULSE>();
        X_STEP_RESET();
    #endif
        break;
    case Y_AXIS:
    #if ENABLED(SQUARE_WAVE_STEPPING)
        Y_STEP_SET();
    #else
        // #error dead code found by automatic analyses (see BFW-5461)
        Y_STEP_SET();
        delay_us_precise<MINIMUM_STEPPER_PULSE>();
        Y_STEP_RESET();
    #endif
        break;
    case Z_AXIS:
    #if ENABLED(SQUARE_WAVE_STEPPING)
        Z_STEP_SET();
    #else
        // #error dead code found by automatic analyses (see BFW-5461)
        Z_STEP_SET();
        delay_us_precise<MINIMUM_STEPPER_PULSE>();
        Z_STEP_RESET();
    #endif
        break;
    default:
        break;
    }
}

static void step_to_phase(AxisEnum axis, int phase) {
    auto &stepper = stepper_axis(axis);
    while (phase != stepper.MSCNT()) {
        single_step_axis(axis);
        delay_us_precise<MINIMUM_STEPPER_PULSE>();
    }
}
#endif

/**
 * Disable phase stepping for axis. Reconfigures the motor driver. It is not
 * safe to invoke this procedure within interrupt context. No movement shall be
 * in progress.
 **/
static void disable_phase_stepping(AxisEnum axis_num) {
    debug_assert(axis_num < SUPPORTED_AXIS_COUNT);
    debug_assert(axis_states[axis_num].enabled);
    debug_assert(!planner.processing());

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    auto &stepper = static_cast<TMC2130Stepper &>(stepper_axis(axis_num));
    auto &axis_state = axis_states[axis_num];

    axis_state.active.store(false, std::memory_order_seq_cst);
    motor_serial_lock_clear_isr_starved();
    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types &= ~enable_mask;

#if !HAS_BURST_STEPPING()
    // In order to avoid glitch in motor motion, we have to first, make steps to
    // get MSCNT into sync and then we disable XDirect mode
    int current_phase = normalize_motor_phase(axis_state.last_phase);
    step_to_phase(axis_num, current_phase);
    stepper.direct_mode(false);
#endif

    // Reset driver params to original state
    stepper.microsteps(axis_state.original_microsteps);
    stepper.intpol(axis_state.had_interpolation);
    stepper.rms_current(stepper.rms_current(), axis_state.initial_hold_multiplier);

    // Resynchronize driver direction to last known direction
    switch (axis_num) {
    case X_AXIS:
        buddy::hw::xDir.writeb(!(Stepper::motor_direction(axis_num) ^ INVERT_X_DIR));
        break;
    case Y_AXIS:
        buddy::hw::yDir.writeb(!(Stepper::motor_direction(axis_num) ^ INVERT_Y_DIR));
        break;
    case Z_AXIS:
        buddy::hw::zDir.writeb(!(Stepper::motor_direction(axis_num) ^ INVERT_Z_DIR));
        break;
    default:
        break;
    }

    // Resync count_direction with last_direction_bits
    Stepper::set_count_direction(axis_num, Stepper::motor_direction(axis_num) ? -1 : 1);

    // Disable and shutdown timer if we're the last axis
    axis_state.enabled = false;
    if (!any_axis_enabled()) {
        HAL_TIM_Base_Stop_IT(tim_handle_phase_stepping);
    }
}

} // namespace phase_stepping

void phase_stepping::enable(AxisEnum axis_num, bool enable) {
    debug_assert(axis_num < SUPPORTED_AXIS_COUNT);
    assert_initialized();

    auto &axis_state = axis_states[axis_num];
    if (axis_state.enabled == enable) {
        return;
    }
    if (enable) {
        phase_stepping::enable_phase_stepping(axis_num);
    } else {
        phase_stepping::disable_phase_stepping(axis_num);
    }
}

void phase_stepping::clear_targets() {
    for (auto &axis_state : axis_states) {
        bool was_active = axis_state.active.exchange(false, std::memory_order_seq_cst);

        axis_state.has_current_target = false;
        while (!axis_state.pending_targets.isEmpty()) {
            axis_state.pending_targets.dequeue();
        }

        axis_state.active.store(was_active, std::memory_order_seq_cst);
    }
}

constexpr int current_regulation_range_max = 255;

// template is needed in order to be able to use optimization
// inv_endpoint_minus_breakpoint must be constexpr and we can not have constexpr function parameters
template <float breakpoint, float endpoint, int reduction_to>
[[maybe_unused]] FORCE_INLINE static int regulate_current(float speed) {
    constexpr float slope = (current_regulation_range_max - reduction_to) / (endpoint - breakpoint);
    constexpr float precalculated_const = reduction_to - (int)(breakpoint * slope);

    speed = std::abs(speed);
    if (speed < breakpoint) {
        return current_regulation_range_max;
    }
    if (speed > endpoint) {
        return reduction_to;
    }

    // speed optimized calculation of original formula:
    // current = reduction_to + (speed  - breakpoint) * slope
    return static_cast<int>((speed * slope) + precalculated_const);
}

// Given axis and speed, return current adjustment expressed as range <0, 255>
[[maybe_unused]] inline static int current_adjustment(int /*axis*/, float speed) {
#if PRINTER_IS_PRUSA_XL()
    return regulate_current<6.f, 10.f, 150>(speed);
#elif PRINTER_IS_PRUSA_iX() // TODO simple copy-paste of XL values. To be removed as soon as iX values are measured
    return regulate_current<6.f, 10.f, 150>(speed);
#elif PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
    return current_regulation_range_max;
#elif PRINTER_IS_PRUSA_MK4()
    return current_regulation_range_max;
#else
    #error "Unsupported printer"
#endif
}

int phase_stepping::phase_difference(int a, int b) {
    int diff = a - b;
    if (diff > MOTOR_PERIOD / 2) {
        diff -= MOTOR_PERIOD;
    } else if (diff < -MOTOR_PERIOD / 2) {
        diff += MOTOR_PERIOD;
    }
    return diff;
}

static void mark_missed_transaction(AxisState &axis_state) {
    ++axis_state.missed_tx_cnt;
    if (axis_state.missed_tx_cnt > ALLOWED_MISSED_TX) {
        bsod("Phase stepping: Too many missed transactions");
    }
}

static bool is_refresh_period_sane(uint32_t now, uint32_t last_timer_tick) {
    // The refresh period of the timer should be constant. We mainly care about
    // the case when there was too long delay between refreshes as it marks that
    // the interrupt was delayed and the next update might be sooner than we
    // anticipate.
    static constexpr uint UPDATE_DURATION_US = 20; // Rather pesimistic update

    uint32_t refresh_period = ticks_diff(now, last_timer_tick);
    return refresh_period < 2 * REFRESH_PERIOD_US - UPDATE_DURATION_US;
}

static FORCE_INLINE FORCE_OFAST void refresh_axis(
    AxisState &axis_state, uint32_t now, uint32_t previous_tick) {

    static constexpr float speed_diff_threshold = 20.0f;

    // Acquire - "synchronize"/pull values protected by the active "lock" (yes,
    // it works as kind of a lock on certain writers, like clear_targets or
    // jump_to_position).
    if (!axis_state.active.load(std::memory_order_acquire)) {
        return;
    }

    [[maybe_unused]] const auto axis_index = axis_state.axis_index;
    [[maybe_unused]] const auto axis_enum = AxisEnum(axis_state.axis_index);

    if (!is_refresh_period_sane(now, previous_tick)) {
        // If the ISR handler was delayed, we don't have enough time to process
        // the update. Abort the update so we can catch up.
        mark_missed_transaction(axis_state);
        motor_serial_lock_mark_isr_starved();
        return;
    }

#if !HAS_BURST_STEPPING()
    if (!phase_stepping::spi::initialize_transaction()) {
        mark_missed_transaction(axis_state);
        return;
    }
#endif

    uint32_t move_epoch = ticks_diff(now, axis_state.initial_time);
    float move_position = axis_state.last_position;

    if (!axis_state.has_current_target || move_epoch >= axis_state.current_target.duration) {
        if (axis_state.has_current_target) {
            axis_state.initial_time += axis_state.current_target.duration;
            move_position = axis_state.current_target.target_pos;
            axis_state.has_current_target = false;
            calibration_move_cleanup(axis_state);
        }

        std::optional<MoveTarget> current_target;

        if (!axis_state.pending_targets.isEmpty()) {
            // Pull new movement
            current_target = axis_state.pending_targets.dequeue();

            if (axis_state.pending_targets.count() < 3) {
                // We are running low on more segments. Try to ask the move ISR
                // to provide a refill.
                //
                // If we are running low because we are stopping a print (or
                // something similar), then we'll unfortunately bother it
                // multiple times until we actually do run dry. But that should
                // be mostly harmless.
                PreciseStepping::wake_up();
            }
        } else {
            // Running dry. Steal the next target as the last resort not to stall the stepping.
            //
            // Stall for few iterations of the interrupt is probably OK, it'll
            // catch up, but we've seen stalls in order of seconds.
            current_target = axis_state.next_target.steal();
        }

        if (current_target.has_value()) {
            axis_state.current_target = *current_target;
            axis_state.has_current_target = true;

            if (__builtin_fabsf(current_target->start_v - axis_state.previous_speed) > speed_diff_threshold) {
                [[maybe_unused]] const auto res = debug_events_queue.enqueue(SuddenSpeedChange { .timestamp = now, .axis = axis_index ? 'Y' : 'X', .original_speed = axis_state.previous_speed, .new_speed = current_target->start_v });
            }

            axis_state.is_cruising.store((current_target->half_accel == 0) && (current_target->duration > 10'000), std::memory_order_release);
            {
                float end_speed, end_pos_unused;
                axis_position(axis_state, current_target->duration, end_speed, end_pos_unused);
                axis_state.is_slowed_down = __builtin_fabsf(end_speed) < 2; // if < 2 we slowed down
            }
            if (axis_state.stalled_for != 0) {
                [[maybe_unused]] const auto res = debug_events_queue.enqueue(Stalled {
                    .timestamp = now,
                    .axis = axis_index ? 'Y' : 'X',
                    .number_of_iteration = axis_state.stalled_for,
                });
            }
            axis_state.stalled_for = 0;

            move_position = current_target->initial_pos;
            // In case the new target is already past, pretend we got only so far as to its end.
            move_epoch = std::min(ticks_diff(now, axis_state.initial_time), static_cast<int32_t>(axis_state.current_target.duration));

            calibration_new_move(axis_state);
        } else {
            // No new movement
            axis_state.is_cruising.store(false, std::memory_order_release);
            if (planner.draining()) {
                axis_state.is_slowed_down = true;
            }

            if (!axis_state.is_slowed_down) {
                ++axis_state.stalled_for;
            }
        }
    }

    float speed, position;
    if (axis_state.has_current_target) {
        axis_position(axis_state, move_epoch, speed, position);
    } else {
        speed = 0.f;
        position = move_position;
    }

    axis_state.previous_speed = speed;

    float physical_position = resolve_axis_inversion(axis_state.inverted, position) + axis_state.offset;
    float physical_speed = resolve_axis_inversion(axis_state.inverted, speed);

    if (physical_speed != 0.f) {
        // update the direction in order to fetch the correct lut
        axis_state.direction = physical_speed > 0;
    }
    const auto &current_lut = resolve_current_lut(axis_state);

    int new_phase = pos_to_phase(axis_enum, physical_position);
    debug_assert(phase_difference(axis_state.last_phase, new_phase) < 256);

#if HAS_BURST_STEPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    int phase_correction = calibration_phase_correction(axis_state, current_lut, position, new_phase);

    int shifted_phase = normalize_motor_phase(new_phase + phase_correction);
    int steps_diff = phase_difference(shifted_phase, axis_state.driver_phase);
    burst_stepping::set_phase_diff(axis_enum, steps_diff);
    axis_state.driver_phase = shifted_phase;
#else
    CoilCurrents new_currents = calibration_phase_correction(axis_state, current_lut, position, new_phase);

    int c_adj = current_adjustment(axis_index, mm_to_rev(axis_enum, physical_speed));
    new_currents.a = new_currents.a * c_adj / 255;
    new_currents.b = new_currents.b * c_adj / 255;
    if (new_currents != axis_state.last_currents) {
        spi::set_xdirect(axis_index, new_currents);
        axis_state.last_currents = new_currents;
    }
#endif

    // Only update counters if position didn't change, so that when idling the stepper counters can
    // be manipulated directly. When motion is restarted init_step_generator is guaranteed to be
    // called and will refresh the new starting values.
    if (position != axis_state.last_position) {
        // update counters to the new position
        int32_t steps_made = pos_to_steps(axis_enum, position);
        Stepper::set_axis_steps(axis_enum, axis_state.initial_count_position + steps_made);
        Stepper::set_axis_steps_from_startup(axis_enum, axis_state.initial_count_position_from_startup + steps_made);

        // flag axis movement (if any)
        if (physical_speed != 0.f) {
            Stepper::report_axis_movement(axis_enum, physical_speed);
        }

        axis_state.last_position = position;
    }

    axis_state.last_phase = new_phase;
    axis_state.missed_tx_cnt = 0;
    axis_state.last_timer_tick = last_timer_tick;
}

FORCE_OFAST void phase_stepping::handle_periodic_refresh() {
    // This routine is extremely time sensitive and it should be as fast as
    // possible.
    //
    // The latest measurement show the following timing (from ISR to end 7 µs
    // total):
    //
    // - time + move advancement handling: 970ns (happy path)
    // - position computation: 1.9µs
    // - post to phase: 1.3 µs
    // - current lookup: 800 ns
    // - Quick transmission: 900ns (time from call to first bit) + 1µs transaction termination
    uint32_t now = ticks_us() + REFRESH_PERIOD_US;

    // always refresh the last_timer_tick
    uint32_t old_tick = last_timer_tick;
    last_timer_tick = now;

#if HAS_BURST_STEPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    // Fire the previously setup steps...
    if (burst_stepping::fire()) {
        if (!PreciseStepping::stopping()) {
            // ...and refresh all axes
            for (std::size_t i = 0; i != SUPPORTED_AXIS_COUNT; i++) {
                refresh_axis(axis_states[i], now, old_tick);
            }
        }
    }
#else
    phase_stepping::spi::finish_transmission();

    if (!PreciseStepping::stopping_relaxed()) {
        ++axis_num_to_refresh;
        if (axis_num_to_refresh == axis_states.size()) {
            axis_num_to_refresh = 0;
        }
        refresh_axis(axis_states[axis_num_to_refresh], now, old_tick);
    }
    if (std::ranges::all_of(axis_states, [](const auto &state) -> bool {
            return !(state.enabled.load(std::memory_order_relaxed) && state.active.load(std::memory_order_relaxed)) || state.missed_tx_cnt == 0;
        })) {
        // Only if all axes have refreshed, we can let tasks to run
        motor_serial_lock_clear_isr_starved();
    }
#endif
}

bool phase_stepping::any_axis_enabled() {
    if (!initialized) {
        // Nothing can be enabled if we are not even initialized.
        return false;
    }

    return std::ranges::any_of(axis_states, [](const auto &state) -> bool {
        return (state.enabled);
    });
}

int phase_stepping::logical_ustep(AxisEnum axis) {
    int mscnt = stepper_axis(axis).MSCNT();
    if (axis >= opts::SUPPORTED_AXIS_COUNT) {
        return mscnt;
    }
    const AxisState &axis_state = axis_states[axis];
    if (!axis_state.enabled) {
        return mscnt;
    }

    // ensure we're not being called while still moving
    debug_assert(!axis_state.has_current_target);
#if HAS_BURST_STEPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    debug_assert(!burst_stepping::busy());
    debug_assert(mscnt == axis_state.driver_phase);
#endif

    return axis_state.last_phase;
}

FORCE_OFAST void phase_stepping::axis_position(const AxisState &axis_state, uint32_t move_epoch, float &out_speed, float &out_position) {
    float epoch = move_epoch / static_cast<float>(TICK_FREQ);
    const MoveTarget &trg = axis_state.current_target;
    out_speed = trg.start_v + 2.f * trg.half_accel * epoch;
    out_position = trg.initial_pos + trg.start_v * epoch + trg.half_accel * epoch * epoch;
}

namespace phase_stepping {
void reset_compensation(AxisEnum axis) {
    axis_states[axis].forward_current.clear();
    axis_states[axis].backward_current.clear();
}

void load() {
    assert_initialized();
    load_from_persistent_storage(AxisEnum::X_AXIS);
    load_from_persistent_storage(AxisEnum::Y_AXIS);
}

/**
 * @brief Used for saving correction to/from a file
 *
 */
struct CorrectionSaveFormat {
    uint8_t reserve[32] {}; // 32 zeroed out bytes to have some room in the future for potential versioning etc (head)
    MotorPhaseCorrection correction;
};

void save_correction_to_file(const CorrectedCurrentLut &lut, const char *file_path) {
    FILE *save_file = fopen(file_path, "wb");
    if (!save_file) {
        debug_assert(false); // should never happen
        return;
    }

    CorrectionSaveFormat save_format { .correction = lut.get_correction_table() };
    [[maybe_unused]] auto written = fwrite(&save_format, 1, sizeof(CorrectionSaveFormat), save_file);
    debug_assert(written == sizeof(CorrectionSaveFormat));

    fclose(save_file);
}

void load_correction_from_file(CorrectedCurrentLut &lut, const char *file_path) {
    FILE *read_file = fopen(file_path, "rb");
    if (!read_file) {
        return; // not an error
    }

    CorrectionSaveFormat save_format {};
    auto read = fread(&save_format, 1, sizeof(CorrectionSaveFormat), read_file);
    if (read == sizeof(CorrectionSaveFormat)) {
        lut.modify_correction_table([&](MotorPhaseCorrection &table) {
            table = save_format.correction;
        });
    }
    fclose(read_file);
}

void save_to_persistent_storage(AxisEnum axis) {
    debug_assert(axis < SUPPORTED_AXIS_COUNT);
    save_to_persistent_storage_without_enabling(axis);
    config_store().set_phase_stepping_enabled(axis, axis_states[axis].enabled);
}

void save_to_persistent_storage_without_enabling(AxisEnum axis) {
    debug_assert(axis < SUPPORTED_AXIS_COUNT);
    save_correction_to_file(axis_states[axis].forward_current, get_correction_file_path(axis, CorrectionType::forward));
    save_correction_to_file(axis_states[axis].backward_current, get_correction_file_path(axis, CorrectionType::backward));
}

void load_from_persistent_storage(AxisEnum axis) {
    debug_assert(axis < SUPPORTED_AXIS_COUNT);
    load_correction_from_file(axis_states[axis].forward_current, get_correction_file_path(axis, CorrectionType::forward));
    load_correction_from_file(axis_states[axis].backward_current, get_correction_file_path(axis, CorrectionType::backward));

    phase_stepping::enable(axis, config_store().get_phase_stepping_enabled(axis));
}

void remove_from_persistent_storage(AxisEnum axis, CorrectionType lut_type) {
    debug_assert(axis < SUPPORTED_AXIS_COUNT);
    if (unlink(get_correction_file_path(axis, lut_type)) != 0) {
        if (errno == ENOENT) {
            return; // file may not exist and that's ok
        }
        bsod("unlink");
    }
}

void remove_from_persistent_storage(AxisEnum axis) {
    remove_from_persistent_storage(axis, phase_stepping::CorrectionType::forward);
    remove_from_persistent_storage(axis, phase_stepping::CorrectionType::backward);
}

void remove_from_persistent_storage() {
    for (int i = 0; i < phase_stepping::opts::SUPPORTED_AXIS_COUNT; i++) {
        remove_from_persistent_storage(static_cast<AxisEnum>(i));
    }
}

} // namespace phase_stepping

// This function is intentionally placed inside the phase stepping source codes
// to allow for inlining and cross-function optimizations without LTO being
// enabled.
extern "C" void PHSTEP_TIMER_ISR_HANDLER(void) {
    // We avoid slow HAL handling on purpose as phase stepping is invoked
    // frequently and every microsecond saves about 4 % of CPU load. That is:
    // - we don't use traceISR_ENTER()/EXIT() as it takes 1.2 µs
    // - we don't use HAL handler: HAL_TIM_IRQHandler(&htim13) as it takes 3 µs
    // - we avoid indirect access to peripheral registers via handle as it takes
    //   0.4 µs
    TIM13->SR &= ~TIM_FLAG_UPDATE;
    phase_stepping::handle_periodic_refresh();
}

// For the same reason as the ISR handler above, we include the
// quick_tmc_spi.cpp, burst_stepper.cpp, axes.cpp and lut.cpp instead of
// compiling them separately:
#if HAS_BURST_STEPPING()
    // #error dead code found by automatic analyses (see BFW-5461)
    #include "burst_stepper.cpp"
#else
    #include "quick_tmc_spi.cpp"
#endif
#include "lut.cpp"
#include "axes.cpp"
#include <bsod/bsod.h>
