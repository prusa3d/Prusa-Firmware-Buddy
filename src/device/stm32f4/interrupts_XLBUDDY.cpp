#include "interrupts_helper.hpp"
#include <device/board.h>
#include <device/peripherals.h>
#include <device/peripherals_uart.hpp>
#include <option/buddy_enable_wui.h>
#include <option/has_burst_stepping.h>

#if BUDDY_ENABLE_WUI()
    #include "espif.h"
#endif

static_assert(BOARD_IS_XLBUDDY());

// TODO Document ADC peripherals
TRACED_ISR(DMA2_Stream4_IRQHandler, HAL_DMA_IRQHandler, hadc1.DMA_Handle);
TRACED_ISR(DMA2_Stream0_IRQHandler, HAL_DMA_IRQHandler, hadc3.DMA_Handle);

// SPI for trinamic driver
TRACED_ISR(SPI3_IRQHandler, HAL_SPI_IRQHandler, &SPI_HANDLE_FOR(tmc));
TRACED_ISR(DMA1_Stream5_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(tmc).hdmatx);

// SPI for flash memory
TRACED_ISR(DMA2_Stream3_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(flash).hdmarx);
TRACED_ISR(DMA2_Stream6_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(flash).hdmatx);

// SPI for LCD
TRACED_ISR(DMA2_Stream5_IRQHandler, HAL_DMA_IRQHandler, SPI_HANDLE_FOR(lcd).hdmatx);

#if HAS_BURST_STEPPING()

// DMA is required for burst stepping and can't be used by leds
BARE_ISR(DMA2_Stream1_IRQHandler, HAL_DMA_IRQHandler, hspi4.hdmatx);

// SPI for leds - using interrupts
TRACED_ISR(SPI4_IRQHandler, HAL_SPI_IRQHandler, &hspi4);

#else

// SPI for leds - using DMA
TRACED_ISR(DMA2_Stream1_IRQHandler, HAL_DMA_IRQHandler, hspi4.hdmatx);

#endif

// UART for puppy modbus
void uart_for_puppies_idle_isr() {
    uart_for_puppies.IdleISR();
}
TRACED_ISR(USART3_IRQHandler, HAL_UART_IRQHandler_with_idle, &uart_handle_for_puppies, uart_for_puppies_idle_isr);
TRACED_ISR(DMA1_Stream1_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_puppies.hdmarx);
TRACED_ISR(DMA1_Stream3_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_puppies.hdmatx);

#if BUDDY_ENABLE_WUI()

// UART for ESP network interface card
TRACED_ISR(UART8_IRQHandler, HAL_UART_IRQHandler_with_idle, &uart_handle_for_esp, espif_receive_data);
TRACED_ISR(DMA1_Stream6_IRQHandler, HAL_DMA_IRQHandler_with_idle, uart_handle_for_esp.hdmarx, espif_receive_data);
TRACED_ISR(DMA1_Stream0_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_esp.hdmatx);

#endif
