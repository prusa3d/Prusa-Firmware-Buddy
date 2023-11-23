#pragma once
#include "inttypes.h"
#include "stm32g0xx_hal.h"
#ifdef __cplusplus
extern "C" {
#endif

extern TIM_HandleTypeDef TimerSysHandle;

uint32_t HAL_GetTick(void);

uint32_t ticks_ms();
inline uint32_t last_ticks_ms() {
    return ticks_ms();
}

#ifdef __cplusplus
}
#endif
