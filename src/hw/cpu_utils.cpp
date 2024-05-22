/**
 * @file
 */

#include "cpu_utils.hpp"
#include "../common/timing_precise.hpp"
#include <device/hal.h>
#include <limits>

/**
 * Update period in ticks = calculation_period_ticks_power_2^2
 * e.g.:
 *
 * | calculation_period_ticks_power_2  | Update period in ticks |
 * | --------------------------------- | ---------------------- |
 * |                                 0 |    1                   |
 * |                                 1 |    2                   |
 * |                                10 | 1024                   |
 *
 * osGetCPUUsage resolution is affected by this, probe_workunit_us and tick duration.
 * For default tick duration of 1000 us and probe_workunit_us = 100
 * resolution and sample period is
 * | ...ticks_power_2  | Update period | Resolution |
 * | ----------------- | ------------- | ---------- |
 * |                 0 |    1 ms       |     10%    |
 * |                 1 |    2 ms       |      5%    |
 * |                 2 |    4 ms       |    2.5%    |
 * |                 3 |    8 ms       |   1.25%    |
 * |                 4 |   16 ms       |      1%    |
 * |                10 | 1.024 s       |      1%    |
 */
static constexpr uint32_t calculation_period_ticks_power_2 = 10;
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
static constexpr uint32_t probe_workunit_us = 100;
static constexpr uint32_t calculation_period_ticks = 1ul << calculation_period_ticks_power_2;

static constexpr uint32_t tick_us = 1000 * HAL_TICK_FREQ_DEFAULT;
static constexpr uint32_t percents = 100;
static constexpr uint32_t divisor = tick_us / percents;

static constexpr uint32_t probe_workunit_us_divided = probe_workunit_us / divisor;

static constexpr uint32_t calculation_period_us = calculation_period_ticks * tick_us;
static constexpr uint32_t calculation_period_us_divided = calculation_period_us / divisor;

static_assert(calculation_period_us_divided == (calculation_period_ticks * percents), "wrong assumption");

static volatile int cpu_usage_percent = 0;
static uint32_t cpu_idle_time_us_divided = 0;

static void (*watchdog_function)() = nullptr; ///< Pointer used as an idle task watchdog

void osSetIdleTaskWatchdog(void (*function)()) {
    watchdog_function = function;
}

extern "C" void vApplicationIdleHook() {
    delay_us_precise<probe_workunit_us>();
    cpu_idle_time_us_divided += probe_workunit_us_divided;

    if (watchdog_function != nullptr) {
        watchdog_function();
    }
}

extern "C" void vApplicationTickHook(void) {
    static unsigned tick = 0;

    if (tick++ > calculation_period_ticks) {
        tick = 0;

        if (cpu_idle_time_us_divided > (calculation_period_us_divided)) {
            cpu_idle_time_us_divided = calculation_period_us_divided;
        }
        cpu_usage_percent = (percents - (cpu_idle_time_us_divided >> calculation_period_ticks_power_2));
        cpu_idle_time_us_divided = 0;
    }
}

int osGetCPUUsage() {
    return cpu_usage_percent;
}
