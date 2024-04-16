#include "phase_stepping.hpp"
#include "debug_util.hpp"

#if HAS_BURST_STEPPING()
    #include "burst_stepper.hpp"
#else
    #include "quick_tmc_spi.hpp"
#endif

#include "../precise_stepping/precise_stepping.hpp"
#include "../precise_stepping/internal.hpp"
#include "../input_shaper/input_shaper.hpp"
#include <config_store/store_instance.hpp>

#include <device/peripherals.h>
#include <module/motion.h>
#include <module/stepper.h>
#include <module/stepper/trinamic.h>
#include <trinamic.h>
#include <TMCStepper.h>

#include <Pin.hpp>
#include <log.h>

#include <cassert>
#include <cmath>

LOG_COMPONENT_DEF(PhaseStepping, LOG_SEVERITY_DEBUG);

using namespace phase_stepping;
using namespace phase_stepping::opts;
using namespace buddy::hw;

// Global definitions
std::array<
    std::unique_ptr<AxisState>,
    SUPPORTED_AXIS_COUNT>
    phase_stepping::axis_states = { { nullptr, nullptr } };

// Module definitions
#if !HAS_BURST_STEPPING()
static uint_fast8_t axis_num_to_refresh = 0;
#endif
static uint32_t last_timer_tick = 0;

MoveTarget::MoveTarget(float position)
    : initial_pos(position)
    , half_accel(0)
    , start_v(0)
    , duration(0)
    , target(position) {}

MoveTarget::MoveTarget(const move_t &move, int axis, const uint64_t move_duration_ticks) {
    assert(move_duration_ticks <= std::numeric_limits<uint32_t>::max());
    float r = get_move_axis_r(move, axis);
    initial_pos = extract_physical_position(AxisEnum(axis), move.start_pos);
    half_accel = r * float(move.half_accel);
    start_v = r * float(move.start_v);
    duration = uint32_t(move_duration_ticks);
    target = target_position();
}

MoveTarget::MoveTarget(const input_shaper_state_t &is_state, const uint64_t move_duration_ticks)
    : initial_pos(is_state.start_pos)
    , half_accel(is_state.half_accel)
    , start_v(is_state.start_v)
    , duration(uint32_t(move_duration_ticks)) {
    target = target_position();
    assert(move_duration_ticks <= std::numeric_limits<uint32_t>::max());
}

float MoveTarget::target_position() const {
    float epoch = duration / 1000000.f;
    return initial_pos + start_v * epoch + half_accel * epoch * epoch;
}

void phase_stepping::init() {
    phase_stepping::axis_states[0].reset(new AxisState(AxisEnum::X_AXIS));
    phase_stepping::axis_states[1].reset(new AxisState(AxisEnum::Y_AXIS));
}

void phase_stepping::load() {
    load_from_persistent_storage(AxisEnum::X_AXIS);
    load_from_persistent_storage(AxisEnum::Y_AXIS);
}

FORCE_INLINE uint64_t convert_absolute_time_to_ticks(const double time) {
    return uint64_t(time * 1'000'000.);
}

FORCE_INLINE uint64_t calc_move_segment_end_time_in_ticks(const move_t &move) {
    return convert_absolute_time_to_ticks(move.print_time + move.move_time);
}

FORCE_INLINE uint64_t calc_move_segment_end_time_in_ticks(const input_shaper_state_t &is_state) {
    return convert_absolute_time_to_ticks(is_state.nearest_next_change);
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
    const move_t &move,
    move_segment_step_generator_t &step_generator,
    step_generator_state_t & /*step_generator_state*/) {
    auto &axis_state = *step_generator.phase_step_state;
    const uint8_t axis = step_generator.axis;

    assert(axis_state.pending_targets.isEmpty());

    axis_state.initial_time = ticks_us();

    axis_state.last_position = axis_state.target->initial_pos;
    axis_state.last_processed_move = &move;

    int32_t initial_steps_made = pos_to_steps(AxisEnum(axis), axis_state.target->initial_pos);
    axis_state.initial_count_position = Stepper::get_axis_steps(AxisEnum(axis)) - initial_steps_made;
    axis_state.initial_count_position_from_startup = Stepper::get_axis_steps_from_startup(AxisEnum(axis)) - initial_steps_made;

    axis_state.active = true;
}

