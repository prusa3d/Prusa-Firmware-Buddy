/**
 * @file
 */
#pragma once
#include <stdint.h>
#include <cmath>

namespace restore_z {
/**
 * Used in store_definition.hpp so
 * its layout can never be changed.
 */
struct Position {
    float current_position_z;
    uint8_t axis_known_position;
};

inline bool operator==(Position lhs, Position rhs) {
    return (lhs.current_position_z == rhs.current_position_z)
        && (lhs.axis_known_position == rhs.axis_known_position);
}

/**
 * Used in store_definition.hpp so
 * the value can never be changed.
 *
 * Means no value stored.
 */
inline constexpr Position default_position { NAN, 0 };
} // namespace restore_z
