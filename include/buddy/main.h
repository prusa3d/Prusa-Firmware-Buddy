#pragma once
#include <device/cmsis.h>
#include <device/peripherals.h>
#include "printers.h"
#include "../src/common/uartrxbuff.h"
#include <stdint.h>
#include <device/board.h>
#include "MarlinPin.h"

// Do not use HAL external interrupt handlers, use PIN_TABLE to setup and handle external interrupts instead
#pragma GCC poison HAL_GPIO_EXTI_IRQHandler HAL_GPIO_EXTI_Callback

#ifdef __cplusplus

extern "C" {
#endif //__cplusplus

void main_cpp();

extern int HAL_GPIO_Initialized;
extern int HAL_ADC_Initialized;
extern int HAL_PWM_Initialized;
extern void init_error_screen();

extern uartrxbuff_t uart1rxbuff;

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif //__cplusplus
