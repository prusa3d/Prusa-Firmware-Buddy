#include "timing.h"
#include "timing_private.h"
#include "timer_defaults.h"

#define TICKS_PERIOD 0x80000000

int32_t ticks_diff(uint32_t ticks_a, uint32_t ticks_b) {
    return ((int32_t)(((ticks_a - ticks_b + TICKS_PERIOD / 2) & (TICKS_PERIOD - 1)) - TICKS_PERIOD / 2));
}

uint64_t clock_to_ns(uint32_t counter) {
    uint64_t cnt = counter;
    cnt *= (uint64_t)1000;
    return cnt / (uint64_t)TIM_BASE_CLK_MHZ;
}

uint64_t ms_to_clock(uint32_t ms) {
    return (uint64_t)ms * (uint64_t)(TIM_BASE_CLK_MHZ * 1000);
}
