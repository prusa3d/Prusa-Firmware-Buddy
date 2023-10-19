#include "main.h"
#include "cmsis_os.h"
#include "config.h"
#include "bsod.h"
#include <crash_dump/dump.hpp>

#include "sys.h"
#include "buffered_serial.hpp"
#include <option/has_puppies.h>
#include <device/board.h>
#include "log.h"
#include "tusb.h"
#include <device/peripherals.h>
#include "wdt.h"
#include "safe_state.h"
#include <option/buddy_enable_wui.h>

#ifdef BUDDY_ENABLE_WUI
    #include "espif.h"
#endif

extern ETH_HandleTypeDef heth;
extern HCD_HandleTypeDef hhcd_USB_OTG_HS;

using namespace crash_dump;

extern "C" {

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void) {
    bsod("MemManage_Handler");
    while (1) {
    }
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void) {
    bsod("BusFault_Handler");
    while (1) {
    }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void) {
    bsod("UsageFault_Handler");
    while (1) {
    }
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void) {
}

void USART2_IRQHandler() {
    traceISR_ENTER();
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart2);
#if BOARD_IS_BUDDY
        uart2_idle_cb();
#endif
    }
    HAL_UART_IRQHandler(&huart2);
    traceISR_EXIT();
}

void USART6_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart6);
#if BUDDY_ENABLE_WUI() && uart_esp == 6
        espif_receive_data(&huart6);
#elif BOARD_IS_XBUDDY
    #if !HAS_PUPPIES()
        uart6_idle_cb();
    #endif
#endif
    }
    HAL_UART_IRQHandler(&huart6);
}

/**
 * @brief This function handles UART8 global interrupt.
 */
void UART8_IRQHandler(void) {
#if defined(BUDDY_ENABLE_WUI) && uart_esp == 8
    if (__HAL_UART_GET_FLAG(&huart8, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart8);
        espif_receive_data(&huart8);
    }
#endif

    HAL_UART_IRQHandler(&huart8);
}

#if BOARD_IS_XLBUDDY
extern void uart3_idle_cb();
void USART3_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart3);
        uart3_idle_cb();
    }
    HAL_UART_IRQHandler(&huart3);
}
#endif

/**
 * @brief This function handles Window watchdog interrupt.
 */
void WWDG_IRQHandler(void) {
    traceISR_ENTER();
    HAL_WWDG_IRQHandler(&hwwdg);
    traceISR_EXIT();
}

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)

/**
 * @brief This function handles DMA1 stream5 global interrupt.
 */
void DMA1_Stream5_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_spi3_tx);
    traceISR_EXIT();
}
/**
 * @brief This function handles DMA2 stream3 global interrupt.
 */
void DMA2_Stream3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_spi5_rx);
}

/**
 * @brief This function handles DMA2 stream6 global interrupt.
 */
void DMA2_Stream6_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_spi5_tx);
}

/**
 * @brief This function handles DMA2 stream5 global interrupt.
 */
void DMA2_Stream5_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_spi6_tx);
    traceISR_EXIT();
}

#elif (BOARD_IS_BUDDY)

/**
 * @brief This function handles DMA1 stream0 global interrupt.
 */
void DMA1_Stream0_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_spi3_rx);
    traceISR_EXIT();
}

/**
 * @brief This function handles DMA1 stream3 global interrupt.
 */
void DMA1_Stream3_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
    traceISR_EXIT();
}

/**
 * @brief This function handles DMA1 stream4 global interrupt.
 */
void DMA1_Stream4_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
    traceISR_EXIT();
}

/**
 * @brief This function handles DMA1 stream5 global interrupt.
 */
void DMA1_Stream5_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
    traceISR_EXIT();
}

/**
 * @brief This function handles DMA1 stream7 global interrupt.
 */
void DMA1_Stream7_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_spi3_tx);
    traceISR_EXIT();
}

#else
    #error "Unknown board."
#endif

#if BOARD_IS_XLBUDDY
void DMA1_Stream1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart3_rx);
}

void DMA1_Stream3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart3_tx);
}

#endif

/**
 * @brief This function handles TIM8 trigger and commutation interrupts and TIM14 global interrupt.
 */
void TIM8_TRG_COM_TIM14_IRQHandler(void) {
    traceISR_ENTER();
    HAL_TIM_IRQHandler(&htim14);
    traceISR_EXIT();
}

/**
 * @brief This function handles DMA2 stream0 global interrupt.
 */
#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
void DMA2_Stream4_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_adc1);
}

void DMA2_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_adc3);
}

void DMA2_Stream1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_spi4_tx);
}
#endif

#if (BOARD_IS_BUDDY)
/**
 * @brief This function handles DMA2 stream6 global interrupt.
 */
void DMA2_Stream6_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart6_tx);
}

/**
 * @brief This function handles DMA2 stream1 global interrupt.
 */
void DMA2_Stream1_IRQHandler(void) {
    traceISR_ENTER();

    // HAL_DMA_IRQHandler(&hdma_usart6_rx);
    if (__HAL_DMA_GET_IT_SOURCE(&hdma_usart6_rx, DMA_IT_HT) != RESET || __HAL_DMA_GET_IT_SOURCE(&hdma_usart6_rx, DMA_IT_TC) != RESET) {
    #if BUDDY_ENABLE_WUI()
        espif_receive_data(&huart6);
    #endif // BUDDY_ENABLE_WUI
    }
    HAL_DMA_IRQHandler(&hdma_usart6_rx);
    traceISR_EXIT();
}
#endif

#if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
/**
 * @brief This function handles DMA2 stream2 global interrupt.
 */
void DMA2_Stream2_IRQHandler(void) {
    traceISR_ENTER();
    #if (BOARD_IS_BUDDY)
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
    #elif (BOARD_IS_XBUDDY)
    HAL_DMA_IRQHandler(&hdma_usart6_rx);
    #endif
    traceISR_EXIT();
}

void DMA2_Stream7_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_usart6_tx);
    traceISR_EXIT();
}
#endif

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
/**
 * @brief This function handles DMA1 stream0 global interrupt.
 */
void DMA1_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_uart8_tx);
}

/**
 * @brief This function handles DMA1 stream6 global interrupt.
 */
void DMA1_Stream6_IRQHandler(void) {
    if (__HAL_DMA_GET_IT_SOURCE(&hdma_uart8_rx, DMA_IT_HT) != RESET || __HAL_DMA_GET_IT_SOURCE(&hdma_uart8_rx, DMA_IT_TC) != RESET) {
    #if BUDDY_ENABLE_WUI()
        espif_receive_data(&huart8);
    #endif // BUDDY_ENABLE_WUI()
    }
    HAL_DMA_IRQHandler(&hdma_uart8_rx);
}
#endif

/**
 * @brief This function handles Ethernet global interrupt.
 */
void ETH_IRQHandler(void) {
    traceISR_ENTER();
    HAL_ETH_IRQHandler(&heth);
    traceISR_EXIT();
}

/**
 * @brief This function handles USB On The Go FS global interrupt.
 */
void OTG_FS_IRQHandler(void) {
    traceISR_ENTER();
    tud_int_handler(0);
    traceISR_EXIT();
}

/**
 * @brief This function handles USB On The Go HS global interrupt.
 */
void OTG_HS_IRQHandler(void) {
    traceISR_ENTER();
    HAL_HCD_IRQHandler(&hhcd_USB_OTG_HS);
    traceISR_EXIT();
}
}
