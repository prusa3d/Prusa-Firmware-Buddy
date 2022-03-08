//----------------------------------------------------------------------------//
// timer_defaults.h - default values for timers
#pragma once

//macros defining tick timer
//must be 32bit timer !!!
#define TICK_TIMER            TIM5
#define TICK_TIMER_CLK_ENABLE __HAL_RCC_TIM5_CLK_ENABLE
#define TICK_TIMER_IRQ        TIM5_IRQn
#define TICK_TIMER_IRQHandler TIM5_IRQHandler
#define TICK_TIMER_Prior      0

enum {
    TIM_BASE_CLK_MHZ = 84,

    //timer defaults
    //50000 us
    TIM1_default_Prescaler = 0x3fff,
    TIM1_default_Period = 0xff,
    TIM3_default_Prescaler = 0x3fff,
    TIM3_default_Period = 0xff,
};

//must be macro to be able to initialize variables with it
#define GEN_PERIOD_US(prescaler, period) \
    ((prescaler + 1) * (period + 1) / (int32_t)TIM_BASE_CLK_MHZ)
