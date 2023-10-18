#pragma once

#include "../../inc/MarlinConfig.h"

#include <utility>
#include <bitset>
#include <array>
#include <optional>
#include <memory>

struct move_t;
struct step_event_info_t;
struct step_generator_state_t;
struct move_segment_step_generator_t;

#ifdef PHASE_STEPPING

#include "common.hpp"
#include "lut.hpp"

namespace phase_stepping {

/**
  * Very simple and efficient implementation of 2-element queue of T
  **/
template < typename T >
class TwoOf {
public:
    TwoOf(): _items({{}, {}}), _active(0) {};

    int size() const {
        return int(_items[0].has_value()) + int(_items[1].has_value());
    }

    bool empty() const {
        return !_items[0].has_value() && !_items[1].has_value();
    }

    bool full() const {
        return _items[0].has_value() && _items[1].has_value();
    }

    const T& front() const {
        assert(!empty());

        return *_items[_active];
    }

    const T& back() const {
        assert(!empty());

        int next_idx = _next_idx();
        if (_items[ next_idx ].has_value())
            return *_items[next_idx];
        return *_items[_active];
    }

    T pop() {
        assert(!empty());

        T val = *_items[_active];
        _items[_active].reset();
        _active = _next_idx();
        return val;
    }

    void push(T t) {
        assert(!full());

        if (!_items[ _active ].has_value())
            _items[_active] = std::move(t);
        else
            _items[_next_idx()] = std::move(t);
    }

    template < typename... Args >
    void emplace(Args... args) {
        assert(!full());

        if (!_items[ _active ].has_value())
            _items[_active].emplace(std::forward<Args>(args)...);
        else
            _items[_next_idx()].emplace(std::forward<Args>(args)...);
    }
private:
    unsigned _next_idx() const {
        return (_active + 1) % 2;
    }

    std::array< std::optional< T >, 2 > _items;
    unsigned                            _active;
};

struct MoveTarget {
    MoveTarget() = default;
    MoveTarget(float position);
    MoveTarget(const move_t& move, int axis);

    float initial_pos  = 0;
    float half_accel   = 0;
    float start_v      = 0;
    uint32_t duration  = 0; // Movement duration in us
};

struct AxisState {
    AxisState(AxisEnum axis): axis_index(axis) {}

    int axis_index;

    CorrectedCurrentLut forward_current, backward_current;

    std::atomic< bool > active = false;

    bool              inverted = false;     // Inverted axis direction flag
    int               zero_rotor_phase = 0; // Rotor phase for position 0
    int               last_phase       = 0; // Last known rotor phase
    float             last_position    = 0.f;
    TwoOf<MoveTarget> pending_targets;      // 2 element queue of pre-processed elements
    move_t *          last_processed_move = nullptr;

    uint32_t                  initial_time = 0; // Initial timestamp when the movement start
    std::optional<MoveTarget> target;           // Current target to move

    int32_t initial_count_position = 0; // Value for updating Stepper::count_position
    int32_t initial_count_position_from_startup = 0; // Value for updating Stepper::count_position_from_startup

    int    missed_tx_cnt = 0;
};

/**
 * Initializes phase stepping. It has to be called before any other phase
 * stepping function.
 **/
void init();

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
 * Public interface for PreciseStepping - given a move and other states, setup
 * given axis generator with the move.
 **/
void init_step_generator(
    const move_t &move,
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state);

/**
 * Public interface pro PreciseStepping - build next step event
 */
step_event_info_t next_step_event(
    move_segment_step_generator_t &step_generator,
    step_generator_state_t &step_generator_state);

/**
 * This function should be invoked periodically at ~90 kHz. It is is safe to
 * invoke this function within interrupt context.
 **/
void handle_periodic_refresh();

/**
 * Return whether any of the axis is phase stepping mode
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
 * Given axis state and time in µs ticks from movement start, compute axis
 * speed and position.
 */
std::pair<float, float> axis_position(const AxisState& axis_state, uint32_t move_epoch);

/**
 * Extracts physical axis position from logical one
 **/
template < typename Pos >
float extract_physical_position(AxisEnum axis, const Pos& pos) {
    #ifdef COREXY
        if (axis == X_AXIS)
            return pos[0] + pos[1];
        else if (axis == Y_AXIS)
            return pos[0] - pos[1];
        else if (axis == Z_AXIS)
            return pos[2];
        else
            bsod("Unsupported AXIS");
    #else
        return pos[axis];
    #endif
}


/**
 * This array keeps axis state (and LUT tables) for each axis
 **/
extern std::array<
    std::unique_ptr<AxisState>,
    SUPPORTED_AXIS_COUNT> axis_states;


/**
 * RAII guard for temporary disabling/enabling phase stepping.
 **/
template < bool ENABLED >
class EnsureState {
    bool released = false;
    std::array< bool, SUPPORTED_AXIS_COUNT > _prev_active = {};
public:
    EnsureState() {
        for (std::size_t i = 0; i != axis_states.size(); i++) {
            _prev_active[i] = axis_states[i]->active;
            phase_stepping::enable(AxisEnum(i), ENABLED);
        }
    }

    ~EnsureState() {
        release();
    }

    void release() {
        if (released)
            return;
        released = true;
        for (std::size_t i = 0; i != axis_states.size(); i++) {
            phase_stepping::enable(AxisEnum(i), _prev_active[i]);
        }
    }
};

using EnsureEnabled = EnsureState<true>;
using EnsureDisabled = EnsureState<false>;

} // namespace phase_stepping

#else // ifdef PHASE_STEPPING
    #include "phase_stepping_dummies.hpp"
#endif // ifdef PHASE_STEPPING
