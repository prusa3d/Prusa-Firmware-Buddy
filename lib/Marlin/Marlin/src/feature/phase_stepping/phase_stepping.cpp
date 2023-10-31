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
std::array<AxisState, SUPPORTED_AXIS_COUNT> phase_stepping::axis_states;

// Module definitions
static int axis_num_to_refresh = 0;
static const std::array< OutputPin, SUPPORTED_AXIS_COUNT > cs_pins = {{ xCs, yCs }};

MoveTarget::MoveTarget(double position):
    initial_pos(position), half_accel(0), start_v(0), duration(0)
{}

MoveTarget::MoveTarget(const move_t& move, int axis) {
    double r = get_move_axis_r(move, axis);
    initial_pos = extract_physical_position(AxisEnum(axis), move.start_pos);
    half_accel = r * move.half_accel;
    start_v = r * move.start_v;
    duration = move.move_t * 1'000'000;
}

void phase_stepping::init_step_generator(
    const move_t &cmove,
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state)
{
    auto& axis_state = *step_generator.phase_step_state;
    auto& move = const_cast< move_t& >(cmove); // Meeh, hacky! TBA: Reconsider changing contract
    axis_state.active = false;

    const uint8_t axis = step_generator.axis;
    step_generator_state.step_generator[axis] = &step_generator;
    step_generator_state.next_step_func[axis] = (generator_next_step_f)next_step_event;

    axis_state.initial_time = ticks_us();
    // The initialization move segment is always at position 0, so we have to
    // adjust the zero rotor phase
    axis_state.last_position = 0;
    axis_state.zero_rotor_phase = axis_state.last_phase;
    axis_state.target = MoveTarget(move, axis);
    axis_state.current_move = &move;
    axis_state.last_processed_move = &move;
    mark_ownership(move);

    axis_state.active = true;
}

step_event_info_t phase_stepping::next_step_event(
    move_segment_step_generator_t& step_generator,
    step_generator_state_t &/*step_generator_state*/)
{
    AxisState& axis_state = *step_generator.phase_step_state;

    assert(axis_state.last_processed_move != nullptr);

    move_t *next_move = PreciseStepping::move_segment_queue_next_move(*axis_state.last_processed_move);
    bool new_move_enqueued = false;
    if (!axis_state.pending_moves.full() && next_move != nullptr) {
        // PreciseStepping generates and infinite move segment on motion stop.
        // We cannot enqueue such segment
        if (!is_ending_empty_move(*next_move)) {
            axis_state.pending_moves.push(next_move);
            mark_ownership(*next_move);
        }
        axis_state.last_processed_move = next_move;
        new_move_enqueued = true;
    }

    move_t& last_move = *axis_state.last_processed_move;
    return step_event_info_t{
                .time = last_move.print_time + last_move.move_t,
                .flags = STEP_EVENT_FLAG_X_ACTIVE << step_generator.axis,
                .status = new_move_enqueued
                            ? STEP_EVENT_INFO_STATUS_GENERATED_VALID
                            : STEP_EVENT_INFO_STATUS_GENERATED_INVALID };
}


void phase_stepping::enable_phase_stepping(AxisEnum axis_num) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    auto& stepper = static_cast< TMC2130Stepper& >(stepper_axis(axis_num));
    auto& axis_state = axis_states[axis_num];

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
    axis_state.current_move = nullptr;
    axis_state.target = MoveTarget(0);

    // Read axis configuration and cache it so we can access it fast
    if (axis_num == AxisEnum::X_AXIS)
        axis_state.inverted = INVERT_X_DIR;
    else if (axis_num_to_refresh == AxisEnum::Y_AXIS)
        axis_state.inverted = INVERT_Y_DIR;
    else if (axis_num_to_refresh == AxisEnum::Z_AXIS)
        axis_state.inverted = INVERT_Z_DIR;

    axis_state.active = true;
    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types |= enable_mask;

    HAL_TIM_Base_Start_IT(&TIM_HANDLE_FOR(phase_stepping));
    HAL_TIM_OC_Start_IT(&TIM_HANDLE_FOR(phase_stepping), TIM_CHANNEL_1);
}

