#pragma once

#include <option/has_phase_stepping.h>
#include <option/has_burst_stepping.h>

#include "../precise_stepping/fwdecl.hpp"

#if not(HAS_PHASE_STEPPING())
    #include "phase_stepping_dummies.hpp"
#else

    #include "common.hpp"
    #include "lut.hpp"

    #include <libs/circularqueue.h>
    #include <core/types.h>
    #include <bsod.h>

    #include <algorithm>
    #include <memory>
    #include <atomic>
    #include <cassert>

namespace phase_stepping {

struct MoveTarget {
    MoveTarget() = default;
    MoveTarget(float position);
    MoveTarget(const move_t &move, int axis, uint64_t move_duration_ticks);
    MoveTarget(const input_shaper_state_t &is_state, uint64_t move_duration_ticks);

    float initial_pos = 0;
    float half_accel = 0;
    float start_v = 0;
    uint32_t duration = 0; // Movement duration in us

    float target_position() const;
};

struct AxisState {
    AxisState(AxisEnum axis)
        : axis_index(axis) {}

    const int axis_index;

    CorrectedCurrentLut forward_current, backward_current;

    std::atomic<bool> active = false;

    bool inverted = false; // Inverted axis direction flag
    int zero_rotor_phase = 0; // Rotor phase for position 0
    int last_phase = 0; // Last known physical rotor phase
    #if HAS_BURST_STEPPING()
    int driver_phase = 0; // Last known phase the driver uses
    int phase_correction = 0; // Currently applied phase correction
    #endif
    float last_position = 0.f; // Last known logical position
    bool direction = true; // Last non-zero movement direction

    CircularQueue<MoveTarget, 16> pending_targets; // 16 element queue of pre-processed elements

    const move_t *last_processed_move = nullptr;
    uint64_t current_print_time_ticks = 0;

    uint32_t initial_time = 0; // Initial timestamp when the movement start
    std::optional<MoveTarget> target; // Current target to move

    std::atomic<bool> is_moving = false;
    std::atomic<bool> is_cruising = false;

    float initial_hold_multiplier; // Original holding current multiplier
    int32_t initial_count_position = 0; // Value for updating Stepper::count_position
    int32_t initial_count_position_from_startup = 0; // Value for updating Stepper::count_position_from_startup

    uint32_t missed_tx_cnt = 0;
    uint32_t last_timer_tick = 0;

    #if HAS_BURST_STEPPING()
    int original_microsteps = 0;
    bool had_interpolation = false;
    #endif
};

/**
 * Initializes phase stepping. It has to be called before any other phase
 * stepping function.
 **/
void init();

/**
 * Load and enable previous settings, if any.
 **/
void load();

/**
 * Generic function for enabling/disabling axis. Unless needed otherwise, this
 * should be the default way of enabling/disabling it for an axis. When axis is
 * already in desired state, it does nothing.
 **/
void enable(AxisEnum axis_num, bool enable);

/**
 * Enables phase stepping for axis. Reconfigures the motor driver. It is not
 * safe to invoke this procedure within interrupt context. No movement shall be
 * be in progress.
 **/
void enable_phase_stepping(AxisEnum axis_num);

/**
 * Disable phase stepping for axis. Reconfigures the motor driver. It is not
 * safe to invoke this procedure within interrupt context. No movement shall be
 * in progress.
 **/
void disable_phase_stepping(AxisEnum axis_num);

/**
 * Kill immediately all motion on phase-stepped axes. Doesn't perform any
 * ramp-down.
 **/
void stop_immediately();

/**
 * Public interface for PreciseStepping - given a move and other states, setup
 * given axis generator with the move in classical mode.
 **/
void init_step_generator_classic(
    const move_t &move,
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state);

/**
 * Public interface for PreciseStepping - given a move and other states, setup
 * given axis generator with the move in input_shaping mode
 **/
void init_step_generator_input_shaping(
    const move_t &move,
    input_shaper_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state);

/**
 * Public interface pro PreciseStepping - build next step event using classical
 * step generator
 */
step_event_info_t next_step_event_classic(
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state);

/**
 * Public interface pro PreciseStepping - build next step event using input
 * shaping step generator step generator
 */
step_event_info_t next_step_event_input_shaping(
    input_shaper_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state);

/**
 * This function should be invoked periodically at ~90 kHz. It is is safe to
 * invoke this function within interrupt context.
 **/
void handle_periodic_refresh();

/**
 * Return whether any of the axis is in phase stepping mode
 */
bool any_axis_active();

/**
 * Given position, compute coefficient for converting position to motor phase
 **/
int32_t pos_to_phase(int axis, float position);

/**
 * Given position, compute step equivalent
 **/
int32_t pos_to_steps(int axis, float position);

/**
 * Given position, compute planner msteps equivalent
 **/
int32_t pos_to_msteps(int axis, float position);

/**
 * Given position or speed in length unit, return it in revolution units
 **/
float mm_to_rev(int motor, float mm);

/**
 * Given axis, report number of phase steps for single µstep
 */
int phase_per_ustep(int axis);

/**
 * Return a motor step count for given axis
 **/
constexpr int get_motor_steps(AxisEnum axis) {
    if (axis == AxisEnum::X_AXIS || axis == AxisEnum::Y_AXIS) {
    #ifdef HAS_LDO_400_STEP
        return 400;
    #else
        return 200;
    #endif
    }
    return 200;
}

/**
 * Given axis state and time in µs ticks from movement start, compute axis
 * speed and position.
 */
std::tuple<float, float> axis_position(const AxisState &axis_state, uint32_t move_epoch);

/**
 * Extracts physical axis position from logical one
 **/
template <typename Pos>
float extract_physical_position(AxisEnum axis, const Pos &pos) {
    #ifdef COREXY
    if (axis == X_AXIS) {
        return pos[0] + pos[1];
    } else if (axis == Y_AXIS) {
        return pos[0] - pos[1];
    } else if (axis == Z_AXIS) {
        return pos[2];
    } else {
        bsod("Unsupported AXIS");
    }
    #else
    return pos[axis];
    #endif
}

/**
 * Compute the smallest difference between motor phases.
 */
int phase_difference(int a, int b);

