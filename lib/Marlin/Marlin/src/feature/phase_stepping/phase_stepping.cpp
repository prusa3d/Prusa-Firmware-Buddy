#include "phase_stepping.hpp"
#include "quick_tmc_spi.hpp"
#include "../precise_stepping/precise_stepping.hpp"
#include "../precise_stepping/internal.hpp"
#include <device/peripherals.h>
#include <module/motion.h>
#include <module/stepper.h>
#include <module/stepper/trinamic.h>
#include <hwio.h>

#include <cassert>
#include <cmath>

#include <TMCStepper.h>

#include "log.h"
LOG_COMPONENT_DEF(PhaseStepping, LOG_SEVERITY_DEBUG);

using namespace phase_stepping;
using namespace buddy::hw;

// Global definitions
std::array<
    std::unique_ptr<AxisState>,
    SUPPORTED_AXIS_COUNT>
    phase_stepping::axis_states = { { nullptr, nullptr } };

// Module definitions
static unsigned int axis_num_to_refresh = 0;
static const std::array<OutputPin, SUPPORTED_AXIS_COUNT> cs_pins = { { xCs, yCs } };

MoveTarget::MoveTarget(float position)
    : initial_pos(position)
    , half_accel(0)
    , start_v(0)
    , duration(0) {}

MoveTarget::MoveTarget(const move_t &move, int axis) {
    float r = get_move_axis_r(move, axis);
    initial_pos = extract_physical_position(AxisEnum(axis), move.start_pos);
    half_accel = r * float(move.half_accel);
    start_v = r * float(move.start_v);
    duration = move.move_t * 1'000'000;
}

MoveTarget::MoveTarget(const input_shaper_state_t &is_state)
    : initial_pos(is_state.start_pos)
    , half_accel(is_state.half_accel)
    , start_v(is_state.start_v)
    , duration(1'000'000 * (is_state.nearest_next_change - is_state.print_time)) {}

float MoveTarget::target_position() const {
    float epoch = duration / 1000000.f;
    return initial_pos + start_v * epoch + half_accel * epoch * epoch;
}

void phase_stepping::init() {
    phase_stepping::axis_states[0].reset(new AxisState(AxisEnum::X_AXIS));
    phase_stepping::axis_states[1].reset(new AxisState(AxisEnum::Y_AXIS));
}

static void init_step_generator_internal(
    const move_t &move,
    move_segment_step_generator_t &step_generator,
    step_generator_state_t & /*step_generator_state*/) {
    auto &axis_state = *step_generator.phase_step_state;
    const uint8_t axis = step_generator.axis;

    assert(axis_state.pending_targets.isEmpty());

    axis_state.initial_time = ticks_us();

    // Adjust rotor phase such that when we virtually move, we don't move
    // physically
    float last_target_position = axis_state.last_position;
    float new_start_position = axis_state.inverted
        ? axis_state.target->initial_pos
        : -axis_state.target->initial_pos;
    axis_state.zero_rotor_phase = normalize_motor_phase(
        +axis_state.zero_rotor_phase
        - pos_to_phase(axis, new_start_position)
        + pos_to_phase(axis, last_target_position));

    axis_state.last_position = new_start_position;
    axis_state.last_processed_move = &move;
    axis_state.last_processed_event_time = move.print_time;

    int32_t initial_steps_made = pos_to_steps(AxisEnum(axis), new_start_position);
    axis_state.initial_count_position = Stepper::get_axis_steps(AxisEnum(axis)) - initial_steps_made;
    axis_state.initial_count_position_from_startup = Stepper::get_axis_steps_from_startup(AxisEnum(axis)) - initial_steps_made;

    axis_state.active = true;
}

void phase_stepping::init_step_generator_classic(
    const move_t &move,
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    auto &axis_state = *step_generator.phase_step_state;
    axis_state.active = false;

    const uint8_t axis = step_generator.axis;
    axis_state.target = MoveTarget(move, axis);
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)next_step_event_classic;

    init_step_generator_internal(move, step_generator, step_generator_state);
}

void phase_stepping::init_step_generator_input_shaping(
    const move_t &move,
    input_shaper_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state) {
    auto &axis_state = *step_generator.phase_step_state;
    axis_state.active = false;

    // Inherit input shaper initialization...
    input_shaper_step_generator_init(move, step_generator, step_generator_state);
    axis_state.target = MoveTarget(*step_generator.is_state);

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

    move_t *next_move = PreciseStepping::move_segment_queue_next_move(*axis_state.last_processed_move);
    StepEventInfoStatus status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_INVALID;
    if (axis_state.pending_targets.isFull()) {
        status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_PENDING;
    } else if (next_move != nullptr) {
        // PreciseStepping generates and infinite move segment on motion stop.
        // We cannot enqueue such segment
        if (!is_ending_empty_move(*next_move)) {
            uint8_t axis = axis_state.axis_index;

            auto new_target = MoveTarget(*next_move, axis);
            float target_pos = new_target.target_position();
            if (axis_state.inverted) {
                target_pos = -target_pos;
            }
            int32_t target_steps = pos_to_steps(AxisEnum(axis), target_pos);
            PreciseStepping::step_generator_state.current_distance[axis] = target_steps;

            axis_state.pending_targets.enqueue(new_target);

            status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_VALID;
            axis_state.last_processed_event_time = next_move->print_time + next_move->move_t;
        } else {
            status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_PENDING;
        }
        axis_state.last_processed_move = next_move;
    }
    return step_event_info_t {
        .time = axis_state.last_processed_event_time,
        .flags = static_cast<StepEventFlag_t>(STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis),
        .status = status
    };
}