void phase_stepping::disable_phase_stepping(AxisEnum axis_num) {
    assert(axis_num < SUPPORTED_AXIS_COUNT);

    // We know that PHASE_STEPPING is enabled only on TMC2130 boards
    auto& stepper = static_cast<TMC2130Stepper&>(stepper_axis(axis_num));
    auto& axis_state = axis_states[axis_num];

    axis_state.active = false;
    auto enable_mask = PHASE_STEPPING_GENERATOR_X << axis_num;
    PreciseStepping::physical_axis_step_generator_types &= ~enable_mask;

    // In order to avoid glitch in motor motion, we have to first, make steps to
    // get MSCNT into sync and then we disable XDirect mode

    int original_microsteps = stepper.microsteps();
    stepper.microsteps(256);
    int current_phase = normalize_motor_phase(axis_state.last_phase);
    while (current_phase != stepper.MSCNT() ) {
        switch(axis_num) {
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
    auto& axis_state = axis_states[axis_num];
    if (axis_state.active == enable)
        return;
    if (enable)
        phase_stepping::enable_phase_stepping(axis_num);
    else
        phase_stepping::disable_phase_stepping(axis_num);
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
    if (axis_num_to_refresh == axis_states.size())
        axis_num_to_refresh = 0;

    AxisState& axis_state = axis_states[axis_num_to_refresh];

    if (!axis_state.active) {
        return;
    }

    uint32_t now = ticks_us();
    uint32_t move_epoch = now - axis_state.initial_time;

    while (move_epoch > axis_state.target.duration) {
        uint32_t time_overshoot = 0;

        if (axis_state.current_move != nullptr) {
            discard_ownership(*axis_state.current_move);
            axis_state.current_move = nullptr;
            // We just overshot; if there is pending segment we should continue
            // with it, otherwise it means there was a delay and we should start
            // with current the time
            time_overshoot = move_epoch - axis_state.target.duration;
        }

        if (!axis_state.pending_moves.empty()) {
            // Pull new movement
            axis_state.current_move = axis_state.pending_moves.pop();
            axis_state.target = MoveTarget(*axis_state.current_move, axis_num_to_refresh);
            axis_state.initial_time = now - time_overshoot;
            move_epoch = time_overshoot;
        } else {
            // Stay in position
            move_epoch = axis_state.target.duration;
            break;
        }
    }

    double position = axis_position(axis_state, move_epoch);
    if (!axis_state.inverted)
        position = -position;
    axis_state.last_phase = pos_to_phase(axis_num_to_refresh, position) + axis_state.zero_rotor_phase;

    // TBA report movement to Stepper - we don't do it at the moment as
    // Stepper::count_direction, Stepper::count_direction_from_startup and
    // Stepper::count_direction doesn't seem to be used anywhere. And we would
    // just burn CPU cycles on keeping them updated.
    const auto& current_lut = position > axis_state.last_position
        ? axis_state.forward_current
        : axis_state. backward_current;
    auto [a, b] = current_lut.get_current(axis_state.last_phase);

    axis_state.last_position = position;

    auto ret = spi::set_xdirect(axis_num_to_refresh, a, b);
    if (ret == HAL_OK)
        axis_state.missed_tx_cnt = 0;
    else {
        axis_state.missed_tx_cnt++;
    }

    if (axis_state.missed_tx_cnt > 5000)
        bsod("Phase stepping: TX failure");
}

bool phase_stepping::any_axis_active() {
    for ( auto& state : axis_states ) {
        if (state.active)
            return true;
    }
    return false;
}

__attribute__((optimize("-Ofast"))) double phase_stepping::pos_to_phase(int axis, double position) {
    return position * (256.0 * 80.0 / 16.0); // TBA take into account actual printer config

    // The following original version took 3.2µs to compute:

    // static constexpr int STEPS_PER_UNIT[] = DEFAULT_AXIS_STEPS_PER_UNIT;
    // static constexpr int MICROSTEPS[] = { X_MICROSTEPS, Y_MICROSTEPS, Z_MICROSTEPS };

    // assert(axis >= 0 && axis <= 2);

    // return position * 256 * STEPS_PER_UNIT[axis] / MICROSTEPS[axis];
}

__attribute__((optimize("-Ofast"))) double phase_stepping::axis_position(const AxisState& axis_state, uint32_t move_epoch) {
    double epoch = move_epoch / 1000000.0;
    const MoveTarget& trg = axis_state.target;
    return trg.initial_pos + trg.start_v * epoch + trg.half_accel * epoch * epoch;
}
