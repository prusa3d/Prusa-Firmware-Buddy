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
#include <logging/log.hpp>

#include <cassert>
#include <cmath>

#include <timing_precise.hpp>

LOG_COMPONENT_DEF(PhaseStepping, logging::Severity::debug);

using namespace phase_stepping;
using namespace phase_stepping::opts;
using namespace buddy::hw;

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
    , target_pos(initial_pos)
    , end_time(0) {}

MoveTarget::MoveTarget(float initial_pos, const move_t &move, int axis, const uint64_t move_duration_ticks)
    : initial_pos(initial_pos)
    , duration(uint32_t(move_duration_ticks)) {
    assert(move_duration_ticks <= std::numeric_limits<uint32_t>::max());
    float r = get_move_axis_r(move, axis);
    half_accel = r * float(move.half_accel);
    start_v = r * float(move.start_v);
    target_pos = target_position();
    end_time = move_end_time(move.print_time + move.move_time);
}

MoveTarget::MoveTarget(float initial_pos, const input_shaper_state_t &is_state, const uint64_t move_duration_ticks)
    : initial_pos(initial_pos)
    , half_accel(is_state.half_accel)
    , start_v(is_state.start_v)
    , duration(uint32_t(move_duration_ticks)) {
    assert(move_duration_ticks <= std::numeric_limits<uint32_t>::max());
    target_pos = target_position();
    end_time = move_end_time(is_state.nearest_next_change);
}

float MoveTarget::move_end_time(double end_time) const {
    // To ensure the lock-free behavior of current_target_end_time (and conserve space) we reduce
    // the precision of end_time to float. Since end_time is only used for flushing, the precision
    // is not crucial (no steps are produced) as long as we *guarantee* proper ordering.
    // The value needs to be strictly <= when compared and we do so here.
    float f_end_time = end_time;
    if (f_end_time > end_time) {
        f_end_time = std::nextafterf(f_end_time, 0.f);
    }
    return f_end_time;
}

float MoveTarget::target_position() const {
    float epoch = duration / static_cast<float>(TICK_FREQ);
    return initial_pos + start_v * epoch + half_accel * epoch * epoch;
}

void phase_stepping::init() {
    phase_stepping::initialize_axis_motor_params();
    initialized = true;
}

void phase_stepping::load() {
    assert_initialized();
    load_from_persistent_storage(AxisEnum::X_AXIS);
    load_from_persistent_storage(AxisEnum::Y_AXIS);
}