void phase_stepping::init_step_generator_classic(
    const move_t &move,
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    assert(is_beginning_empty_move(move));

    auto &axis_state = *step_generator.phase_step_state;
    axis_state.active = false;

    const uint8_t axis = step_generator.axis;
    axis_state.current_print_time_ticks = convert_absolute_time_to_ticks(move.print_time);

    const uint64_t next_print_time_ticks = calc_move_segment_end_time_in_ticks(move);
    const uint64_t move_duration_ticks = next_print_time_ticks - axis_state.current_print_time_ticks;
    axis_state.target = MoveTarget(move, axis, move_duration_ticks);
    axis_state.current_print_time_ticks = next_print_time_ticks;

    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)next_step_event_classic;

    // We are keeping a reference to a move segment in last_processed_move.
    // Otherwise, a move segment can be discarded while we are using it.
    move.reference_cnt += 1;

    init_step_generator_internal(move, step_generator, step_generator_state);
}

void phase_stepping::init_step_generator_input_shaping(
    const move_t &move,
    input_shaper_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    assert(is_beginning_empty_move(move));

    auto &axis_state = *step_generator.phase_step_state;
    axis_state.active = false;

    // Inherit input shaper initialization...
    input_shaper_step_generator_init(move, step_generator, step_generator_state);

    axis_state.current_print_time_ticks = convert_absolute_time_to_ticks(step_generator.is_state->print_time);

    const uint64_t next_print_time_ticks = calc_move_segment_end_time_in_ticks(*step_generator.is_state);
    const uint64_t move_duration_ticks = next_print_time_ticks - axis_state.current_print_time_ticks;
    axis_state.target = MoveTarget(*step_generator.is_state, move_duration_ticks);
    axis_state.current_print_time_ticks = next_print_time_ticks;

    // ...and then override next_step_func with phase stepping one
    const uint8_t axis = step_generator.axis;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)next_step_event_input_shaping;

    init_step_generator_internal(move, step_generator, step_generator_state);
}

step_event_info_t phase_stepping::next_step_event_classic(
    move_segment_step_generator_t &step_generator,
    step_generator_state_t & /*step_generator_state*/) {
    AxisState &axis_state = *step_generator.phase_step_state;

    assert(axis_state.last_processed_move != nullptr);
    assert(axis_state.last_processed_move->reference_cnt != 0);

    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };
    if (axis_state.pending_targets.isFull()) {
        next_step_event.time = axis_state.last_processed_move->print_time + axis_state.last_processed_move->move_time;
        next_step_event.status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_PENDING;
    } else if (const move_t *next_move = PreciseStepping::move_segment_queue_next_move(*axis_state.last_processed_move); next_move != nullptr) {
        next_step_event.time = next_move->print_time;

        if (!is_ending_empty_move(*next_move)) {
            uint8_t axis = axis_state.axis_index;

            const uint64_t next_print_time_ticks = calc_move_segment_end_time_in_ticks(*next_move);
            const uint64_t move_duration_ticks = next_print_time_ticks - axis_state.current_print_time_ticks;
            auto new_target = MoveTarget(*next_move, axis, move_duration_ticks);
            axis_state.current_print_time_ticks = next_print_time_ticks;

            float target_pos = new_target.target;
            int32_t target_steps = pos_to_steps(AxisEnum(axis), target_pos);
            PreciseStepping::step_generator_state.current_distance[axis] = target_steps;

            axis_state.active = false;
            axis_state.pending_targets.enqueue(new_target);
            axis_state.active = true;

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
    step_generator_state_t & /*step_generator_state*/) {
    AxisState &axis_state = *step_generator.phase_step_state;

    assert(step_generator.is_state != nullptr);

    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };
    if (axis_state.pending_targets.isFull()) {
        next_step_event.time = step_generator.is_state->nearest_next_change;
        next_step_event.status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_PENDING;
    } else {
        next_step_event.time = step_generator.is_state->nearest_next_change;

        if (const bool is_updated = input_shaper_state_update(*step_generator.is_state, step_generator.axis);
            is_updated && step_generator.is_state->nearest_next_change < MAX_PRINT_TIME) {

            uint8_t axis = axis_state.axis_index;

            const uint64_t next_print_time_ticks = calc_move_segment_end_time_in_ticks(*step_generator.is_state);
            const uint64_t move_duration_ticks = next_print_time_ticks - axis_state.current_print_time_ticks;
            auto new_target = MoveTarget(*step_generator.is_state, move_duration_ticks);
            axis_state.current_print_time_ticks = next_print_time_ticks;

            float target_pos = new_target.target;
            int32_t target_steps = pos_to_steps(AxisEnum(axis), target_pos);
            PreciseStepping::step_generator_state.current_distance[axis] = target_steps;

            axis_state.active = false;
            axis_state.pending_targets.enqueue(new_target);
            axis_state.active = true;

            next_step_event.flags |= STEP_EVENT_FLAG_KEEP_ALIVE;
            next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_KEEP_ALIVE;
        }

        PreciseStepping::move_segment_processed_handler();
    }

    return next_step_event;
}

