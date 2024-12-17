#pragma once
inline uint32_t ticks_ms() { return 1; }
inline uint32_t last_ticks_ms() { return 1; }
inline int32_t ticks_diff(uint32_t ticks_a, uint32_t ticks_b) {
    return ((int32_t)(ticks_a - ticks_b));
}
