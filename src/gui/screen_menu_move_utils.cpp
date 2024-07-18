/**
 * @file screen_menu_move_utils.cpp
 */

#include "screen_menu_move_utils.hpp"

#include "menu_vars.h"
#include "marlin_client.hpp"
#include "Marlin/src/module/planner.h"

#include <algorithm>
#include <limits>
#include <cmath>

void jog_axis(float &position, const float target, const AxisEnum axis) {
    if (position == target) {
        // Yeah and every time I try to go where I really want to be
        // it's already where I am 'cause I'm already there!
        return;
    }

    // This empirical constant was carefully crafted in such
    // a clever way that it seems to work most of the time.
    constexpr float magic_constant = 1. / (BLOCK_BUFFER_SIZE * 60 * 1.25 * 5);
    const float feedrate = MenuVars::GetManualFeedrate()[axis];
    const float short_segment = feedrate * magic_constant;
    const float long_segment = 5 * short_segment;

    // Just fill the entire queue with movements.
    // When i went up to BLOCK_BUFFER_SIZE, it was still choppy in certain situations
    for (uint8_t i = marlin_vars().pqueue; i < BLOCK_BUFFER_SIZE - 1; i++) {
        const float difference = (float)target - position;
        if (difference == 0) {
            break;
        } else if (difference >= long_segment) {
            position += long_segment;
        } else if (difference >= short_segment) {
            position += short_segment;
        } else if (difference <= -long_segment) {
            position -= long_segment;
        } else if (difference <= -short_segment) {
            position -= short_segment;
        } else {
            position = target;
        }
        marlin_client::move_axis(position, feedrate, axis);
    }
}

namespace {
/// Helper array with all the axes
constexpr std::array<AxisEnum, 3> AXES = { X_AXIS, Y_AXIS, Z_AXIS };
// This empirical constant was carefully crafted in such
// a clever way that it seems to work most of the time.
// Be aware the magic constant is 5 times bigger then in
// jog_axis since we base the calculation in job_mutliple_axis
// from the longer segments.
constexpr float MAGIC_CONSTANT = 1. / (BLOCK_BUFFER_SIZE * 60 * 1.25);
/// Helper constant to denote a unknown axis
constexpr size_t UNKNOWN_AXIS = X_AXIS - 1;

/// Maximal zero number limit
constexpr auto ZERO_LIMIT_HIGH = std::numeric_limits<float>::epsilon() * 1000.f;
/// Minimal zero number limit
constexpr float ZERO_LIMIT_LOW = -ZERO_LIMIT_HIGH;

/**
 * @brief Custom zero check for float numbers
 *
 * Sadly the standard library doesn't supply any function to create a range float comparison
 */
inline constexpr bool is_zero(float f) {
    return f >= ZERO_LIMIT_LOW && f <= ZERO_LIMIT_HIGH;
}

/// Helper function to validate that all the numbers in a xyz_float_t vector are zeros
inline constexpr bool all_zero(const xyz_float_t &vec) {
    return std::ranges::all_of(vec.pos, is_zero);
}

/**
 * @brief Helper function to calculate the correct movement speed from given direction.
 *
 * The speed in this context is a feedrate, which corresponds to the vector magnitude
 * of all combined axes movements. The default speeds are taken from
 * MenuVars::GetManualFeedrate.
 */
float get_correct_feedrate_from_direction(const xyz_float_t &direction) {
    const xyz_float_t feedrates {
        static_cast<float>(MenuVars::GetManualFeedrate()[X_AXIS]),
        static_cast<float>(MenuVars::GetManualFeedrate()[Y_AXIS]),
        static_cast<float>(MenuVars::GetManualFeedrate()[Z_AXIS])
    };
    const xyz_float_t long_segments = feedrates * MAGIC_CONSTANT;
    xyz_float_t modifiers {
        1.0f,
        1.0f,
        1.0f
    };

    for (const auto axis : AXES) {
        if (is_zero(direction[axis])) {
            modifiers[axis] = 0.0f;
        } else if (fabs(direction[axis]) < long_segments[axis]) {
            modifiers[axis] = fabs(direction[axis]) / long_segments[axis];
        }
    }

    const auto res = (feedrates * modifiers).magnitude();

    assert(res != 0.0f);
    return res;
}

} // namespace

