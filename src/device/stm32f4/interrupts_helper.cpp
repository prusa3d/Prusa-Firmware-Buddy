#include "interrupts_helper.hpp"

void HAL_UART_IRQHandler_with_idle(UART_HandleTypeDef *huart, void (*callback)()) {
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        (*callback)();
    }
    HAL_UART_IRQHandler(huart);
}

void HAL_DMA_IRQHandler_with_idle(DMA_HandleTypeDef *hdma, void (*callback)()) {

    if (__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_HT) != RESET || __HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_TC) != RESET) {
        (*callback)();
    }
    HAL_DMA_IRQHandler(hdma);
}
