#pragma once

#include <segger/SEGGER_SYSVIEW_FreeRTOS.h>
#include <stm32f4xx_hal_dma.h>
#include <stm32f4xx_hal_uart.h>

#define TRACED_ISR(name, impl, ...) \
    extern "C" void name() {        \
        traceISR_ENTER();           \
        impl(__VA_ARGS__);          \
        traceISR_EXIT();            \
    }

#define BARE_ISR(name, impl, ...) \
    extern "C" void name() {      \
        impl(__VA_ARGS__);        \
    }

// Like HAL_UART_IRQHandler() but with callback() on IDLE condition
void HAL_UART_IRQHandler_with_idle(UART_HandleTypeDef *huart, void (*callback)());

// Like HAL_DMA_IRQHandler() but with callback() on IDLE condition
void HAL_DMA_IRQHandler_with_idle(DMA_HandleTypeDef *hdma, void (*callback)());
