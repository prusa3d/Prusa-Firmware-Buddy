/// @file pulse_gen.cpp
#include "pulse_gen.h"

namespace modules {
namespace pulse_gen {

PulseGen::PulseGen(steps_t max_jerk, steps_t acceleration) {
    // Axis status
    position = 0;
    last_rate = 0;
    this->max_jerk = max_jerk;
    this->acceleration = acceleration;

    // Block buffer
    current_block = nullptr;
}

void PulseGen::CalculateTrapezoid(block_t *block, steps_t entry_speed, steps_t exit_speed) {
    // Minimum stepper rate 120Hz, maximum 40kHz. If the stepper rate goes above 10kHz,
    // the stepper interrupt routine groups the pulses by 2 or 4 pulses per interrupt tick.
    rate_t initial_rate = entry_speed;
    rate_t final_rate = exit_speed;

    // Limit minimal step rate (Otherwise the timer will overflow.)
    if (initial_rate < config::minStepRate)
        initial_rate = config::minStepRate;
    if (initial_rate > block->nominal_rate)
        initial_rate = block->nominal_rate;
    if (final_rate < config::minStepRate)
        final_rate = config::minStepRate;
    if (final_rate > block->nominal_rate)
        final_rate = block->nominal_rate;

    // Don't allow zero acceleration.
    rate_t acceleration = block->acceleration;
    if (acceleration == 0)
        acceleration = 1;

    // estimate_acceleration_distance(float initial_rate, float target_rate, float acceleration)
    // (target_rate*target_rate-initial_rate*initial_rate)/(2.0*acceleration));
    rate_t initial_rate_sqr = initial_rate * initial_rate;
    rate_t nominal_rate_sqr = block->nominal_rate * block->nominal_rate;
    rate_t final_rate_sqr = final_rate * final_rate;
    rate_t acceleration_x2 = acceleration << 1;
    // ceil(estimate_acceleration_distance(initial_rate, block->nominal_rate, acceleration));
    steps_t accelerate_steps = (nominal_rate_sqr - initial_rate_sqr + acceleration_x2 - 1) / acceleration_x2;
    // floor(estimate_acceleration_distance(block->nominal_rate, final_rate, -acceleration));
    steps_t decelerate_steps = (nominal_rate_sqr - final_rate_sqr) / acceleration_x2;
    steps_t accel_decel_steps = accelerate_steps + decelerate_steps;
    // Size of Plateau of Nominal Rate.
    steps_t plateau_steps = 0;

    // Is the Plateau of Nominal Rate smaller than nothing? That means no cruising, and we will
    // have to use intersection_distance() to calculate when to abort acceleration and start braking
    // in order to reach the final_rate exactly at the end of this block.
    if (accel_decel_steps < block->steps) {
        plateau_steps = block->steps - accel_decel_steps;
    } else {
        uint32_t acceleration_x4 = acceleration << 2;
        // Avoid negative numbers
        if (final_rate_sqr >= initial_rate_sqr) {
            // accelerate_steps = ceil(intersection_distance(initial_rate, final_rate, acceleration, block->steps));
            // intersection_distance(float initial_rate, float final_rate, float acceleration, float distance)
            // (2.0*acceleration*distance-initial_rate*initial_rate+final_rate*final_rate)/(4.0*acceleration);
            accelerate_steps = final_rate_sqr - initial_rate_sqr + acceleration_x4 - 1;
            if (block->steps & 1)
                accelerate_steps += acceleration_x2;
            accelerate_steps /= acceleration_x4;
            accelerate_steps += (block->steps >> 1);
            if (accelerate_steps > block->steps) {
                accelerate_steps = block->steps;
                final_rate = sqrt(acceleration_x2 * accelerate_steps + initial_rate_sqr);
            }
        } else {
            decelerate_steps = initial_rate_sqr - final_rate_sqr;
            if (block->steps & 1)
                decelerate_steps += acceleration_x2;
            decelerate_steps /= acceleration_x4;
            decelerate_steps += (block->steps >> 1);
            if (decelerate_steps > block->steps)
                decelerate_steps = block->steps;
            accelerate_steps = block->steps - decelerate_steps;
        }
    }

    block->accelerate_until = accelerate_steps;
    block->decelerate_after = accelerate_steps + plateau_steps;
    block->initial_rate = initial_rate;
    block->final_rate = final_rate;
}

bool PulseGen::PlanMoveTo(pos_t target, steps_t feed_rate, steps_t end_rate) {
    // Prepare to set up new block
    if (block_index.full())
        return false;
    block_t *block = &block_buffer[block_index.back()];

    // Bail if this is a zero-length block
    pos_t steps = target - position;
    block->steps = abs(steps);
    if (block->steps <= config::dropSegments) {
        // behave as-if the block has been scheduled
        return true;
    }

    // Direction and speed for this block
    block->direction = steps >= 0;
    block->nominal_rate = feed_rate;

    // Acceleration of the segment, in steps/sec^2
    block->acceleration = acceleration;

    // Calculate the ratio to 2^24 so that the rate division in Step() can be just a right shift
    constexpr float ratio = (float)(1lu << 24) / (F_CPU / config::stepTimerFrequencyDivider);
    constexpr rate_t mul = 8; // pre-multiply to increase the integer division resolution
    static_assert(!(mul & (mul - 1)), "mul must be a power of two");
    block->acceleration_rate = block->acceleration * (rate_t)(ratio * mul) / mul;

    // Simplified forward jerk: do not handle reversals
    steps_t entry_speed;
    if (feed_rate >= last_rate)
        entry_speed = last_rate + min(feed_rate - last_rate, max_jerk);
    else
        entry_speed = last_rate - min(last_rate - feed_rate, max_jerk);

    // Perform the trapezoid calculations
    CalculateTrapezoid(block, entry_speed, end_rate);
    last_rate = block->final_rate;

    // Move forward and update the state
    block_index.push();
    position = target;
    return true;
}

pos_t PulseGen::CurPosition() const {
    pos_t cur_pos = position;
    circular_index_t iter = block_index;

    // if we have a live block remove the partial offset
    if (current_block) {
        cur_pos -= CurBlockShift();
        iter.pop();
    }

    // rollback remaining blocks
    while (!iter.empty()) {
        cur_pos -= BlockShift(&block_buffer[iter.front()]);
        iter.pop();
    }

    return cur_pos;
}

void PulseGen::AbortPlannedMoves(bool halt) {
    // always update to effective position
    position = CurPosition();

    // destroy the current block
    if (current_block) {
        last_rate = acc_step_rate;
        current_block = nullptr;
    }

    // drop all remaining blocks
    block_index.reset();

    // truncate the last rate if halting
    if (halt)
        last_rate = 0;
}

} // namespace motor
} // namespace modules
