#include <device/hal.h>
#include <utility>
#include "timing.h"
#include "timer_defaults.h"
#include "FreeRTOS.h"
#include "tick_timer_api.h"
#include "wdt.h"
#include <buddy/priorities_config.h>

#define TICK_TIMER_CNT (h_tick_tim.Instance->CNT)

// Large numbers to avoid number of 0s errors
constexpr const uint32_t thousand = 1000UL;
constexpr const uint32_t million = thousand * thousand;
constexpr const uint32_t billion = thousand * thousand * thousand;

static volatile uint32_t tick_cnt_s;
static TIM_HandleTypeDef h_tick_tim {};

// rewrite weak function
// no need to call HAL_IncTick(), binded variable is unused
extern "C" uint32_t HAL_GetTick(void) {
    return ticks_ms();
}

// macro xPortSysTickHandler in FreeRTOSConfig.h must be commented
extern "C" void xPortSysTickHandler(void);

// interrupt from ARM-CORE timer
extern "C" void SysTick_Handler(void) {
    wdt_tick_1ms();
    xPortSysTickHandler();
}

#pragma GCC push_options
#pragma GCC optimize("Ofast")

/**
 * @brief Safely sample tick timer without the risk of race.
 * @param[out] sec seconds since boot
 * @param[out] subsec subseconds in TIM_BASE_CLK_MHZ, overflows every 1 second
 * @note Both subsec and sec need to be consistent, subsec will overlflow to 0 at the same time as sec increments.
 */
static void sample_timer(uint32_t &sec, uint32_t &subsec) {
    volatile uint32_t sec_1st_read;
    volatile uint32_t lower_cnt;
    volatile uint32_t sec_2nd_read;

    do {
        sec_1st_read = tick_cnt_s;
        lower_cnt = TICK_TIMER_CNT; // Will be in range 0 .. TIM_BASE_CLK_MHZ * million - 1
        sec_2nd_read = tick_cnt_s;
    } while (sec_1st_read != sec_2nd_read); // Repeat if overflow of the timer has happened

    sec = sec_1st_read;
    subsec = lower_cnt;
}

extern "C" int64_t get_timestamp_us() {
    uint32_t sec, subsec;
    sample_timer(sec, subsec);

    return static_cast<int64_t>(sec) * static_cast<int64_t>(million) + (subsec / TIM_BASE_CLK_MHZ);
}

extern "C" timestamp_t get_timestamp() {
    uint32_t sec, subsec;
    sample_timer(sec, subsec);

    return { sec, (subsec / TIM_BASE_CLK_MHZ) };
}

extern "C" uint32_t ticks_s() {
    return tick_cnt_s;
}

static uint32_t last_ms;
extern "C" uint32_t ticks_ms() {
    uint32_t sec, subsec;
    sample_timer(sec, subsec);

    last_ms = sec * thousand + subsec / (TIM_BASE_CLK_MHZ * thousand);
    return last_ms;
}

extern "C" uint32_t last_ticks_ms() {
    return last_ms;
}

extern "C" uint32_t ticks_us() {
    uint32_t sec, subsec;
    sample_timer(sec, subsec);

    return sec * million + subsec / TIM_BASE_CLK_MHZ;
}

extern "C" void TICK_TIMER_IRQHandler() {
    HAL_TIM_IRQHandler(&h_tick_tim);
}

extern "C" void app_tick_timer_overflow() {
    ++tick_cnt_s;
}

#pragma GCC pop_options

extern "C" void HAL_SuspendTick() {
    __HAL_TIM_DISABLE_IT(&h_tick_tim, TIM_IT_UPDATE);
}

extern "C" void HAL_ResumeTick() {
    __HAL_TIM_ENABLE_IT(&h_tick_tim, TIM_IT_UPDATE);
}

/**
 * @brief shadow weak function in HAL
 *        do nothing, SysTimer is owned by FreeRtos
 *
 * @param TickPriority
 * @return HAL_StatusTypeDef::HAL_OK
 */
extern "C" HAL_StatusTypeDef HAL_InitTick([[maybe_unused]] uint32_t TickPriority) {
    return HAL_OK;
}

/**
 * @brief Must use 32 bit timer
 *        ~12ns tick, 84MHz, 1s period
 *
 * @return HAL_StatusTypeDef
 */
extern "C" HAL_StatusTypeDef tick_timer_init() {
    HAL_StatusTypeDef status;

    /*Configure the IRQ priority */
    HAL_NVIC_SetPriority(TICK_TIMER_IRQ, ISR_PRIORITY_TICK_TIMER, 0);

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
