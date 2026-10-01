#pragma once

#include <cstdint>

[[nodiscard]] constexpr int32_t ticks_diff(uint32_t ticks_a, uint32_t ticks_b) {
    return static_cast<int32_t>(ticks_a - ticks_b);
}

/// Milliseconds since startup, wraps after ~49 days
[[nodiscard]] uint32_t ticks_ms();

/// Microseconds since startup, wraps after ~71 minutes
///
/// Not usable from an ISR with priority higher than the tick timer's.
[[nodiscard]] uint32_t ticks_us();
