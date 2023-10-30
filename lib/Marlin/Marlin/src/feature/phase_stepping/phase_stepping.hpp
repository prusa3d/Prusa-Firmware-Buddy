#pragma once

#include "../../inc/MarlinConfig.h"

#include <utility>
#include <bitset>
#include <array>

struct move_t;
struct step_event_info_t;
struct step_generator_state_t;
struct move_segment_step_generator_t;

#ifdef PHASE_STEPPING

// Phase stepping is just logical extension of input shaper - just instead of
// making precise steps, we directly control phase of the motor at sufficient
// frequency. Therefore, phase_stepping reuses the whole internal logic of input
// shaping; we just redefine step generator.

namespace phase_stepping {

// The number of ticks per electrical period of the Trinamic driver
static constexpr int MOTOR_PERIOD = 1024;

class CurrentLut {
public:
    /**
     * Initialize LUT with pure sine and cosine waves
     */
    CurrentLut();

    void set_current(int idx, int sin, int cos);
    std::pair< int, int > get_current(int idx) const;
private:
    std::array< uint8_t, MOTOR_PERIOD > _sin, _cos;
    std::bitset< MOTOR_PERIOD >         _s_sin, _s_cos;
};


/**
  * Very simple and efficient implementation of 2-element queue of T*
  **/
template < typename T >
class TwoOf {
public:
    TwoOf(): _items({ nullptr, nullptr }), _active( 0 ) {};

    int size() const {
        return int( _items[0] != nullptr ) + int( _items[1] != nullptr );
    }

    bool empty() const {
        return _items[0] == nullptr && _items[1] == nullptr;
    }

    bool full() const {
        return _items[0] != nullptr && _items[1] != nullptr;
    }

    T& front() const {
        assert(!empty());

        return *_items[ _active ];
    }

    T& back() const {
        assert(!empty());

        int next_idx = _next_idx();
        if ( _items[ next_idx ] != nullptr)
            return *_items[next_idx];
        return *_items[_active];
    }

    T *pop() {
        assert(!empty());

        T* val = _items[ _active ];
        _items[ _active ] = nullptr;
        _active = _next_idx();
        return val;
    }

    void push( T *t ) {
        assert(!full());

        if ( _items[ _active ] == nullptr)
            _items[ _active ] = t;
        else
            _items[ _next_idx() ] = t;
    }
private:
    unsigned _next_idx() const {
        return (_active + 1) % 2;
    }

    std::array< T *, 2 > _items;
    unsigned             _active;
};

struct MoveTarget {
    MoveTarget() = default;
    MoveTarget(double position);
    MoveTarget(const move_t& move, int axis);

    double initial_pos = 0;
    double half_accel  = 0;
    double start_v     = 0;
    uint32_t duration  = 0; // Movement duration in us
};

struct AxisState {
    AxisState() = default;

    CurrentLut currents;

    std::atomic< bool > active = false;

    bool          inverted = false;     // Inverted axis direction flag
    int           zero_rotor_phase = 0; // Rotor phase for position 0
    int           last_phase       = 0; // Last known rotor phase
    TwoOf<move_t> pending_moves;        // 2 element queue of the current and next move_t
    move_t *      current_move = nullptr;
    move_t *      last_processed_move = nullptr;

    uint32_t      initial_time = 0;     // Initial timestamp when the movement start
    MoveTarget    target;

    int           missed_tx_cnt = 0;
    double        target_time = 0;
};

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
double pos_to_phase(int axis, double position);

/**
 * Given axis state and time in µs ticks from movement start, compute axis
 * position
 */
double axis_position(const AxisState& axis_state, uint32_t move_epoch);

/**
 * Given a phase, normalize it into range <0, MOTOR_PERIOD)
 **/
int normalize_phase(int phase);

/**
 * Extracts physical axis position from logical one
 **/
template < typename Pos >
double extract_physical_position(AxisEnum axis, const Pos& pos) {
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
 extern std::array<AxisState, 2> axis_states;

} // namespace phase_stepping

#endif // ifdef PHASE_STEPPING
