#pragma once

#include <cstdint>

#include <hal.hpp>

#include <precise_timer.hpp>

extern precise_timing::Timer precise_timer;

#define TIM_BASE_CLK_MHZ precise_timing::counter_period;

inline int32_t ticks_diff(uint32_t ticks_a, uint32_t ticks_b) {
    return ((int32_t)(ticks_a - ticks_b));
}

inline uint64_t ticks_us() {
    return precise_timer.get_timestamp_us();
}

inline uint32_t ticks_ms() {
    return precise_timer.get_timestamp_ms();
}

inline uint64_t get_timestamp_us() {
    return precise_timer.get_timestamp_us();
}

/// Mirrors src/common/timing.h, which this header stands in for. Declared here rather than included
/// from there so that reaching it does not require the whole src/ tree on the include path.
typedef struct {
    uint32_t sec; ///< Seconds since the start of the system
    uint32_t us; ///< Microseconds consistent with sec
} timestamp_t;

/// Needed by src/logging/log_platform.cpp for log timestamps. It is compiled as part of the shared
/// `logging` target, so it sees src/common/timing.h and picks this up at link time - hence C linkage.
extern "C" timestamp_t get_timestamp();
