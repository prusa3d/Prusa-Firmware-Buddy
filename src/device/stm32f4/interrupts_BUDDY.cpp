#include "interrupts_helper.hpp"
#include <device/board.h>
#include <device/peripherals.h>
#include <device/peripherals_uart.hpp>
#include <hw/buffered_serial.hpp>
#include <option/buddy_enable_wui.h>

#if BUDDY_ENABLE_WUI()
    #include "espif.h"
#endif

static_assert(BOARD_IS_BUDDY());

void uart_for_tmc_idle_isr() {
    uart_for_tmc.IdleISR();
}

// SPI for flash memory
TRACED_ISR(SPI3_IRQHandler, HAL_SPI_IRQHandler, &SPI_HANDLE_FOR(flash));
TRACED_ISR(DMA1_Stream0_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(flash).hdmarx);
TRACED_ISR(DMA1_Stream7_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(flash).hdmatx);

// SPI for LCD
TRACED_ISR(DMA1_Stream3_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(lcd).hdmarx);
TRACED_ISR(DMA1_Stream4_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(lcd).hdmatx);

// UART for Trinamic driver
TRACED_ISR(USART2_IRQHandler, HAL_UART_IRQHandler_with_idle, &uart_handle_for_tmc, uart_for_tmc_idle_isr);
TRACED_ISR(DMA1_Stream5_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_tmc.hdmarx);

#if BUDDY_ENABLE_WUI()

// UART for ESP network interface card
TRACED_ISR(USART6_IRQHandler, HAL_UART_IRQHandler_with_idle, &uart_handle_for_esp, espif_receive_data);
TRACED_ISR(DMA2_Stream1_IRQHandler, HAL_DMA_IRQHandler_with_idle, uart_handle_for_esp.hdmarx, espif_receive_data);
TRACED_ISR(DMA2_Stream6_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_esp.hdmatx);

#endif
