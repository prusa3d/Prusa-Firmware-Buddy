#pragma once

#include <option/has_phase_stepping.h>
#include <option/has_burst_stepping.h>

#include "../precise_stepping/fwdecl.hpp"

#if not(HAS_PHASE_STEPPING())
    #include "phase_stepping_dummies.hpp"
#else

    #include "common.hpp"
    #include "lut.hpp"
    #include "../../core/time_ticks.hpp"

    #include <utils/atomic_circular_queue.hpp>
    #include <core/types.h>
    #include <bsod.h>

    #include <algorithm>
    #include <atomic>
    #include <optional>

namespace phase_stepping {

struct MoveTarget {
    MoveTarget() = default;
    MoveTarget(float initial_pos);
    MoveTarget(float initial_pos, const move_t &move, int axis, uint64_t move_duration_ticks);
    MoveTarget(float initial_pos, const input_shaper_state_t &is_state, uint64_t move_duration_ticks);

    float initial_pos = 0;
    float half_accel = 0;
    float start_v = 0;
    uint32_t duration = 0; // Movement duration in us
    float target_pos = 0;

private:
    float target_position() const;
};

struct AxisState {
    AxisState(AxisEnum axis)
        : axis_index(axis)
        , enabled(false)
        , active(false) {}

    const int axis_index;
    std::atomic<bool> enabled = false; // Axis enabled for this axis
    std::atomic<bool> active = false; // Phase stepping interrupt active

    CorrectedCurrentLut forward_current, backward_current;

    bool inverted = false; // Inverted axis direction flag
    float offset = 0; // Physical offset to apply to targets
    int last_phase = 0; // Last known physical rotor phase
    #if HAS_BURST_STEPPING()
                        // #error dead code found by automatic analyses (see BFW-5461)
    int driver_phase = 0; // Last known phase the driver uses
    #else
    CoilCurrents last_currents; // Currently applied coil currents
    #endif
    float last_position = 0.f; // Last known logical position
    const move_t *last_processed_move = nullptr; // Move reference when using classic stepping
    bool direction = true; // Last non-zero physical movement direction

    uint64_t current_print_time_us = 0;
    uint32_t initial_time = 0; // Initial timestamp when the movement start

    // As move_t is being processed by the step generator its data is buffered into next_target.
    // Once complete it is pushed to pending_targets. When current_target is empty, the refresh loop
    // will dequeue items from pending_targets and set it as the current_target. When the position
    // is reached the cycle repeats, until no more targets are present and current_target is reset.
    // Note: these two are not std::optional<MoveTarget> for performance reasons
    // (std::optional adds overhead in the ISR hot path).
    MoveTarget current_target; // Current target to move
    bool has_current_target = false;
    AtomicCircularQueue<MoveTarget, unsigned, 32> pending_targets; // queue of pre-processed elements
    /// Synchronization primitive to allow phase stepping to "steal" the next target.
    ///
    /// The scenario is, the next target is "owned" by the stepper interrupt,
    /// where it is being prepared and then inserted into the pending_targets
    /// queue. By that, it passes to the ownership of the phase stepping
    /// interrupt. But under some circumstances, the phase stepping interrupt
    /// needs to "steal" this move early, before passing it through the pending
    /// targets.
    ///
    /// What we use for this to work:
    ///
    /// * There's the `state` companion variable. That one works somewhat as an
    ///   atomic lock with the `updating` state - when the thing is being
    ///   written, that one is locked (combination with acquire/release
    ///   orderings).
    /// * Whenever anyone wants to take it out, it is replaced (with acquire)
    ///   with empty. That person atomically gains ownership and is allowed to
    ///   read it.
    /// * It can't be overwritten at that time, because:
    ///   - Only the stepping interrupt writes. If that one acquired the
    ///     ownership, it'll not write before it reads first.
    ///   - The phase stepping interrupt has higher priority. That means that if
    ///     that one acquries ownership, it will not get interrupted by the
    ///     stepping interrupt before it finishes reading simply because the
    ///     scheduling mechanism.
    class StealableTarget {
    private:
        enum class State {
            empty,
            full,
            updating,
        };
        MoveTarget value;
        std::atomic<State> state = State::empty;
        static_assert(decltype(state)::is_always_lock_free);