void jog_multiple_axis(xyz_float_t &position, const xyz_float_t target) {
    if (position == target) {
        // Yeah and every time I try to go where I really want to be
        // it's already where I am 'cause I'm already there!
        return;
    }

    const xyz_float_t long_segments {
        static_cast<float>(MenuVars::GetManualFeedrate()[X_AXIS]) * MAGIC_CONSTANT,
        static_cast<float>(MenuVars::GetManualFeedrate()[Y_AXIS]) * MAGIC_CONSTANT,
        static_cast<float>(MenuVars::GetManualFeedrate()[Z_AXIS]) * MAGIC_CONSTANT
    };

    // Fill marlin queue enought so we have continuous movement,
    // but we don't block other events beeing processed
    for (uint8_t i = marlin_vars().pqueue; i < BLOCK_BUFFER_SIZE - 4; i++) {
        const auto difference = target - position;
        if (all_zero(difference)) {
            return;
        }

        xyz_float_t diff {};
        for (const auto axis : AXES) {
            if (difference[axis] != 0.0f) {
                if (difference[axis] >= long_segments[axis]) {
                    diff[axis] = long_segments[axis];
                } else if (difference[axis] <= -long_segments[axis]) {
                    diff[axis] = -long_segments[axis];
                } else {
                    diff[axis] = difference[axis];
                }
            }
        }
        position += diff;

        marlin_client::move_xyz_axes_to(position, get_correct_feedrate_from_direction(diff));
    }
}

void finish_movement(const xyz_float_t &start_position, const xyz_float_t &target) {
    xyz_float_t position = start_position;
    auto direction = target - position;
    xyz_float_t times {
        fabs(direction[X_AXIS]) / MenuVars::GetManualFeedrate()[X_AXIS],
        fabs(direction[Y_AXIS]) / MenuVars::GetManualFeedrate()[Y_AXIS],
        fabs(direction[Z_AXIS]) / MenuVars::GetManualFeedrate()[Z_AXIS],
    };

    // Plans up to 3 movements where each of them finishes movement of at least one axis
    while (!all_zero(times)) {
        xyz_float_t diff { 0.0f, 0.0f, 0.0f };
        size_t axis = UNKNOWN_AXIS;

        // Finds shortest time, since that movement will finishes first
        if (!is_zero(times[X_AXIS])) {
            axis = X_AXIS;
        }
        if (!is_zero(times[Y_AXIS]) && (axis == UNKNOWN_AXIS || times[Y_AXIS] < times[axis])) {
            axis = Y_AXIS;
        }
        if (!is_zero(times[Z_AXIS]) && (axis == UNKNOWN_AXIS || times[Z_AXIS] < times[axis])) {
            axis = Z_AXIS;
        }
        assert(axis != UNKNOWN_AXIS);

        const auto shortest_time = times[axis];

        for (const auto axis : AXES) {
            // recalculate the distance what each axis will achieve in the "shortest_time"
            if (!is_zero(direction[axis])) {
                diff[axis] = std::copysign(static_cast<float>(MenuVars::GetManualFeedrate()[axis]) * shortest_time, direction[axis]);
            }
        }

        // apply adjusted diff for given time
        position += diff;
        marlin_client::move_xyz_axes_to(position, get_correct_feedrate_from_direction(diff));

        // update the times and remaining diff
        times -= xyz_float_t {
            is_zero(times[X_AXIS]) ? 0.0f : shortest_time,
            is_zero(times[Y_AXIS]) ? 0.0f : shortest_time,
            is_zero(times[Z_AXIS]) ? 0.0f : shortest_time,
        };
        direction -= diff;
    }
}