static bool has_ending_segment(const input_shaper_state_t &is_state) {
    return is_state.nearest_next_change - is_state.print_time >= 1000000.;
}

step_event_info_t phase_stepping::next_step_event_input_shaping(
    input_shaper_step_generator_t &step_generator,
    step_generator_state_t & /*step_generator_state*/) {
    AxisState &axis_state = *step_generator.phase_step_state;

    StepEventInfoStatus status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_INVALID;
    if (axis_state.pending_targets.isFull()) {
        status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_PENDING;
    } else {
        bool generated_new = input_shaper_state_update(*step_generator.is_state, step_generator.axis);
        if (generated_new && !has_ending_segment(*step_generator.is_state)) {
            uint8_t axis = axis_state.axis_index;

            auto new_target = MoveTarget(*step_generator.is_state);
            float target_pos = new_target.target_position();
            if (axis_state.inverted) {
                target_pos = -target_pos;
            }
            int32_t target_steps = pos_to_steps(AxisEnum(axis), target_pos);
            PreciseStepping::step_generator_state.current_distance[axis] = target_steps;

            axis_state.active = false;
            axis_state.pending_targets.enqueue(new_target);
            axis_state.active = true;
            status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_GENERATED_VALID;
        }
        if (has_ending_segment(*step_generator.is_state) && !axis_state.pending_targets.isEmpty()) {
            // When the move target queue is not empty, we cannot yield final step
            status = StepEventInfoStatus::STEP_EVENT_INFO_STATUS_PENDING;
        } else {
            axis_state.last_processed_event_time = step_generator.is_state->nearest_next_change;
        }
        PreciseStepping::move_segment_processed_handler();
    }

    return step_event_info_t {
        .time = axis_state.last_processed_event_time,
        .flags = static_cast<StepEventFlag_t>(STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis),
        .status = status
    };
}

void phase_stepping::enable_phase_stepping(AxisEnum axis_num) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    auto &stepper = static_cast<TMC2130Stepper &>(stepper_axis(axis_num));
    auto &axis_state = *axis_states[axis_num];

    // In order to start phase stepping, we have to set phase currents that are
    // in sync with current position, and then switch the driver to current
    // mode.
    int current_phase = stepper.MSCNT();

    auto [a, b] = axis_state.forward_current.get_current(current_phase);

    // Swapping coils isn't a mistake - TMC in Xdirect mode swaps coils
    stepper.coil_A(b);
    stepper.coil_B(a);
    stepper.direct_mode(true);

    // We initialize the zero rotor phase to current phase. The following move
    // segment to come will be move segment to zero, let's prepare for that.
    axis_state.zero_rotor_phase = current_phase;
    axis_state.last_phase = current_phase;
    axis_state.last_position = 0;
    axis_state.target = MoveTarget(0);

    // Read axis configuration and cache it so we can access it fast
    if (axis_num == AxisEnum::X_AXIS) {
        axis_state.inverted = INVERT_X_DIR;
    } else if (axis_num_to_refresh == AxisEnum::Y_AXIS) {
        axis_state.inverted = INVERT_Y_DIR;
    } else if (axis_num_to_refresh == AxisEnum::Z_AXIS) {
        axis_state.inverted = INVERT_Z_DIR;
    }

    axis_state.active = true;
    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types |= enable_mask;

    HAL_TIM_Base_Start_IT(&TIM_HANDLE_FOR(phase_stepping));
    HAL_TIM_OC_Start_IT(&TIM_HANDLE_FOR(phase_stepping), TIM_CHANNEL_1);
}

void phase_stepping::disable_phase_stepping(AxisEnum axis_num) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    auto &stepper = static_cast<TMC2130Stepper &>(stepper_axis(axis_num));
    auto &axis_state = *axis_states[axis_num];

    axis_state.active = false;
    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types &= ~enable_mask;

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
            // Why is zStep with lower case initial letter and YStep with upper case?
            zStep.toggle();
            break;
        default:
            break;
        }
        delayMicroseconds(20);
    }
    stepper.direct_mode(false);
    stepper.microsteps(original_microsteps);

    if (!any_axis_active()) {
        HAL_TIM_OC_Stop_IT(&TIM_HANDLE_FOR(phase_stepping), TIM_CHANNEL_1);
        HAL_TIM_Base_Stop_IT(&TIM_HANDLE_FOR(phase_stepping));
    }
}