FORCE_INLINE uint64_t convert_absolute_time_to_ticks(const double time) {
    return uint64_t(time * TICK_FREQ);
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
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    auto &axis_state = *step_generator.phase_step_state;
    const uint8_t axis = step_generator.axis;

    assert(axis_state.pending_targets.isEmpty());

    axis_state.initial_time = step_generator_state.initial_time;

    axis_state.last_position = axis_state.next_target.initial_pos;
    axis_state.current_target = MoveTarget(axis_state.last_position);
    axis_state.current_target_end_time = MAX_PRINT_TIME;

    int32_t initial_steps_made = pos_to_steps(AxisEnum(axis), axis_state.next_target.initial_pos);
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
    const float move_start_pos = extract_physical_position(AxisEnum(axis), move.start_pos);
    axis_state.next_target = MoveTarget(move_start_pos, move, axis, move_duration_ticks);
    axis_state.current_print_time_ticks = next_print_time_ticks;

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
    assert(is_beginning_empty_move(move));

    auto &axis_state = *step_generator.phase_step_state;
    axis_state.active = false;

    // Inherit input shaper initialization...
    input_shaper_step_generator_init(move, step_generator, step_generator_state);

    axis_state.current_print_time_ticks = convert_absolute_time_to_ticks(step_generator.is_state->print_time);

    const uint64_t next_print_time_ticks = calc_move_segment_end_time_in_ticks(*step_generator.is_state);
    const uint64_t move_duration_ticks = next_print_time_ticks - axis_state.current_print_time_ticks;
    axis_state.next_target = MoveTarget(step_generator.is_state->start_pos, *step_generator.is_state, move_duration_ticks);
    axis_state.current_print_time_ticks = next_print_time_ticks;

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

    assert(axis_state.last_processed_move != nullptr);
    assert(axis_state.last_processed_move->reference_cnt != 0);

    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };
    if (axis_state.pending_targets.isFull()) {
        next_step_event.time = axis_state.current_target_end_time;
        next_step_event.status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_PENDING;
    } else if (const move_t *next_move = PreciseStepping::move_segment_queue_next_move(*axis_state.last_processed_move); next_move != nullptr) {
        next_step_event.time = next_move->print_time;

        const uint8_t axis = axis_state.axis_index;
        const float move_start_pos = extract_physical_position(AxisEnum(axis), next_move->start_pos);

        // push the buffered target
        axis_state.next_target.target_pos = move_start_pos;
        axis_state.pending_targets.enqueue(axis_state.next_target);

        // buffer the next
        if (!is_ending_empty_move(*next_move)) {
            const uint64_t next_print_time_ticks = calc_move_segment_end_time_in_ticks(*next_move);
            const uint64_t move_duration_ticks = next_print_time_ticks - axis_state.current_print_time_ticks;
            axis_state.next_target = MoveTarget(move_start_pos, *next_move, axis, move_duration_ticks);
            axis_state.current_print_time_ticks = next_print_time_ticks;

            const int32_t target_steps = pos_to_steps(AxisEnum(axis), axis_state.next_target.target_pos);
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

    assert(step_generator.is_state != nullptr);

    step_event_info_t next_step_event = { std::numeric_limits<double>::max(), 0, STEP_EVENT_INFO_STATUS_GENERATED_INVALID };
    if (axis_state.pending_targets.isFull()) {
        next_step_event.time = axis_state.current_target_end_time;
        next_step_event.status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_PENDING;
    } else {
        next_step_event.time = step_generator.is_state->nearest_next_change;

        if (input_shaper_state_update(*step_generator.is_state, step_generator.axis)) {
            uint8_t axis = axis_state.axis_index;
            const float move_start_pos = step_generator.is_state->start_pos;

            // push the buffered target
            axis_state.next_target.target_pos = move_start_pos;
            axis_state.pending_targets.enqueue(axis_state.next_target);

            // buffer the next
            if (step_generator.is_state->nearest_next_change < MAX_PRINT_TIME) {
                const uint64_t next_print_time_ticks = calc_move_segment_end_time_in_ticks(*step_generator.is_state);
                const uint64_t move_duration_ticks = next_print_time_ticks - axis_state.current_print_time_ticks;
                axis_state.next_target = MoveTarget(move_start_pos, *step_generator.is_state, move_duration_ticks);
                axis_state.current_print_time_ticks = next_print_time_ticks;

                const int32_t target_steps = pos_to_steps(AxisEnum(axis), axis_state.next_target.target_pos);
                step_generator_state.current_distance[axis] = target_steps;

                next_step_event.flags |= STEP_EVENT_FLAG_KEEP_ALIVE;
                next_step_event.status = STEP_EVENT_INFO_STATUS_GENERATED_KEEP_ALIVE;
            }
        }

        PreciseStepping::move_segment_processed_handler();
    }

    return next_step_event;
}

#ifdef _DEBUG
void phase_stepping::assert_initialized() {
    // This is explicitly kept non-inline to serve as a single trap point
    assert(initialized);
}

void phase_stepping::assert_disabled() {
    // This is explicitly kept non-inline to serve as a single trap point
    assert(!any_axis_enabled());
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
        assert(!burst_stepping::busy());

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

bool phase_stepping::processing() {
    if (!initialized) {
        // processing() can be called early during app_setup(), before init() actually gets called.
        return false;
    }

    // check for pending targets
    for (auto &state : axis_states) {
        if (!state.pending_targets.isEmpty() || state.current_target.has_value()) {
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

    auto &axis_state = axis_states[axis];
    assert(axis_state.pending_targets.isEmpty() && !axis_state.current_target.has_value());

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
    TMCStepperType &stepper = stepper_axis(axis_num);
    auto &axis_state = axis_states[axis_num];
    assert(!axis_state.enabled && !axis_state.active);
    assert(!axis_state.current_target.has_value() && axis_state.pending_targets.isEmpty());

    axis_state.last_position = 0;
    axis_state.direction = Stepper::motor_direction(axis_num);

    // switch off interpolation first to ensure position is settled
    axis_state.had_interpolation = stepper.intpol();
    stepper.intpol(false);

    // switch microsteps and fetch MSCNT at full resolution
    axis_state.original_microsteps = stepper.microsteps();
    stepper.microsteps(256);
    int current_phase = stepper.MSCNT();

    // We initialize the zero rotor phase to current phase. The real initialization is done by
    // set_phase_origin() when the local coordinate system is effectively initialized.
    axis_state.zero_rotor_phase = current_phase;
    axis_state.last_phase = current_phase;

#if HAS_BURST_STEPPING()
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

    // Read axis configuration and cache it so we can access it fast
    axis_state.inverted = Stepper::is_axis_inverted(axis_num);

    // Sync the counters just before enabling the axis
    int32_t initial_steps_made = pos_to_steps(axis_num, axis_state.last_position);
    axis_state.initial_count_position = Stepper::get_axis_steps(axis_num) - initial_steps_made;
    axis_state.initial_count_position_from_startup = Stepper::get_axis_steps_from_startup(axis_num) - initial_steps_made;

    axis_state.missed_tx_cnt = 0;
    axis_state.enabled = true;
    axis_state.active = true;

    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types |= enable_mask;

    HAL_TIM_Base_Start_IT(&TIM_HANDLE_FOR(phase_stepping));
}

#if !HAS_BURST_STEPPING()
static inline void single_step_axis(AxisEnum axis) {
    switch (axis) {
    case X_AXIS:
    #if ENABLED(SQUARE_WAVE_STEPPING)
        X_STEP_SET();
    #else
        X_STEP_SET();
        delay_us_precise<MINIMUM_STEPPER_PULSE>();
        X_STEP_RESET();
    #endif
        break;
    case Y_AXIS:
    #if ENABLED(SQUARE_WAVE_STEPPING)
        Y_STEP_SET();
    #else
        Y_STEP_SET();
        delay_us_precise<MINIMUM_STEPPER_PULSE>();
        Y_STEP_RESET();
    #endif
        break;
    case Z_AXIS:
    #if ENABLED(SQUARE_WAVE_STEPPING)
        Z_STEP_SET();
    #else
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

void phase_stepping::disable_phase_stepping(AxisEnum axis_num) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);
    assert(!planner.processing());

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    auto &stepper = static_cast<TMC2130Stepper &>(stepper_axis(axis_num));
    auto &axis_state = axis_states[axis_num];

    axis_state.active = false;
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

    // Disable and shutdown timer if we're the last axis
    axis_state.enabled = false;
    if (!any_axis_enabled()) {
        HAL_TIM_Base_Stop_IT(&TIM_HANDLE_FOR(phase_stepping));
    }
}

void phase_stepping::enable(AxisEnum axis_num, bool enable) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);
    assert_initialized();

    auto &axis_state = axis_states[axis_num];
    if (axis_state.enabled == enable) {
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

void phase_stepping::clear_targets() {
    for (auto &axis_state : axis_states) {
        bool was_active = axis_state.active;
        axis_state.active = false;

        axis_state.current_target.reset();
        while (!axis_state.pending_targets.isEmpty()) {
            axis_state.pending_targets.dequeue();
        }

        axis_state.active = was_active;
    }
}

// Given axis and speed, return current adjustment expressed as range <0, 255>
[[maybe_unused]] static int current_adjustment(int /*axis*/, float speed) {
    speed = std::abs(speed);
#if PRINTER_IS_PRUSA_XL()
    float BREAKPOINT = 6.f;
    float ENDPOINT = 10.f;
    int REDUCTION_TO = 150;
#elif PRINTER_IS_PRUSA_iX() // TODO simple copy-paste of XL values. To be removed as soon as iX values are measured
    float BREAKPOINT = 6.f;
    float ENDPOINT = 10.f;
    int REDUCTION_TO = 150;
#elif PRINTER_IS_PRUSA_COREONE() // TODO simple copy-paste of XL values. To be removed as soon as iX values are measured
    float BREAKPOINT = 20.f;
    float ENDPOINT = 10.f;
    int REDUCTION_TO = 255;
#else
    #error "Unsupported printer"
#endif
    if (speed < BREAKPOINT) {
        return 255;
    }
    if (speed > ENDPOINT) {
        return REDUCTION_TO;
    }
    return 255 - (speed - BREAKPOINT) * (255 - REDUCTION_TO) / (ENDPOINT - BREAKPOINT);
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

    while (!axis_state.current_target.has_value() || move_epoch > axis_state.current_target->duration) {
        if (axis_state.current_target.has_value()) {
            axis_state.initial_time += axis_state.current_target->duration;
            move_position = axis_state.current_target->target_pos;
            axis_state.current_target.reset();
        }

        if (!axis_state.pending_targets.isEmpty()) {
            // Pull new movement
            const auto current_target = axis_state.pending_targets.dequeue();
            axis_state.current_target = current_target;

            axis_state.current_target_end_time = current_target.end_time;

            axis_state.is_cruising = (current_target.half_accel == 0) && (current_target.duration > 10'000);
            axis_state.is_moving = true;

            move_position = current_target.initial_pos;
            move_epoch = ticks_diff(now, axis_state.initial_time);
        } else {
            // No new movement
            axis_state.is_cruising = false;
            axis_state.is_moving = false;
            break;
        }
    }

    auto [speed, position] = axis_state.current_target.has_value()
        ? axis_position(axis_state, move_epoch)
        : std::make_tuple(0.f, move_position);

    float physical_position = resolve_axis_inversion(axis_state.inverted, position);
    float physical_speed = resolve_axis_inversion(axis_state.inverted, speed);

    if (physical_speed != 0.f) {
        // update the direction in order to fetch the correct lut
        axis_state.direction = physical_speed > 0;
    }
    const auto &current_lut = resolve_current_lut(axis_state);

    int new_phase = normalize_motor_phase(pos_to_phase(axis_enum, physical_position) + axis_state.zero_rotor_phase);
    assert(phase_difference(axis_state.last_phase, new_phase) < 256);

#if HAS_BURST_STEPPING()
    int phase_correction = current_lut.get_phase_shift(new_phase);
    int shifted_phase = normalize_motor_phase(new_phase + phase_correction);
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
        int32_t steps_made = pos_to_steps(axis_enum, position);
        Stepper::set_axis_steps(axis_enum, axis_state.initial_count_position + steps_made);
        Stepper::set_axis_steps_from_startup(axis_enum, axis_state.initial_count_position_from_startup + steps_made);

        // flag axis movement (if any)
        if (physical_speed != 0.f) {
            Stepper::report_axis_movement(axis_enum, physical_speed);
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
    uint32_t now = ticks_us() + REFRESH_PERIOD_US;

    // always refresh the last_timer_tick
    uint32_t old_tick = last_timer_tick;
    last_timer_tick = now;

#if HAS_BURST_STEPPING()
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

    if (!PreciseStepping::stopping()) {
        ++axis_num_to_refresh;
        if (axis_num_to_refresh == axis_states.size()) {
            axis_num_to_refresh = 0;
        }
        refresh_axis(axis_states[axis_num_to_refresh], now, old_tick);
    }
#endif
}

bool phase_stepping::any_axis_enabled() {
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
    assert(!axis_state.current_target.has_value());
#if HAS_BURST_STEPPING()
    assert(!burst_stepping::busy());
    assert(mscnt == axis_state.driver_phase);
#endif

    return axis_state.last_phase;
}

FORCE_OFAST std::tuple<float, float> phase_stepping::axis_position(const AxisState &axis_state, uint32_t move_epoch) {
    float epoch = move_epoch / static_cast<float>(TICK_FREQ);
    const MoveTarget &trg = *axis_state.current_target;
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
    config_store().set_phase_stepping_enabled(axis, axis_states[axis].enabled);
}

void save_to_persistent_storage_without_enabling(AxisEnum axis) {
    assert(axis < SUPPORTED_AXIS_COUNT);
    save_correction_to_file(axis_states[axis].forward_current, get_correction_file_path(axis, CorrectionType::forward));
    save_correction_to_file(axis_states[axis].backward_current, get_correction_file_path(axis, CorrectionType::backward));
}

void load_from_persistent_storage(AxisEnum axis) {
    assert(axis < SUPPORTED_AXIS_COUNT);
    load_correction_from_file(axis_states[axis].forward_current, get_correction_file_path(axis, CorrectionType::forward));
    load_correction_from_file(axis_states[axis].backward_current, get_correction_file_path(axis, CorrectionType::backward));

    phase_stepping::enable(axis, config_store().get_phase_stepping_enabled(axis));
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
    #include "burst_stepper.cpp"
#else
    #include "quick_tmc_spi.cpp"
#endif
#include "lut.cpp"
#include "axes.cpp"