#ifdef _DEBUG
void phase_stepping::assert_initialized() {
    // This is explicitly kept non-inline to serve as a single trap point
    assert(initialized());
}
#endif

void phase_stepping::synchronize() {
    planner.synchronize();
}

bool phase_stepping::processing() {
    // check for pending targets
    for (auto &state : axis_states) {
        if (state && (!state->pending_targets.isEmpty() || state->target.has_value())) {
            return true;
        }
    }

    // ensure the last target has also been flushed
#if HAS_BURST_STEPPING()
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

void phase_stepping::set_phase_origin(AxisEnum axis, float pos) {
    assert(axis < SUPPORTED_AXIS_COUNT);
    assert_initialized();

    auto &axis_state = *axis_states[axis];
    bool was_active = axis_state.active;
    axis_state.active = false;

    float inverted_position = resolve_axis_inversion(axis_state.inverted, pos);
    axis_state.zero_rotor_phase = normalize_motor_phase(-pos_to_phase(axis, inverted_position) + axis_state.last_phase);
    axis_state.last_position = pos;

    axis_state.active = was_active;
}

void phase_stepping::enable_phase_stepping(AxisEnum axis_num) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);
    assert(!planner.has_blocks_queued() && !PreciseStepping::processing());

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    auto &stepper = static_cast<TMC2130Stepper &>(stepper_axis(axis_num));
    auto &axis_state = *axis_states[axis_num];
    assert(!axis_state.active && !axis_state.target.has_value() && axis_state.pending_targets.isEmpty());

    axis_state.last_position = 0;
    axis_state.direction = true; // TODO: should use last_direction_bits

#if HAS_BURST_STEPPING()
    axis_state.original_microsteps = stepper.microsteps();
    axis_state.last_phase = axis_state.zero_rotor_phase = axis_state.driver_phase = stepper.MSCNT();
    axis_state.phase_correction = 0;
    axis_state.had_interpolation = stepper.intpol();
    stepper.intpol(false);
    stepper.microsteps(256);
