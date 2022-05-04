/**
 * @file
 */

#include "cpu_utils.hpp"
#include "../common/timing_precise.hpp"
#include "stm32f4xx_hal.h"
#include <limits>

static volatile int cpu_usage_percent = 0;
static uint32_t cpu_idle_time_us = 0;

/**
 * Size of useless work unit affects how often
 * will IDLE task yield.
 *
 * Big work unit of 1000 us leads to more precise
 * CPU load measurement, but it increases delay
 * when returning CPU to other task which yielded.
 *
 * Small work unit of 100 us improves IDLE task
 * responsiveness at cost of showing about
 * 2% higher CPU load when printer is doing
 * nothing.
 * (Probably just RTOS housekeeping and checking
 * if there is other task to run each 100 us generates
 * 2% CPU load.)
 */
constexpr uint32_t probe_workunit_us = 100;
constexpr uint32_t calculation_period_ticks = 1000;

constexpr uint32_t calculation_period_us = calculation_period_ticks * 1000 * HAL_TICK_FREQ_DEFAULT;
static_assert((calculation_period_us * 100ULL) < std::numeric_limits<uint32_t>::max(), "cpu_usage_percent computation would overflow.");

extern "C" void vApplicationIdleHook() {
    DELAY_US_PRECISE(probe_workunit_us);
    cpu_idle_time_us += probe_workunit_us;
}

extern "C" void vApplicationTickHook(void) {
    static unsigned tick = 0;

    if (tick++ > calculation_period_ticks) {
        tick = 0;

        if (cpu_idle_time_us > calculation_period_us) {
            cpu_idle_time_us = calculation_period_us;
        }
        cpu_usage_percent = (100 - (cpu_idle_time_us * 100) / calculation_period_us);
        cpu_idle_time_us = 0;
    }
}

int osGetCPUUsage() {
    return cpu_usage_percent;
}
