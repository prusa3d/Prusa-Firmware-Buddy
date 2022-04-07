#include "main.h"
#include "stm32f4xx_it.h"
#include "cmsis_os.h"
#include "config.h"
#include "bsod.h"
#include "dump.h"
#include "sys.h"
#include "buffered_serial.hpp"
#include "tusb.h"

#ifdef BUDDY_ENABLE_WUI
    #include "espif.h"
#endif

extern ETH_HandleTypeDef heth;
extern HCD_HandleTypeDef hhcd_USB_OTG_HS;
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern DMA_HandleTypeDef hdma_spi3_rx;
extern TIM_HandleTypeDef htim14;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart6_rx;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;
extern uartrxbuff_t uart6rxbuff;
extern TIM_HandleTypeDef htim6;
extern WWDG_HandleTypeDef hwwdg;

extern DMA_HandleTypeDef hdma_adc1;

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/

/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void __attribute__((naked)) HardFault_Handler(void) {
    DUMP_HARDFAULT_TO_CCRAM();
    dump_to_xflash();
    sys_reset();
    while (1) {
    }
}

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

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

void USART2_IRQHandler() {
    traceISR_ENTER();
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart2);
        uart2_idle_cb(&huart2);
    }
    HAL_UART_IRQHandler(&huart2);
    traceISR_EXIT();
}

#ifdef USE_ESP01_WITH_UART6
void USART6_IRQHandler(void) {

    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart6);
    #ifdef BUDDY_ENABLE_WUI
        espif_receive_data(&huart6);
    #endif // BUDDY_ENABLE_WUI
    }
    HAL_UART_IRQHandler(&huart6);
}
#else  // USE_ESP01_WITH_UART6
void USART6_IRQHandler() {
    traceISR_ENTER();
    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart6);
        uartrxbuff_idle_cb(&uart6rxbuff);
    }
    HAL_UART_IRQHandler(&huart6);
    traceISR_EXIT();
}
#endif // USE_ESP01_WITH_UART6

/**
 * @brief This function handles Window watchdog interrupt.
 */
void WWDG_IRQHandler(void) {
    traceISR_ENTER();
    HAL_WWDG_IRQHandler(&hwwdg);
    traceISR_EXIT();
}

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

/**
 * @brief This function handles TIM8 trigger and commutation interrupts and TIM14 global interrupt.
 */
void TIM8_TRG_COM_TIM14_IRQHandler(void) {
    traceISR_ENTER();
    HAL_TIM_IRQHandler(&htim14);
    traceISR_EXIT();
}

/**
 * @brief This function handles DMA2 stream1 global interrupt.
 */
void DMA2_Stream1_IRQHandler(void) {
    traceISR_ENTER();

#ifdef BUDDY_ENABLE_WUI
    if (__HAL_DMA_GET_IT_SOURCE(&hdma_usart6_rx, DMA_IT_HT) != RESET || __HAL_DMA_GET_IT_SOURCE(&hdma_usart6_rx, DMA_IT_TC) != RESET) {
        espif_receive_data(&huart6);
    }
#endif

    HAL_DMA_IRQHandler(&hdma_usart6_rx);
    traceISR_EXIT();
}

#ifndef USE_ESP01_WITH_UART6
/**
 * @brief This function handles DMA2 stream2 global interrupt.
 */
void DMA2_Stream2_IRQHandler(void) {
    traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
    traceISR_EXIT();
}
#endif

/**
 * @brief This function handles DMA2 stream0 global interrupt.
 */
void DMA2_Stream0_IRQHandler(void) {
    // traceISR_ENTER();
    HAL_DMA_IRQHandler(&hdma_adc1);
    // traceISR_EXIT();
}

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
