#include "timing.h"

#include <device/hal.h>
#include <device/peripherals.hpp>

// TIM7 replaces SysTick as the HAL time base so the same counter can serve
// microsecond timestamps: it counts at 1 MHz and fires every millisecond.
// SysTick is left to FreeRTOS.

namespace hal::peripherals {
TIM_HandleTypeDef htim7;
}

namespace {

volatile uint32_t ticks_ms_ctr = 0;
// Advances by one tick period with every tick, the running TIM7 count is added on top
volatile uint32_t ticks_us_base = 0;

// uwTickFreq holds the tick period in milliseconds
uint32_t tick_period_us() {
    return 1000 * static_cast<uint32_t>(uwTickFreq);
}

} // namespace

extern "C" {

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    using hal::peripherals::htim7;

    if (TickPriority >= (1UL << __NVIC_PRIO_BITS)) {
        return HAL_ERROR;
    }

    __HAL_RCC_TIM7_CLK_ENABLE();

    // Timer clock equals PCLK1 unless the APB prescaler is active, then it is doubled
    RCC_ClkInitTypeDef clkconfig;
    uint32_t flash_latency;
    HAL_RCC_GetClockConfig(&clkconfig, &flash_latency);
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    if (clkconfig.APB1CLKDivider != RCC_HCLK_DIV1) {
        timer_clock *= 2;
    }

    htim7.Instance = TIM7;
    htim7.Init.Prescaler = timer_clock / 1'000'000 - 1;
    htim7.Init.Period = tick_period_us() - 1;
    htim7.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim7) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_TIM_Base_Start_IT(&htim7) != HAL_OK) {
        return HAL_ERROR;
    }

    HAL_NVIC_SetPriority(TIM7_IRQn, TickPriority, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
    uwTickPrio = TickPriority;
    return HAL_OK;
}

void HAL_SuspendTick(void) {
    __HAL_TIM_DISABLE_IT(&hal::peripherals::htim7, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void) {
    __HAL_TIM_ENABLE_IT(&hal::peripherals::htim7, TIM_IT_UPDATE);
}

uint32_t HAL_GetTick(void) {
    return ticks_ms_ctr;
}

void HAL_IncTick(void) {
    ticks_ms_ctr += uwTickFreq;
    ticks_us_base += tick_period_us();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &hal::peripherals::htim7) {
        HAL_IncTick();
    }
}

void TIM7_IRQHandler(void) {
    HAL_TIM_IRQHandler(&hal::peripherals::htim7);
}
}

uint32_t ticks_ms() {
    return ticks_ms_ctr;
}

uint32_t ticks_us() {
    // The base and the counter cannot be read atomically, so re-read until no tick happened in between
    uint32_t base_before, base_after, counter;
    do {
        base_before = ticks_us_base;
        counter = __HAL_TIM_GET_COUNTER(&hal::peripherals::htim7);
        base_after = ticks_us_base;
    } while (base_before != base_after);
    return base_before + counter;
}
