/// @file pulse_gen.h
#pragma once
#include "speed_table.h"
#include "../hal/tmc2130.h"
#include "../hal/circular_buffer.h"
#include "../cmath.h"

namespace modules {

/// Acceleration ramp and stepper pulse generator
namespace pulse_gen {

using config::blockBufferSize;
using hal::tmc2130::TMC2130;
using math::mulU24X24toH16;
using speed_table::calc_timer;
using speed_table::st_timer_t;

typedef CircularIndex<uint8_t, blockBufferSize> circular_index_t;
typedef uint32_t steps_t; ///< Absolute step units
typedef uint32_t rate_t; ///< Type for step rates
typedef int32_t pos_t; ///< Axis position (signed)

class PulseGen {
public:
    PulseGen(steps_t max_jerk, steps_t acceleration);

    /// @returns the jerk for the axis
    inline steps_t Jerk() const { return max_jerk; };

    /// Set maximum jerk for the axis
    inline void SetJerk(steps_t max_jerk) { this->max_jerk = max_jerk; };

    /// @returns the acceleration for the axis
    inline steps_t Acceleration() const { return acceleration; };

    /// Set acceleration for the axis
    inline void SetAcceleration(steps_t accel) { acceleration = accel; }

    /// Enqueue a single move in steps starting and ending at zero speed with maximum
    /// feedrate. Moves can only be enqueued if the axis is not Full().
    /// @param pos target position
    /// @param feed_rate maximum feedrate
    /// @param end_rate endding feedrate (may not be reached!)
    /// @returns true if the move has been enqueued
    bool PlanMoveTo(pos_t pos, steps_t feed_rate, steps_t end_rate = 0);

    /// Stop whatever moves are being done
    /// @param halt When true, also abruptly stop axis movement.
    void AbortPlannedMoves(bool halt = true);

    /// @returns the head position of the axis at the end of all moves
    pos_t Position() const { return position; }

    /// Fetch the current position of the axis while stepping. This function is expensive!
    /// It's necessary only in exceptional cases. For regular usage, Position() should
    /// probably be used instead.
    /// @returns the current position of the axis
    pos_t CurPosition() const;

    /// Set the position of the axis
    /// Should only be called when the queue is empty.
    /// @param x position to set
    inline void SetPosition(pos_t x) { position = x; }

    /// Fetch the target rate of the last planned segment, or the current effective rate
    /// when the move has been aborted.
    inline steps_t Rate() const { return last_rate; }

    /// @returns true if all planned moves have been finished
    inline bool QueueEmpty() const { return block_index.empty(); }

    /// @returns false if new moves can still be planned
    inline bool Full() const { return block_index.full(); }

    inline uint8_t PlannedMoves() const { return block_index.count(); }

