#include "interrupts_helper.hpp"
#include <device/board.h>
#include <device/peripherals.h>
#include <device/peripherals_uart.hpp>
#include <hw/buffered_serial.hpp>
#include <option/buddy_enable_wui.h>
#include <option/has_mmu2.h>
#include <option/has_puppies.h>
#include <printers.h>

#if BUDDY_ENABLE_WUI()
    #include "espif.h"
#endif

static_assert(BOARD_IS_XBUDDY());

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

#if HAS_PUPPIES()

// UART for puppies on iX
void uart_for_puppies_idle_isr() {
    uart_for_puppies.IdleISR();
}
TRACED_ISR(USART6_IRQHandler, HAL_UART_IRQHandler_with_idle, &uart_handle_for_puppies, uart_for_puppies_idle_isr);
TRACED_ISR(DMA2_Stream2_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_puppies.hdmarx);
TRACED_ISR(DMA2_Stream7_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_puppies.hdmatx);

#endif

#if HAS_MMU2_OVER_UART()

// UART for MMU in case there are no puppies
void uart_for_mmu_idle_isr() {
    uart_for_mmu.IdleISR();
}
TRACED_ISR(USART6_IRQHandler, HAL_UART_IRQHandler_with_idle, &uart_handle_for_mmu, uart_for_mmu_idle_isr);
TRACED_ISR(DMA2_Stream2_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_mmu.hdmarx);
TRACED_ISR(DMA2_Stream7_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_mmu.hdmatx);

#endif

#if BUDDY_ENABLE_WUI()

// UART for ESP network interface card
TRACED_ISR(UART8_IRQHandler, HAL_UART_IRQHandler_with_idle, &uart_handle_for_esp, espif_receive_data);
TRACED_ISR(DMA1_Stream6_IRQHandler, HAL_DMA_IRQHandler_with_idle, uart_handle_for_esp.hdmarx, espif_receive_data);
TRACED_ISR(DMA1_Stream0_IRQHandler, HAL_DMA_IRQHandler, uart_handle_for_esp.hdmatx);

#endif
