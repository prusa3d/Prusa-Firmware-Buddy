#include <device/cmsis.h>
#include <device/peripherals.h>
#include "hal/HAL_RS485.hpp"

extern "C" {

void DMA1_Channel2_3_IRQHandler() {
    hal::RS485Driver::DMA_IRQHandler();
}

void USART1_IRQHandler() {
    hal::RS485Driver::USART_IRQHandler();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) {
        HAL_IncTick();
    }
}
}