    /// Single-step the axis
    /// @returns the interval for the next tick
    inline st_timer_t Step(const hal::tmc2130::MotorParams &motorParams) {
        if (!current_block) {
            // fetch next block
            if (!block_index.empty())
                current_block = &block_buffer[block_index.front()];
            if (!current_block)
                return 0;

            // Set direction early so that the direction-change delay is accounted for
            TMC2130::SetDir(motorParams, current_block->direction);

            // Initializes the trapezoid generator from the current block.
            deceleration_time = 0;
            acc_step_rate = uint16_t(current_block->initial_rate);
            acceleration_time = calc_timer(acc_step_rate, step_loops);
            steps_completed = 0;

            // Set the nominal step loops to zero to indicate that the timer value is not
            // known yet. That means, delay the initialization of nominal step rate and
            // step loops until the steady state is reached.
            step_loops_nominal = 0;
        }

        // Step the motor
        for (uint8_t i = 0; i < step_loops; ++i) {
            TMC2130::Step(motorParams);
            if (++steps_completed >= current_block->steps)
                break;
        }

        // Calculate new timer value
        // 13.38-14.63us for steady state,
        // 25.12us for acceleration / deceleration.
        st_timer_t timer;
        if (steps_completed <= current_block->accelerate_until) {
            // v = t * a   ->   acc_step_rate = acceleration_time * current_block->acceleration_rate
            acc_step_rate = mulU24X24toH16(acceleration_time, current_block->acceleration_rate);
            acc_step_rate += uint16_t(current_block->initial_rate);
            // upper limit
            if (acc_step_rate > uint16_t(current_block->nominal_rate))
                acc_step_rate = current_block->nominal_rate;
            // step_rate to timer interval
            timer = calc_timer(acc_step_rate, step_loops);
            acceleration_time += timer;
        } else if (steps_completed > current_block->decelerate_after) {
            st_timer_t step_rate = mulU24X24toH16(deceleration_time, current_block->acceleration_rate);

            if (step_rate > acc_step_rate) { // Check step_rate stays positive
                step_rate = uint16_t(current_block->final_rate);
            } else {
                step_rate = acc_step_rate - step_rate; // Decelerate from acceleration end point.

                // lower limit
                if (step_rate < current_block->final_rate)
                    step_rate = uint16_t(current_block->final_rate);
            }

            // Step_rate to timer interval.
            timer = calc_timer(step_rate, step_loops);
            deceleration_time += timer;
        } else {
            if (!step_loops_nominal) {
                // Calculation of the steady state timer rate has been delayed to the 1st tick
                // of the steady state to lower the initial interrupt blocking.
                timer_nominal = calc_timer(uint16_t(current_block->nominal_rate), step_loops);
                step_loops_nominal = step_loops;
            }
            timer = timer_nominal;
        }

        // If current block is finished, reset pointer
        if (steps_completed >= current_block->steps) {
            current_block = nullptr;
            block_index.pop();
        }

        return timer;
    }

private:
    /// Motion parameters for the current planned or executing move
    struct block_t {
        steps_t steps; ///< Step events
        bool direction; ///< The direction for this block

        rate_t acceleration_rate; ///< The acceleration rate
        steps_t accelerate_until; ///< The index of the step event on which to stop acceleration
        steps_t decelerate_after; ///< The index of the step event on which to start decelerating

        // Settings for the trapezoid generator (runs inside an interrupt handler)
        rate_t nominal_rate; ///< The nominal step rate for this block in steps/sec
        rate_t initial_rate; ///< Rate at start of block
        rate_t final_rate; ///< Rate at exit
        rate_t acceleration; ///< acceleration steps/sec^2
    };

    // Block buffer parameters
    block_t block_buffer[blockBufferSize];
    circular_index_t block_index;
    block_t *current_block;

    // Axis data
    pos_t position; ///< Current axis position
    steps_t max_jerk; ///< Axis jerk (could be constant)
    steps_t acceleration; ///< Current axis acceleration
    steps_t last_rate; ///< Target speed at the last junction

    // Step parameters
    rate_t acceleration_time, deceleration_time;
    st_timer_t acc_step_rate; // decelaration start point
    uint8_t step_loops; // steps per loop
    uint8_t step_loops_nominal; // steps per loop at nominal speed
    st_timer_t timer_nominal; // nominal interval
    steps_t steps_completed; // steps completed

    /// Calculate the trapezoid parameters for the block
    void CalculateTrapezoid(block_t *block, steps_t entry_speed, steps_t exit_speed);

    /// Return the axis shift introduced by the specified full block
    static inline pos_t BlockShift(const block_t *block) {
        return block->direction ? block->steps : -block->steps;
    }

#ifdef UNITTEST_MOTION
public:
#endif
    /// Return the axis shift introduced by the current (live) block
    inline pos_t CurBlockShift() const {
        steps_t steps_missing = (current_block->steps - steps_completed);
        return current_block->direction ? steps_missing : -steps_missing;
    }
};

} // namespace pulse_gen
} // namespace modules
