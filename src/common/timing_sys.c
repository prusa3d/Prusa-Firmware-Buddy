#include "timing.h"
#include "timing_private.h"
#include "timer_defaults.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "tick_timer_api.h"
#include "wdt.h"
#define TICK_TIMER_CNT (h_tick_tim.Instance->CNT)

// cannot use digit separator 1'000'000'000 .. stupid C
// so i used enum to not make mistake in zeroes count
enum {
    thousand = 1000,
    million = thousand * thousand,
    billion = million * thousand
};
static volatile uint32_t tick_cnt_s;
static TIM_HandleTypeDef h_tick_tim = { 0 };

//rewrite weak function
//no need to call HAL_IncTick(), binded variable is unused
uint32_t HAL_GetTick(void) {
    return ticks_ms();
}

// macro xPortSysTickHandler in FreeRTOSConfig.h must be commented
extern void xPortSysTickHandler(void);

//interrupt from ARM-CORE timer
void SysTick_Handler(void) {
    wdt_tick_1ms();
    xPortSysTickHandler();
}

uint64_t timestamp_ns() {
    while (1) {
        // could use 64 bit variable for seconds, but it would increase chance of overflow
        volatile uint32_t sec_1st_read = tick_cnt_s;
        volatile uint32_t lower_cnt = TICK_TIMER_CNT;
        volatile uint32_t sec_2nd_read = tick_cnt_s;

        if (sec_1st_read != sec_2nd_read)
            // an overflow of the timer has happened, lets try again
            continue;

        uint64_t ret = clock_to_ns(lower_cnt);
        ret %= billion;                                    //remove seconds from nanosecond variable
        ret += (uint64_t)billion * (uint64_t)sec_1st_read; //convert seconds to nano seconds

        return ret;
    }
}

uint32_t ticks_s() {
    return tick_cnt_s;
}

uint32_t ticks_ms() {
    uint64_t val = timestamp_ns();
    val /= (uint64_t)million;
    return (uint32_t)val;
}

uint32_t ticks_us() {
    uint64_t ret = timestamp_ns();
    ret /= (uint64_t)thousand;
    return (uint32_t)ret;
}

uint32_t ticks_ns() {
    return clock_to_ns(TICK_TIMER_CNT);
}

void TICK_TIMER_IRQHandler() {
    HAL_TIM_IRQHandler(&h_tick_tim);
}

void app_tick_timer_overflow() {
    ++tick_cnt_s;
}

void HAL_SuspendTick() {
    __HAL_TIM_DISABLE_IT(&h_tick_tim, TIM_IT_UPDATE);
}

void HAL_ResumeTick() {
    __HAL_TIM_ENABLE_IT(&h_tick_tim, TIM_IT_UPDATE);
}

/**
 * @brief shadow weak function in HAL
 *        do nothing, SysTimer is owned by FreeRtos
 *
 * @param TickPriority
 * @return HAL_StatusTypeDef::HAL_OK
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    return HAL_OK;
}

/**
 * @brief Must use 32 bit timer
 *        ~12ns tick, 84MHz, 1s period
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef tick_timer_init() {
    HAL_StatusTypeDef status;

    /*Configure the IRQ priority */
    HAL_NVIC_SetPriority(TICK_TIMER_IRQ, TICK_TIMER_Prior, 0);

    /* Enable the global Interrupt */
    HAL_NVIC_EnableIRQ(TICK_TIMER_IRQ);

    /* Enable clock */
    TICK_TIMER_CLK_ENABLE();

    h_tick_tim.Instance = TICK_TIMER;
    h_tick_tim.Init.Prescaler = 0; // no prescaler = we get full 84Mhz
    h_tick_tim.Init.CounterMode = TIM_COUNTERMODE_UP;
    h_tick_tim.Init.Period = (TIM_BASE_CLK_MHZ * million) - 1; // set period to 1s
    h_tick_tim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if ((status = HAL_TIM_Base_Init(&h_tick_tim)) == HAL_OK) {
        return HAL_TIM_Base_Start_IT(&h_tick_tim);
    }

    return status;
}
