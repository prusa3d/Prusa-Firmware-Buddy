#include "timing.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "main.h"

#define TIMER_CURRENT_US        (htim12.Instance->CNT)
#define TIMER_CURRENT_NS_DIV_12 (htim4.Instance->CNT)

static uint64_t timestamp_cnt_ns;
static uint32_t tick_cnt_us;
static uint32_t tick_cnt_ns;

void tick_ms_irq() {
    // ms resolution managed by STM's HAL
    HAL_IncTick();
    // us resolution
    tick_cnt_us += 1000;
    // ns resolution
    tick_cnt_ns += 1000000;
    timestamp_cnt_ns += 1000000;
}

uint32_t ticks_ms() {
    return uwTick;
}

uint32_t ticks_us() {
    while (true) {
        uint32_t local_tick_cnt_us = tick_cnt_us;
        uint32_t local_timer_current_us = TIMER_CURRENT_US;

        if (local_tick_cnt_us != tick_cnt_us)
            // a tick ISR has happened in between the reading of the timer counter and tick_cnt_us
            // therefore, we have to read them again
            continue;
        return local_tick_cnt_us + local_timer_current_us;
    }
}

uint32_t ticks_ns() {
    while (true) {
        uint32_t local_tick_cnt_ns = tick_cnt_ns;
        uint32_t local_timer_current_us = TIMER_CURRENT_US;
        uint32_t local_timer_current_ns_12 = TIMER_CURRENT_NS_DIV_12;

        if (local_timer_current_us != TIMER_CURRENT_US)
            // an overflow of the ns-res timer has happened, lets try again
            continue;

        if (local_tick_cnt_ns != tick_cnt_ns)
            // a tick ISR has happened, lets try again
            continue;

        return local_tick_cnt_ns + (local_timer_current_us * 1000) + (local_timer_current_ns_12 * 12);
    }
}

uint64_t timestamp_ns() {
    while (true) {
        uint64_t local_timestamp_cnt_ns = timestamp_cnt_ns;
        uint32_t local_timer_current_us = TIMER_CURRENT_US;
        uint32_t local_timer_current_ns_12 = TIMER_CURRENT_NS_DIV_12;

        if (local_timer_current_us != TIMER_CURRENT_US)
            // an overflow of the ns-res timer has happened, lets try again
            continue;

        if (local_timestamp_cnt_ns != timestamp_cnt_ns)
            // a tick ISR has happened, lets try again
            continue;

        return local_timestamp_cnt_ns + (local_timer_current_us * 1000) + (local_timer_current_ns_12 * 12);
    }
}
