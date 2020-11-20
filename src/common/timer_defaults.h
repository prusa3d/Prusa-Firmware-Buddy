//----------------------------------------------------------------------------//
// timer_defaults.h - default values for timers
#pragma once

enum {
    TIM_BASE_CLK_MHZ = 84,

    //timer defaults
    //50000 us
    TIM1_default_Prescaler = 0x3fff,
    TIM1_default_Period = 0xff,
    TIM3_default_Prescaler = 0x3fff,
    TIM3_default_Period = 0xff,
};

//must be macro to be able to inicialize variables with it
#define GEN_PERIOD_US(prescaler, period) \
    ((prescaler + 1) * (period + 1) / (int32_t)TIM_BASE_CLK_MHZ)
