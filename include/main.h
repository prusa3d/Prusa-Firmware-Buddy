#pragma once
#include "printers.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int HAL_GPIO_Initialized;
extern int HAL_ADC_Initialized;
extern int HAL_PWM_Initialized;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

extern void Error_Handler(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "main_MINI.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
