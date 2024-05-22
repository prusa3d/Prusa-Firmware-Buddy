#include "timing_sys.h"
#include "cmsis_os.h"
#include "timing.h"
#include "device/cmsis.h"
#include "stm32g0xx_hal.h"
#include "device/peripherals.h"
#include "buddy/priorities_config.h"

TIM_HandleTypeDef TimerSysHandle;

// this is ticks value, incremented every 1ms
volatile uint32_t ticks_ms_ctr = 0;

// this is base for us timer, it is incremented by 1000 every 1ms, so current value of time has to be added to obtain time
volatile uint32_t ticks_us_base = 0;

/// This is called from HAL, to configure timebase timer on every clock change
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    assert(TickPriority == ISR_PRIORITY_TICK_TIMER);
    RCC_ClkInitTypeDef clkconfig;
    uint32_t uwTimclock;
    uint32_t uwAPB1Prescaler;
    uint32_t uwPrescalerValue;
    uint32_t pFLatency;
    HAL_StatusTypeDef status = HAL_OK;

    /* Check uwTickFreq for MisraC 2012 (even if uwTickFreq is a enum type that don't take the value zero)*/
    if ((uint32_t)uwTickFreq != 0U) {
        /* Enable TIM14 clock */
        __HAL_RCC_TIM14_CLK_ENABLE();

        /* Get clock configuration */
        HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

        /* Get APB1 prescaler */
        uwAPB1Prescaler = clkconfig.APB1CLKDivider;

        /* Compute TIM14 clock */
        if (uwAPB1Prescaler == RCC_HCLK_DIV1) {
            uwTimclock = HAL_RCC_GetPCLK1Freq();
        } else {
            uwTimclock = 2U * HAL_RCC_GetPCLK1Freq();
        }

        /* Compute the prescaler value to have TIM14 counter clock equal to 1MHz */
        uwPrescalerValue = (uint32_t)((uwTimclock / 1000000U) - 1U);

        /* Initialize TIM14 */
        TimerSysHandle.Instance = TIM14;

        /* Initialize TIMx peripheral as follow:
        + Period = [(TIM14CLK/uwTickFreq) - 1]. to have a (1/uwTickFreq) s time base.
        + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
        + ClockDivision = 0
        + Counter direction = Up
        */
        TimerSysHandle.Init.Period = (1000000U / (1000U / (uint32_t)uwTickFreq)) - 1U;
        TimerSysHandle.Init.Prescaler = uwPrescalerValue;
        TimerSysHandle.Init.ClockDivision = 0U;
        TimerSysHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
        TimerSysHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        if (HAL_TIM_Base_Init(&TimerSysHandle) == HAL_OK) {
            /* Start the TIM time Base generation in interrupt mode */
            if (HAL_TIM_Base_Start_IT(&TimerSysHandle) == HAL_OK) {
                /* Enable the TIM14 global Interrupt */
                HAL_NVIC_EnableIRQ(TIM14_IRQn);

                /* Configure the SysTick IRQ priority */
                if (TickPriority < (1UL << __NVIC_PRIO_BITS)) {
                    /*Configure the TIM14 IRQ priority */
                    HAL_NVIC_SetPriority(TIM14_IRQn, TickPriority, 0U);
                    uwTickPrio = TickPriority;
                } else {
                    status = HAL_ERROR;
                }
            } else {
                status = HAL_ERROR;
            }
        } else {
            status = HAL_ERROR;
        }
    } else {
        status = HAL_ERROR;
    }

    /* Return function status */
    return status;
}

// override HAL weak function
void HAL_SuspendTick(void) {
    /* Disable TIM14 update interrupt */
    __HAL_TIM_DISABLE_IT(&TimerSysHandle, TIM_IT_UPDATE);
}

// override HAL weak function
void HAL_ResumeTick(void) {
    /* Enable TIM14 update interrupt */
    __HAL_TIM_ENABLE_IT(&TimerSysHandle, TIM_IT_UPDATE);
}

// override HAL weak function
uint32_t HAL_GetTick(void) {
    return ticks_ms_ctr;
}

// override HAL weak function
void HAL_IncTick(void) {
    ticks_ms_ctr += uwTickFreq; // increment ms value
    ticks_us_base += 1000; // also increment us value base by 1000
}

uint32_t ticks_ms() {
    return ticks_ms_ctr;
}

uint32_t ticks_us() {
    // we need to get ticks_us_base & value of timer atomically, because HAL_IncTick can be called at any time.
    // this is achieved by sampling it multiple times, until we are sure it didn't change.

    // WARNING: this will not work correctly when called from ISR with priority higher then tick timer

    int32_t us_timer_base_before, us_timer_base_after, us_timer;
    do {
        us_timer_base_before = ticks_us_base;
        us_timer = __HAL_TIM_GetCounter(&TimerSysHandle);
        us_timer_base_after = ticks_us_base;
    } while (us_timer_base_before != us_timer_base_after);

    return us_timer_base_before + us_timer;
}

timestamp_t get_timestamp() {
    uint32_t ms_copy = ticks_ms();
    return { ms_copy / 1000, (ms_copy % 1000) * 1000 }; // Will overflow in 49 days with ticks_ms()
}
