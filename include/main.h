#pragma once
#include "printers.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int HAL_GPIO_Initialized;
extern int HAL_ADC_Initialized;
extern int HAL_PWM_Initialized;

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern RTC_HandleTypeDef hrtc;

extern void Error_Handler(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#define GPIOV          (7 * 16) // virtual GPIO
#define VIRTUAL_PIN_0  0
#define VIRTUAL_PIN_1  1
#define VIRTUAL_PIN_2  2
#define VIRTUAL_PIN_3  3
#define VIRTUAL_PIN_4  4
#define VIRTUAL_PIN_5  5
#define VIRTUAL_PIN_6  6
#define VIRTUAL_PIN_7  7
#define VIRTUAL_PIN_8  8
#define VIRTUAL_PIN_9  9
#define VIRTUAL_PIN_10 10
#define VIRTUAL_PIN_11 11
#define VIRTUAL_PIN_12 12
#define VIRTUAL_PIN_13 13
#define VIRTUAL_PIN_14 14
#define VIRTUAL_PIN_15 15

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "main_MINI.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
