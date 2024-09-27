#include "interrupts_helper.hpp"
#include <device/board.h>
#include <device/peripherals.h>
#include <hw/buffered_serial.hpp>
#include <option/buddy_enable_wui.h>
#include <option/has_puppies.h>
#include <printers.h>

#if BUDDY_ENABLE_WUI()
    #include "espif.h"
#endif

static_assert(BOARD_IS_XBUDDY());

#if HAS_PUPPIES()
void uart6_idle_cb() {
}
#else
static uint8_t uart6rx_data[32];
buddy::hw::BufferedSerial uart6 {
    &huart6,
    nullptr,
    uart6rx_data,
    sizeof(uart6rx_data),
    buddy::hw::BufferedSerial::CommunicationMode::DMA
};
void uart6_idle_cb() {
    uart6.IdleISR();
}
#endif

// TODO Document ADC peripherals
TRACED_ISR(DMA2_Stream4_IRQHandler, HAL_DMA_IRQHandler, hadc1.DMA_Handle);
TRACED_ISR(DMA2_Stream0_IRQHandler, HAL_DMA_IRQHandler, hadc3.DMA_Handle);

// SPI for trinamic driver
TRACED_ISR(SPI3_IRQHandler, HAL_SPI_IRQHandler, &SPI_HANDLE_FOR(tmc));
TRACED_ISR(DMA1_Stream5_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(tmc).hdmatx);

#if PRINTER_IS_PRUSA_iX()

// SPI for side leds on iX
TRACED_ISR(DMA2_Stream1_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(led).hdmatx);

#endif

// SPI for flash memory
TRACED_ISR(DMA2_Stream3_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(flash).hdmarx);
TRACED_ISR(DMA2_Stream6_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(flash).hdmatx);

// SPI for LCD
TRACED_ISR(DMA2_Stream5_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(lcd).hdmatx);

// UART for MMU on MK4 or puppies on iX
TRACED_ISR(USART6_IRQHandler, HAL_UART_IRQHandler_with_idle, &huart6, uart6_idle_cb);
TRACED_ISR(DMA2_Stream2_IRQHandler, HAL_DMA_IRQHandler, huart6.hdmarx);
TRACED_ISR(DMA2_Stream7_IRQHandler, HAL_DMA_IRQHandler, huart6.hdmatx);

#if BUDDY_ENABLE_WUI()

// UART for ESP network interface card
TRACED_ISR(UART8_IRQHandler, HAL_UART_IRQHandler_with_idle, &UART_HANDLE_FOR(esp), espif_receive_data);
TRACED_ISR(DMA1_Stream6_IRQHandler, HAL_DMA_IRQHandler_with_idle, UART_HANDLE_FOR(esp).hdmarx, espif_receive_data);
TRACED_ISR(DMA1_Stream0_IRQHandler, HAL_DMA_IRQHandler, UART_HANDLE_FOR(esp).hdmatx);

#endif
