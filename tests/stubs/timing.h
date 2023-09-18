#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Fake timing for testing purposes
// Test is supposed to provide an implementation for this
// See time_sync.cpp for inspiration
uint32_t ticks_us();
uint32_t ticks_s();
uint32_t ticks_ms();
inline uint32_t last_ticks_ms() {
    return ticks_ms();
}

#ifdef __cplusplus
}
#endif