    #if HAS_BURST_STEPPING()
/**
 * Given an axis, report the current µstep without phase correction
 */
int logical_ustep(AxisEnum axis);
    #endif

/**
 * A simple wrapper around planner.synchronize() to avoid recursive planner inclusion
 */
void synchronize();

/**
 * This array keeps axis state (and LUT tables) for each axis
 **/
extern std::array<
    std::unique_ptr<AxisState>,
    opts::SUPPORTED_AXIS_COUNT>
    axis_states;

/**
 * Check if given axis is being used for phase stepping or not.
 *
 * Note: This is an in-line definition so this short function can be in-lined
 */
inline bool is_enabled(AxisEnum axis_num) {
    if (axis_num < opts::SUPPORTED_AXIS_COUNT) {
        return axis_states[axis_num]->active;
    }
    return false;
}

/**
 * RAII guard for not changing the state
 */
class EnsureNoChange {
public:
    EnsureNoChange() {
        // This is work-around for not-triggering unused variable warning on the
        // dummy guard usage
        __asm__ __volatile__("nop;\n\t");
    };

    ~EnsureNoChange() {
        // This is work-around for not-triggering unused variable warning on the
        // dummy guard usage
        __asm__ __volatile__("nop;\n\t");
    };
    void release() {};
};

/**
 * Check wether init() has been called
 */
static inline bool initialized() {
    return std::ranges::all_of(axis_states, [](const auto &state) { return state != nullptr; });
}

    /**
     * Ensure init() has been called
     */
    #ifndef _DEBUG
static constexpr void assert_initialized() {}
    #else
void assert_initialized();
    #endif

/**
 * RAII guard for temporary disabling/enabling phase stepping with planner synchronization.
 **/
template <bool ENABLED>
class EnsureState {
    bool released = false;
    bool any_axis_change = false;
    std::array<bool, opts::SUPPORTED_AXIS_COUNT> _prev_active = {};

public:
    EnsureState() {
        assert_initialized();

        any_axis_change = std::ranges::any_of(axis_states, [](const auto &state) -> bool {
            return state->active != ENABLED;
        });

        if (any_axis_change) {
            synchronize();

            for (std::size_t i = 0; i != axis_states.size(); i++) {
                _prev_active[i] = axis_states[i]->active;
                phase_stepping::enable(AxisEnum(i), ENABLED);
            }
        }
    }

    EnsureState(const EnsureState &) = delete;
    EnsureState(const EnsureState &&) = delete;
    EnsureState &operator=(const EnsureState &) = delete;
    EnsureState &operator=(const EnsureState &&) = delete;

    ~EnsureState() {
        release();
    }

    void release() {
        if (released) {
            return;
        }
        released = true;
        if (any_axis_change) {
            synchronize();
            for (std::size_t i = 0; i != axis_states.size(); i++) {
                phase_stepping::enable(AxisEnum(i), _prev_active[i]);
            }
        }
    }
};

using EnsureEnabled = EnsureState<true>;
using EnsureDisabled = EnsureState<false>;
using EnsureSuitableForHoming = std::conditional_t<
    option::has_burst_stepping,
    EnsureNoChange, EnsureDisabled>;

enum class CorrectionType {
    forward,
    backward,
};

constexpr const char *get_correction_file_path(AxisEnum axis, CorrectionType lut_type) {
    switch (axis) {
    case AxisEnum::X_AXIS:
        switch (lut_type) {
        case CorrectionType::forward:
            return "/internal/phase_step_x_fwd";
        case CorrectionType::backward:
            return "/internal/phase_step_x_bck";
        }
        break;
    case AxisEnum::Y_AXIS:
        switch (lut_type) {
        case CorrectionType::forward:
            return "/internal/phase_step_y_fwd";
        case CorrectionType::backward:
            return "/internal/phase_step_y_bck";
        }
        break;
    default:
        break;
    }
    assert(false);
    return "";
}

/**
 * @brief Saves correction to a file
 *
 * @param lut
 * @param file_path
 */
void save_correction_to_file(const CorrectedCurrentLut &lut, const char *file_path);

/**
 * @brief Loads correction from a file
 *
 * @param lut
 * @param file_path
 */
void load_correction_from_file(CorrectedCurrentLut &lut, const char *file_path);

/**
 * @brief Call to save current state into persistent media (ie eeprom/xflash)
 * state == lookup tables, is enabled/disabled
 *
 */
void save_to_persistent_storage(AxisEnum axis);

/**
 * @brief Call to load current state from persistent media (ie eeprom/xflash)
 * state == lookup tables, is enabled/disabled
 *
 */
void load_from_persistent_storage(AxisEnum axis);

/**
 * Return if some processing is still pending.
 */
bool processing();

} // namespace phase_stepping

#endif // if !HAS_PHASE_STEPPING