    public:
        /// Overwrites the value.
        ///
        /// To be called from the stepping interrupt.
        void set(const MoveTarget &v) {
            // We don't really _need_ the old value, but unfortunately, acquire
            // memory order needs to be on an operation that reads.
            [[maybe_unused]] State old = state.exchange(State::updating, std::memory_order_acquire);
            debug_assert(old != State::updating);
            value = v;
            state.store(State::full, std::memory_order_release);
        }
        /// Have a look at the value without locking/acquiring.
        ///
        /// Used during initialization within the stepper interrupt, no need to
        /// protect against anything / synchronize in any way - it only reads
        /// and only the stepper interrupt writes (and it doesn't care if the
        /// value was logically "consumed" or not).
        const MoveTarget &peek() const {
            return value;
        }
        /// Extract the value the "proper" way.
        ///
        /// To be called from the stepping interrupt.
        std::optional<MoveTarget> take() {
            State old = state.exchange(State::empty, std::memory_order_acquire);
            switch (old) {
            case State::empty:
                return std::nullopt;
            case State::full:
                return value;
            case State::updating:
                bsod_unreachable();
            }
            bsod_unreachable();
        }
        /// Steal the value.
        ///
        /// This is to be called by the phase stepping interrupt.
        ///
        /// This can return nullopt either when the value is empty or when it
        /// is being updated. The latter means a slight delay (likely until our
        /// next iteration of the interrupt) and we are probably going to get a
        /// proper move by then. So we _are_ moving forward and aren't stalled.
        std::optional<MoveTarget> steal() {
            State old = State::full;
            if (state.compare_exchange_strong(old, State::empty, std::memory_order_acquire, std::memory_order_relaxed)) {
                // Successfully replaced full with empty -> it's ours.
                //
                // We can copy it out, we are in higher-priority interrupt and
                // it the owner won't touch it now.
                return value;
            } else {
                // There was either empty (nothing to steal) or updating (not a
                // consistent state -> we don't want garbage and we can wait,
                // because we'll get something soon through the usual path).
                return std::nullopt;
            }
        }
    };
    StealableTarget next_target; // Next planned target to move

    TimeTicks next_target_end_time; // Absolute end time for the next planned target
    std::atomic<bool> is_cruising = false;

    float initial_hold_multiplier; // Original holding current multiplier
    int32_t initial_count_position = 0; // Value for updating Stepper::count_position
    int32_t initial_count_position_from_startup = 0; // Value for updating Stepper::count_position_from_startup

    uint32_t missed_tx_cnt = 0;
    uint32_t last_timer_tick = 0;

    int original_microsteps = 0;
    bool had_interpolation = false;

    bool is_slowed_down = true;
    unsigned stalled_for = 0;
    float previous_speed = 0.0f;
};

/**
 * @brief Initializes internal phase stepping state.
 * It has to be called before any other phase stepping function.
 **/
void init();

/**
 * @brief Directly jump to the given position without any ramping motion.
 * @param axis_enum Axis to move
 * @param pos Physical position to move to
 * @param set_origin Alter the origin so that the given position doesn't move
 *
 * Command the motor to move at the given physical position. The rotor must be at standstill and
 * already close to the intended position in order not to skip. When set_origin is true alter the
 * origin instead for the motor not to move at the given position.
 */
void jump_to_position(AxisEnum axis_num, float pos, bool set_origin);

/**
 * @brief Generic function for enabling/disabling axis.
 *
 * Unless needed otherwise, this should be the default way of enabling/disabling it for an axis.
 * When axis is already in desired state, it does nothing.
 *
 * At least one call to jump_to_position() is required to set the starting position prior to enable
 * one axis.
 *
 **/
void enable(AxisEnum axis_num, bool enable);

/**
 * Clear any current and all pending targets, stopping all motion on
 * phase-stepped axes. Doesn't perform any ramp-down.
 **/
void clear_targets();

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
 * Return whether any axis is in phase stepping mode
 */
bool any_axis_enabled();

/**
 * Given axis state and time in µs ticks from movement start, compute axis
 * speed and position.
 */
void axis_position(const AxisState &axis_state, uint32_t move_epoch, float &out_speed, float &out_position);

/**
 * Extracts physical axis position from logical one
 **/
template <typename Pos>
float extract_physical_position(AxisEnum axis, const Pos &pos) {
    #ifdef COREXY
    if (axis == X_AXIS) {
        return static_cast<float>(pos[0] + pos[1]);
    } else if (axis == Y_AXIS) {
        return static_cast<float>(pos[0] - pos[1]);
    } else if (axis == Z_AXIS) {
        return static_cast<float>(pos[2]);
    } else {
        bsod("Unsupported AXIS");
    }
    #else
    return static_cast<float>(pos[axis]);
    #endif
}

/**
 * Compute the smallest difference between motor phases.
 */
int phase_difference(int a, int b);

/**
 * Given an axis, report the current µstep without phase correction
 */
int logical_ustep(AxisEnum axis);

/**
 * A simple wrapper around planner.synchronize() to avoid recursive planner inclusion
 */
void synchronize();

/**
 * Return if some processing is still pending.
 */
bool processing();

/**
 * Return if a motion target is queued or being consumed.
 *
 * Narrower than processing(): does not count in-flight SPI/burst DMA traffic
 * that can occur without any commanded motion (e.g. holding-current rescale).
 */
bool motion_pending();

/**
 * Check phase stepping internal state
 * NOTE: To be called while idle!
 */
    #ifndef _DEBUG
static constexpr void check_state() {}
    #else
void check_state();
    #endif

/**
 * This array keeps axis state (and LUT tables) for each axis
 **/
extern std::array<AxisState, opts::SUPPORTED_AXIS_COUNT> axis_states;

/**
 * Returns whether init() has been called
 */
bool is_initialized();

