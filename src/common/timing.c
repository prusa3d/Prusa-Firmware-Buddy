#include "timing.h"
#include "timer_defaults.h"

#define TICKS_PERIOD 0x80000000

int32_t ticks_diff(uint32_t ticks_a, uint32_t ticks_b) {
    return ((int32_t)(((ticks_a - ticks_b + TICKS_PERIOD / 2) & (TICKS_PERIOD - 1)) - TICKS_PERIOD / 2));
}

uint64_t ticks_to_ns(uint32_t ticks) {
    uint64_t cnt = ticks;
    cnt *= (uint64_t)1000;
    return cnt / (uint64_t)TIM_BASE_CLK_MHZ;
}

uint64_t ms_to_ticks(uint32_t ms) {
    return (uint64_t)ms * (uint64_t)(TIM_BASE_CLK_MHZ * 1000);
}