#else
    // In order to start phase stepping, we have to set phase currents that are
    // in sync with current position, and then switch the driver to current
    // mode.
    int current_phase = stepper.MSCNT();

    axis_state.last_currents = resolve_current_lut(axis_state).get_current(current_phase);

    // Set IHOLD to be the same as IRUN (as IHOLD is always used in XDIRECT)
    axis_state.initial_hold_multiplier = stepper.hold_multiplier();
    stepper.rms_current(stepper.rms_current(), 1.);

    // Swapping coils isn't a mistake - TMC in Xdirect mode swaps coils
    stepper.coil_A(axis_state.last_currents.b);
    stepper.coil_B(axis_state.last_currents.a);
    stepper.direct_mode(true);

    // We initialize the zero rotor phase to current phase. The real initialization is done by
    // set_phase_origin() when the local coordinate system is effectively initialized.
    axis_state.zero_rotor_phase = current_phase;
    axis_state.last_phase = current_phase;
#endif
    // Read axis configuration and cache it so we can access it fast
    if (axis_num == AxisEnum::X_AXIS) {
        axis_state.inverted = INVERT_X_DIR;
    } else if (axis_num == AxisEnum::Y_AXIS) {
        axis_state.inverted = INVERT_Y_DIR;
    } else if (axis_num == AxisEnum::Z_AXIS) {
        axis_state.inverted = INVERT_Z_DIR;
    }

    // Sync the counters just before enabling the axis
    int32_t initial_steps_made = pos_to_steps(axis_num, axis_state.last_position);
    axis_state.initial_count_position = Stepper::get_axis_steps(axis_num) - initial_steps_made;
    axis_state.initial_count_position_from_startup = Stepper::get_axis_steps_from_startup(axis_num) - initial_steps_made;

    axis_state.missed_tx_cnt = 0;
    axis_state.active = true;

    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types |= enable_mask;

    HAL_TIM_Base_Start_IT(&TIM_HANDLE_FOR(phase_stepping));
}

void phase_stepping::disable_phase_stepping(AxisEnum axis_num) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);
    assert(!planner.processing());

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    auto &stepper = static_cast<TMC2130Stepper &>(stepper_axis(axis_num));
    auto &axis_state = *axis_states[axis_num];

    axis_state.active = false;
    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types &= ~enable_mask;

#if HAS_BURST_STEPPING()
    stepper.microsteps(axis_state.original_microsteps);
    stepper.intpol(axis_state.had_interpolation);
#else
    // In order to avoid glitch in motor motion, we have to first, make steps to
    // get MSCNT into sync and then we disable XDirect mode
    int original_microsteps = stepper.microsteps();
    stepper.microsteps(256);
    int current_phase = normalize_motor_phase(axis_state.last_phase);
    while (current_phase != stepper.MSCNT()) {
        switch (axis_num) {
        case 0:
            XStep->toggle();
            break;
        case 1:
            YStep->toggle();
            break;
        case 2:
            zStep.toggle();
            break;
        default:
            break;
        }
        delay_us_precise(20);
    }
    stepper.direct_mode(false);
    stepper.microsteps(original_microsteps);
#endif

    // Reset IHOLD to the original state
    stepper.rms_current(stepper.rms_current(), axis_state.initial_hold_multiplier);

    if (!any_axis_active()) {
        HAL_TIM_Base_Stop_IT(&TIM_HANDLE_FOR(phase_stepping));
    }
}

void phase_stepping::enable(AxisEnum axis_num, bool enable) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);
    assert_initialized();

    auto &axis_state = axis_states[axis_num];
    if (axis_state->active == enable) {
        return;
    }
    if (enable) {
        // Enable phase stepping and reset PS to update the phase origin
        phase_stepping::enable_phase_stepping(axis_num);
        PreciseStepping::reset_from_halt();
    } else {
        phase_stepping::disable_phase_stepping(axis_num);
    }
}

void phase_stepping::stop_immediately() {
    for (auto &axis_state : axis_states) {
        bool was_active = axis_state->active;
        axis_state->active = false;

        axis_state->target.reset();
        while (!axis_state->pending_targets.isEmpty()) {
            axis_state->pending_targets.dequeue();
        }

        axis_state->active = was_active;
    }
}