    /**
     * Ensure init() has been called
     */
    #ifndef _DEBUG
static constexpr void assert_initialized() {}
    #else
void assert_initialized();
    #endif

    /**
     * Ensure phase stepping is fully disabled on all axes
     */
    #ifndef _DEBUG
static constexpr void assert_disabled() {}
    #else
void assert_disabled();
    #endif

/**
 * Check if given axis is being used for phase stepping or not.
 *
 * Note: This is an in-line definition so this short function can be in-lined
 */
inline bool is_enabled(AxisEnum axis_num) {
    assert_initialized();
    if (axis_num < opts::SUPPORTED_AXIS_COUNT) {
        return axis_states[axis_num].enabled;
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
 * RAII guard for temporary setting phase stepping state with planner synchronization.
 **/
class StateRestorer {
    bool released = true;
    bool any_axis_change = false;
    std::array<bool, opts::SUPPORTED_AXIS_COUNT> _prev_active = {};

public:
    StateRestorer() {}
    StateRestorer(bool new_state) {
        set_state(new_state);
    }

    StateRestorer(const StateRestorer &) = delete;
    StateRestorer &operator=(const StateRestorer &) = delete;
    StateRestorer(StateRestorer &&) = delete;

    ~StateRestorer() {
        release();
    }

    void set_state(bool new_state) {
        assert_initialized();

        any_axis_change = std::ranges::any_of(axis_states, [&](const auto &state) -> bool {
            return state.enabled != new_state;
        });

        if (any_axis_change) {
            synchronize();

            for (std::size_t i = 0; i != axis_states.size(); i++) {
                if (released) {
                    // save original state on first change
                    _prev_active[i] = axis_states[i].enabled;
                }
                phase_stepping::enable(AxisEnum(i), new_state);
            }

            released = false;
        }
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

class EnsureEnabled : public StateRestorer {
public:
    EnsureEnabled()
        : StateRestorer(true) {}
};

class EnsureDisabled : public StateRestorer {
public:
    EnsureDisabled()
        : StateRestorer(false) {}
};

    #if PRINTER_IS_PRUSA_MK4()
// MK4 has homing sensitivity recalibration not compatible with burst
// stepping.
using EnsureSuitableForHoming = EnsureDisabled;
    #else
using EnsureSuitableForHoming = std::conditional_t<
    option::has_burst_stepping,
    EnsureNoChange, EnsureDisabled>;
    #endif

class WithCorrectionDisabled {
    AxisEnum axis;
    int harmonic;
    SpectralItem original;

public:
    WithCorrectionDisabled(AxisEnum axis, int harmonic)
        : axis(axis)
        , harmonic(harmonic) {
        original = axis_states[axis].forward_current.get_correction_table()[harmonic];
        axis_states[axis].forward_current.modify_correction_table([&](auto &table) {
            table[harmonic] = { 0, 0 };
        });
    }

    ~WithCorrectionDisabled() {
        axis_states[axis].forward_current.modify_correction_table([&](auto &table) {
            table[harmonic] = original;
        });
    }
};

enum class CorrectionType {
    forward,
    backward,
};

/**
 * Reset runtime current lookup tables for axis.
 */
void reset_compensation(AxisEnum axis);

/**
 * Load and enable previous settings, if any.
 **/
void load();

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
    debug_assert(false);
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

/** Like save_to_persistent_storage() but does not enable */
void save_to_persistent_storage_without_enabling(AxisEnum axis);

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
 * @brief Call to remove state from persistent media (ie eeprom/xflash)
 */
void remove_from_persistent_storage(AxisEnum axis, CorrectionType lut_type);

/// Removes luts for the specific axis from the persistent media
void remove_from_persistent_storage(AxisEnum axis);

/// Removes all LUTS from the persistent media
void remove_from_persistent_storage();

} // namespace phase_stepping

#endif // if !HAS_PHASE_STEPPING
