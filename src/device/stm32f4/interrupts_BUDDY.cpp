#include "interrupts_helper.hpp"
#include <device/board.h>
#include <device/peripherals.h>
#include <hw/buffered_serial.hpp>
#include <option/buddy_enable_wui.h>

#if BUDDY_ENABLE_WUI()
    #include "espif.h"
#endif

static_assert(BOARD_IS_BUDDY());

// TODO stick this somewhere else
static uint8_t uart2rx_data[32];
buddy::hw::BufferedSerial uart2 {
    &huart2,
    nullptr,
    uart2rx_data,
    sizeof(uart2rx_data),
    buddy::hw::BufferedSerial::CommunicationMode::IT,
};
void uart2_idle_cb() {
    uart2.IdleISR();
}

// SPI for flash memory
TRACED_ISR(SPI3_IRQHandler, HAL_SPI_IRQHandler, &SPI_HANDLE_FOR(flash));
TRACED_ISR(DMA1_Stream0_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(flash).hdmarx);
TRACED_ISR(DMA1_Stream7_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(flash).hdmatx);

// SPI for LCD
TRACED_ISR(DMA1_Stream3_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(lcd).hdmarx);
TRACED_ISR(DMA1_Stream4_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(lcd).hdmatx);

// UART for Trinamic driver
TRACED_ISR(USART2_IRQHandler, HAL_UART_IRQHandler_with_idle, &UART_HANDLE_FOR(tmc), uart2_idle_cb);
TRACED_ISR(DMA1_Stream5_IRQHandler, HAL_DMA_IRQHandler, UART_HANDLE_FOR(tmc).hdmarx);

#if BUDDY_ENABLE_WUI()

// UART for ESP network interface card
TRACED_ISR(USART6_IRQHandler, HAL_UART_IRQHandler_with_idle, &UART_HANDLE_FOR(esp), espif_receive_data);
TRACED_ISR(DMA2_Stream1_IRQHandler, HAL_DMA_IRQHandler_with_idle, UART_HANDLE_FOR(esp).hdmarx, espif_receive_data);
TRACED_ISR(DMA2_Stream6_IRQHandler, HAL_DMA_IRQHandler, UART_HANDLE_FOR(esp).hdmatx);

#endif