// Given axis and speed, return current adjustment expressed as range <0, 255>
[[maybe_unused]] static int current_adjustment(int /*axis*/, float speed) {
    speed = std::abs(speed);
#if PRINTER_IS_PRUSA_XL
    float BREAKPOINT = 6.f;
    float ENDPOINT = 10.f;
    int REDUCTION_TO = 150;

    if (speed < BREAKPOINT) {
        return 255;
    }
    if (speed > ENDPOINT) {
        return REDUCTION_TO;
    }
    return 255 - (speed - BREAKPOINT) * (255 - REDUCTION_TO) / (ENDPOINT - BREAKPOINT);
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

    static constexpr uint REFRESH_PERIOD_US = 1'000'000 / REFRESH_FREQ;
    static constexpr uint UPDATE_DURATION_US = 20; // Rather pesimistic update

    uint32_t refresh_period = ticks_diff(now, last_timer_tick);
    return refresh_period < 2 * REFRESH_PERIOD_US - UPDATE_DURATION_US;
}

static FORCE_INLINE FORCE_OFAST void refresh_axis(
    AxisState &axis_state, uint32_t now, uint32_t previous_tick) {
    if (!axis_state.active) {
        return;
    }

    [[maybe_unused]] const auto axis_index = axis_state.axis_index;
    [[maybe_unused]] const auto axis_enum = AxisEnum(axis_state.axis_index);

    if (!is_refresh_period_sane(now, previous_tick)) {
        // If the ISR handler was delayed, we don't have enough time to process
        // the update. Abort the update so we can catch up.
        mark_missed_transaction(axis_state);
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

    while (!axis_state.target.has_value() || move_epoch > axis_state.target->duration) {
        uint32_t time_overshoot = 0;
        if (axis_state.target.has_value()) {
            time_overshoot = ticks_diff(move_epoch, axis_state.target->duration);
            move_position = axis_state.target->target;
            axis_state.target.reset();
        }

        if (!axis_state.pending_targets.isEmpty()) {
            // Pull new movement
            axis_state.target = axis_state.pending_targets.dequeue();

            axis_state.is_cruising = axis_state.target->half_accel == 0 && axis_state.target->duration > 10'000;
            axis_state.is_moving = true;

            // Time overshoots accounts for the lost time in the previous state
            axis_state.initial_time = now - time_overshoot;
            move_position = axis_state.target->initial_pos;
            move_epoch = time_overshoot;
        } else {
            // No new movement
            axis_state.is_cruising = false;
            axis_state.is_moving = false;
            break;
        }
    }

    auto [speed, position] = axis_state.target.has_value()
        ? axis_position(axis_state, move_epoch)
        : std::make_tuple(0.f, move_position);

    float physical_position = resolve_axis_inversion(axis_state.inverted, position);
    float physical_speed = resolve_axis_inversion(axis_state.inverted, speed);

    if (physical_speed != 0.f) {
        // update the direction in order to fetch the correct lut
        axis_state.direction = physical_speed > 0;
    }
    const auto &current_lut = resolve_current_lut(axis_state);

    int new_phase = normalize_motor_phase(pos_to_phase(axis_index, physical_position) + axis_state.zero_rotor_phase);
    assert(phase_difference(axis_state.last_phase, new_phase) < 256);

#if HAS_BURST_STEPPING()
    axis_state.phase_correction = current_lut.get_phase_shift(new_phase);
    int shifted_phase = normalize_motor_phase(new_phase + axis_state.phase_correction);
    int steps_diff = phase_difference(shifted_phase, axis_state.driver_phase);
    burst_stepping::set_phase_diff(axis_enum, steps_diff);
    axis_state.driver_phase = shifted_phase;
#else
    auto new_currents = current_lut.get_current(new_phase);
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
        int32_t steps_made = pos_to_steps(axis_index, position);
        Stepper::set_axis_steps(axis_enum, axis_state.initial_count_position + steps_made);
        Stepper::set_axis_steps_from_startup(axis_enum, axis_state.initial_count_position_from_startup + steps_made);

        // flag axis movement (if any)
        if (speed != 0.f) {
            Stepper::report_axis_movement(axis_enum, speed);
        }

        axis_state.last_position = position;
        axis_state.last_phase = new_phase;
    }

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

    static constexpr uint REFRESH_PERIOD_US = 1'000'000 / REFRESH_FREQ;
    uint32_t now = ticks_us() + REFRESH_PERIOD_US;

    // always refresh the last_timer_tick
    uint32_t old_tick = last_timer_tick;
    last_timer_tick = now;

#if HAS_BURST_STEPPING()
    // Fire the previously setup steps...
    burst_stepping::fire();

    // ...and refresh all axes
    for (std::size_t i = 0; i != SUPPORTED_AXIS_COUNT; i++) {
        refresh_axis(*axis_states[i], now, old_tick);
    }
#else
    phase_stepping::spi::finish_transmission();

    ++axis_num_to_refresh;
    if (axis_num_to_refresh == axis_states.size()) {
        axis_num_to_refresh = 0;
    }
    refresh_axis(*axis_states[axis_num_to_refresh], now, old_tick);
#endif
}