void phase_stepping::enable(AxisEnum axis_num, bool enable) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);
    auto &axis_state = axis_states[axis_num];
    if (axis_state->active == enable) {
        return;
    }
    if (enable) {
        phase_stepping::enable_phase_stepping(axis_num);
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
static int current_adjustment(int /*axis*/, float speed) {
    speed = std::abs(speed);
#if PRINTER_IS_PRUSA_XL
    float BREAKPOINT = 4.7f;
    float ENDPOINT = 8.f;
    int ENDPOINT_REDUCTION = 100;

    if (speed < BREAKPOINT) {
        return 255;
    }
    if (speed > ENDPOINT) {
        return ENDPOINT_REDUCTION;
    }
    return 255 - (speed - BREAKPOINT) * (255 - ENDPOINT_REDUCTION) / (ENDPOINT - BREAKPOINT);
#endif

    bsod("Unsupported printer");
}

int phase_stepping::phase_difference(int a, int b) {
    int direct_diff = (a - b + MOTOR_PERIOD) % MOTOR_PERIOD;
    int cyclic_diff = MOTOR_PERIOD - direct_diff;
    return std::min(direct_diff, cyclic_diff);
}

__attribute__((optimize("-Ofast"))) void phase_stepping::handle_periodic_refresh() {
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

    // Move to next axis
    ++axis_num_to_refresh;
    if (axis_num_to_refresh == axis_states.size()) {
        axis_num_to_refresh = 0;
    }

    AxisState &axis_state = *axis_states[axis_num_to_refresh];

    if (!axis_state.active) {
        return;
    }

    uint32_t now = ticks_us();
    uint32_t move_epoch = ticks_diff(now, axis_state.initial_time);

    while (!axis_state.target.has_value() || move_epoch > axis_state.target->duration) {
        uint32_t time_overshoot = 0;
        if (axis_state.target.has_value()) {
            time_overshoot = ticks_diff(move_epoch, axis_state.target->duration);
            axis_state.target.reset();
        }

        if (!axis_state.pending_targets.isEmpty()) {
            // Pull new movement
            axis_state.target = axis_state.pending_targets.dequeue();

            axis_state.is_cruising = axis_state.target->half_accel == 0 && axis_state.target->duration > 10'000;
            axis_state.is_moving = true;

            // Time overshoots accounts for the lost time in the previous state
            axis_state.initial_time = now - time_overshoot;
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
        : std::make_tuple(0.f, (axis_state.inverted ? axis_state.last_position : -axis_state.last_position));
    if (!axis_state.inverted) {
        position = -position;
        speed = -speed;
    }

    int new_phase = normalize_motor_phase(pos_to_phase(axis_num_to_refresh, position) + axis_state.zero_rotor_phase);
    assert(phase_difference(axis_state.last_phase, new_phase) < 256);

    // Report movement to Stepper
    int32_t steps_made = pos_to_steps(position, axis_num_to_refresh);
    Stepper::set_axis_steps(AxisEnum(axis_num_to_refresh),
        axis_state.initial_count_position + steps_made);
    Stepper::set_axis_steps_from_startup(AxisEnum(axis_num_to_refresh),
        axis_state.initial_count_position_from_startup + steps_made);

    const auto &current_lut = speed > 0
        ? axis_state.forward_current
        : axis_state.backward_current;
    auto [a, b] = current_lut.get_current(new_phase);
    int c_adj = current_adjustment(axis_num_to_refresh, mm_to_rev(axis_num_to_refresh, speed));
    a = a * c_adj / 255;
    b = b * c_adj / 255;

    axis_state.last_position = position;
    axis_state.last_phase = new_phase;

    auto ret = spi::set_xdirect(axis_num_to_refresh, a, b);
    if (ret == HAL_OK) {
        axis_state.missed_tx_cnt = 0;
    } else {
        axis_state.missed_tx_cnt++;
    }

    if (axis_state.missed_tx_cnt > 5000) {
        bsod("Phase stepping: TX failure");
    }
}

bool phase_stepping::any_axis_active() {
    for (auto &state : axis_states) {
        if (state->active) {
            return true;
        }
    }
    return false;
}

__attribute__((optimize("-Ofast"))) int32_t phase_stepping::pos_to_phase(int axis, float position) {
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

__attribute__((optimize("-Ofast"))) int32_t phase_stepping::pos_to_steps(int axis, float position) {
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

__attribute__((optimize("-Ofast"))) int32_t pos_to_msteps(int axis, float position) {
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

__attribute__((optimize("-Ofast"))) float phase_stepping::mm_to_rev(int motor, float mm) {
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

__attribute__((optimize("-Ofast"))) std::tuple<float, float> phase_stepping::axis_position(const AxisState &axis_state, uint32_t move_epoch) {
    float epoch = move_epoch / 1000000.f;
    const MoveTarget &trg = *axis_state.target;
    return {
        trg.start_v + 2.f * trg.half_accel * epoch,
        trg.initial_pos + trg.start_v * epoch + trg.half_accel * epoch * epoch
    };
}