bool phase_stepping::any_axis_active() {
    return std::ranges::any_of(axis_states, [](const auto &state) -> bool {
        return (state && state->active);
    });
}

int32_t phase_stepping::pos_to_phase(int axis, float position) {
    static constinit std::array<float, SUPPORTED_AXIS_COUNT> FACTORS = []() consteval {
        static_assert(SUPPORTED_AXIS_COUNT <= 3);

        int STEPS_PER_UNIT[] = DEFAULT_AXIS_STEPS_PER_UNIT;
        int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };

        std::array<float, SUPPORTED_AXIS_COUNT> ret;
        for (int i = 0; i != SUPPORTED_AXIS_COUNT; i++) {
            ret[i] = 256.f * STEPS_PER_UNIT[i] / MICROSTEPS[i];
        }
        return ret;
    }();
    return normalize_motor_phase(position * FACTORS[axis]);
}

int32_t phase_stepping::pos_to_steps(int axis, float position) {
    static constinit std::array<float, SUPPORTED_AXIS_COUNT> FACTORS = []() consteval {
        static_assert(SUPPORTED_AXIS_COUNT <= 3);

        int STEPS_PER_UNIT[] = DEFAULT_AXIS_STEPS_PER_UNIT;

        std::array<float, SUPPORTED_AXIS_COUNT> ret;
        for (int i = 0; i != SUPPORTED_AXIS_COUNT; i++) {
            ret[i] = float(STEPS_PER_UNIT[i]);
        }
        return ret;
    }();
    return position * FACTORS[axis];
}

int32_t pos_to_msteps(int axis, float position) {
    static constinit std::array<float, SUPPORTED_AXIS_COUNT> FACTORS = []() consteval {
        static_assert(SUPPORTED_AXIS_COUNT <= 3);

        int STEPS_PER_UNIT[] = DEFAULT_AXIS_STEPS_PER_UNIT;

        std::array<float, SUPPORTED_AXIS_COUNT> ret;
        for (int i = 0; i != SUPPORTED_AXIS_COUNT; i++) {
            ret[i] = float(STEPS_PER_UNIT[i]) / PLANNER_STEPS_MULTIPLIER;
        }
        return ret;
    }();
    return position * FACTORS[axis];
}

float phase_stepping::mm_to_rev(int motor, float mm) {
    static constinit std::array<float, SUPPORTED_AXIS_COUNT> FACTORS = []() consteval {
        static_assert(SUPPORTED_AXIS_COUNT <= 3);

        int STEPS_PER_UNIT[] = DEFAULT_AXIS_STEPS_PER_UNIT;
        int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };

        std::array<float, SUPPORTED_AXIS_COUNT> ret;
        for (int i = 0; i != SUPPORTED_AXIS_COUNT; i++) {
            ret[i] = 1.f / (get_motor_steps(AxisEnum(i)) * float(MICROSTEPS[i]) / float(STEPS_PER_UNIT[i]));
        }
        return ret;
    }();
    return mm * FACTORS[motor];
}

int phase_stepping::phase_per_ustep(int axis) {
    static constinit std::array<int, SUPPORTED_AXIS_COUNT> FACTORS = []() consteval {
        static_assert(SUPPORTED_AXIS_COUNT <= 3);

        int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };

        std::array<int, SUPPORTED_AXIS_COUNT> ret;
        for (int i = 0; i != SUPPORTED_AXIS_COUNT; i++) {
            ret[i] = 256 / MICROSTEPS[i];
        }
        return ret;
    }();
    return FACTORS[axis];
}

#if HAS_BURST_STEPPING()
int phase_stepping::logical_ustep(AxisEnum axis) {
    int mscnt = stepper_axis(axis).MSCNT();
    if (axis >= opts::SUPPORTED_AXIS_COUNT) {
        return mscnt;
    }
    const AxisState &axis_state = *axis_states[axis];
    if (!axis_state.active) {
        return mscnt;
    }

    // ensure we're not being called while still moving
    assert(!axis_state.target.has_value());
    assert(!burst_stepping::busy());

    return normalize_motor_phase(mscnt - axis_state.phase_correction);
}
#endif

FORCE_OFAST std::tuple<float, float> phase_stepping::axis_position(const AxisState &axis_state, uint32_t move_epoch) {
    float epoch = move_epoch / 1000000.f;
    const MoveTarget &trg = *axis_state.target;
    return {
        trg.start_v + 2.f * trg.half_accel * epoch,
        trg.initial_pos + trg.start_v * epoch + trg.half_accel * epoch * epoch
    };
}

namespace phase_stepping {
namespace {
    /**
     * @brief Used for saving correction to/from a file
     *
     */
    struct CorrectionSaveFormat {
        uint8_t reserve[32] {}; // 32 zeroed out bytes to have some room in the future for potential versioning etc (head)
        MotorPhaseCorrection correction;
    };
} // namespace

void save_correction_to_file(const CorrectedCurrentLut &lut, const char *file_path) {
    FILE *save_file = fopen(file_path, "wb");
    if (!save_file) {
        assert(false); // should never happen
        return;
    }

    CorrectionSaveFormat save_format { .correction = lut.get_correction() };
    [[maybe_unused]] auto written = fwrite(&save_format, 1, sizeof(CorrectionSaveFormat), save_file);
    assert(written == sizeof(CorrectionSaveFormat));

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
        lut.modify_correction([&](MotorPhaseCorrection &table) {
            table = save_format.correction;
        });
    }
    fclose(read_file);
}

void save_to_persistent_storage(AxisEnum axis) {
    assert(axis < SUPPORTED_AXIS_COUNT);
    save_to_persistent_storage_without_enabling(axis);
    config_store().set_phase_stepping_enabled(axis, axis_states[axis]->active);
}

void save_to_persistent_storage_without_enabling(AxisEnum axis) {
    assert(axis < SUPPORTED_AXIS_COUNT);
    save_correction_to_file(axis_states[axis]->forward_current, get_correction_file_path(axis, CorrectionType::forward));
    save_correction_to_file(axis_states[axis]->backward_current, get_correction_file_path(axis, CorrectionType::backward));
}

void load_from_persistent_storage(AxisEnum axis) {
    assert(axis < SUPPORTED_AXIS_COUNT);
    phase_stepping::enable(axis, config_store().get_phase_stepping_enabled(axis));

    load_correction_from_file(axis_states[axis]->forward_current, get_correction_file_path(axis, CorrectionType::forward));
    load_correction_from_file(axis_states[axis]->backward_current, get_correction_file_path(axis, CorrectionType::backward));
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
// quick_tmc_spi.cpp, burst_stepper.cpp and lut.cpp instead of compiling them
// separately:
#if HAS_BURST_STEPPING()
    #include "burst_stepper.cpp"
#else
    #include "quick_tmc_spi.cpp"
#endif
#include "lut.cpp"
