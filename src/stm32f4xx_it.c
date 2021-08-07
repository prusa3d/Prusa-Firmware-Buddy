/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2019 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
#include "cmsis_os.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "config.h"
#include "bsod.h"
#include "dump.h"
#include "sys.h"
#include "buffered_serial.hpp"
#include "lwesp_ll_buddy.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern ETH_HandleTypeDef heth;
extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern HCD_HandleTypeDef hhcd_USB_OTG_HS;
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;
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

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void) {
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void __attribute__((naked)) HardFault_Handler(void) {
    /* USER CODE BEGIN HardFault_IRQn 0 */
    DUMP_HARDFAULT_TO_CCRAM();
    dump_to_xflash();
    sys_reset();
    /* USER CODE END HardFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_HardFault_IRQn 0 */
        /* USER CODE END W1_HardFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void) {
    /* USER CODE BEGIN MemoryManagement_IRQn 0 */
    bsod("MemManage_Handler");
    /* USER CODE END MemoryManagement_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
        /* USER CODE END W1_MemoryManagement_IRQn 0 */
    }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void) {
    /* USER CODE BEGIN BusFault_IRQn 0 */
    bsod("BusFault_Handler");
    /* USER CODE END BusFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_BusFault_IRQn 0 */
        /* USER CODE END W1_BusFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void) {
    /* USER CODE BEGIN UsageFault_IRQn 0 */
    bsod("UsageFault_Handler");
    /* USER CODE END UsageFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
        /* USER CODE END W1_UsageFault_IRQn 0 */
    }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void) {
    /* USER CODE BEGIN DebugMonitor_IRQn 0 */

    /* USER CODE END DebugMonitor_IRQn 0 */
    /* USER CODE BEGIN DebugMonitor_IRQn 1 */

    /* USER CODE END DebugMonitor_IRQn 1 */
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
        esp_receive_data(&huart6);
    }
    HAL_UART_IRQHandler(&huart6);
}
#else
void USART6_IRQHandler() {
    traceISR_ENTER();
    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart6);
        uartrxbuff_idle_cb(&uart6rxbuff);
    }
    HAL_UART_IRQHandler(&huart6);
    traceISR_EXIT();
}
#endif
/**
  * @brief This function handles Window watchdog interrupt.
  */
void WWDG_IRQHandler(void) {
    /* USER CODE BEGIN WWDG_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END WWDG_IRQn 0 */
    HAL_WWDG_IRQHandler(&hwwdg);
    /* USER CODE BEGIN WWDG_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END WWDG_IRQn 1 */
}

/**
 * @brief This function handles DMA1 stream3 global interrupt.
 */
void DMA1_Stream3_IRQHandler(void) {
    /* USER CODE BEGIN DMA1_Stream3_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END DMA1_Stream3_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
    /* USER CODE BEGIN DMA1_Stream3_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END DMA1_Stream3_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream4 global interrupt.
  */
void DMA1_Stream4_IRQHandler(void) {
    /* USER CODE BEGIN DMA1_Stream4_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END DMA1_Stream4_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
    /* USER CODE BEGIN DMA1_Stream4_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END DMA1_Stream4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream5 global interrupt.
  */
void DMA1_Stream5_IRQHandler(void) {
    /* USER CODE BEGIN DMA1_Stream5_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END DMA1_Stream5_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
    /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END DMA1_Stream5_IRQn 1 */
}

/**
  * @brief This function handles TIM8 trigger and commutation interrupts and TIM14 global interrupt.
  */
void TIM8_TRG_COM_TIM14_IRQHandler(void) {
    /* USER CODE BEGIN TIM8_TRG_COM_TIM14_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END TIM8_TRG_COM_TIM14_IRQn 0 */
    HAL_TIM_IRQHandler(&htim14);
    /* USER CODE BEGIN TIM8_TRG_COM_TIM14_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END TIM8_TRG_COM_TIM14_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream1 global interrupt.
  */
void DMA2_Stream1_IRQHandler(void) {
    /* USER CODE BEGIN DMA2_Stream1_IRQn 0 */
    traceISR_ENTER();
    if (__HAL_DMA_GET_IT_SOURCE(&hdma_usart6_rx, DMA_IT_HT) != RESET || __HAL_DMA_GET_IT_SOURCE(&hdma_usart6_rx, DMA_IT_TC) != RESET) {
        esp_receive_data(&huart6);
    }
    /* USER CODE END DMA2_Stream1_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_usart6_rx);
    /* USER CODE BEGIN DMA2_Stream1_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END DMA2_Stream1_IRQn 1 */
}

#ifndef USE_ESP01_WITH_UART6
/**
  * @brief This function handles DMA2 stream2 global interrupt.
  */
void DMA2_Stream2_IRQHandler(void) {
    /* USER CODE BEGIN DMA2_Stream2_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END DMA2_Stream2_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
    /* USER CODE BEGIN DMA2_Stream2_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END DMA2_Stream2_IRQn 1 */
}
#endif
/**
  * @brief This function handles DMA2 stream0 global interrupt.
  */
void DMA2_Stream0_IRQHandler(void) {
    /* USER CODE BEGIN DMA2_Stream0_IRQn 0 */
    //traceISR_ENTER();
    /* USER CODE END DMA2_Stream0_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_adc1);
    /* USER CODE BEGIN DMA2_Stream0_IRQn 1 */
    //traceISR_EXIT();
    /* USER CODE END DMA2_Stream0_IRQn 1 */
}

/**
  * @brief This function handles Ethernet global interrupt.
  */
void ETH_IRQHandler(void) {
    /* USER CODE BEGIN ETH_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END ETH_IRQn 0 */
    HAL_ETH_IRQHandler(&heth);
    /* USER CODE BEGIN ETH_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END ETH_IRQn 1 */
}

/**
  * @brief This function handles USB On The Go FS global interrupt.
  */
void OTG_FS_IRQHandler(void) {
    /* USER CODE BEGIN OTG_FS_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END OTG_FS_IRQn 0 */
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
    /* USER CODE BEGIN OTG_FS_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END OTG_FS_IRQn 1 */
}

/**
  * @brief This function handles USB On The Go HS global interrupt.
  */
void OTG_HS_IRQHandler(void) {
    /* USER CODE BEGIN OTG_HS_IRQn 0 */
    traceISR_ENTER();
    /* USER CODE END OTG_HS_IRQn 0 */
    HAL_HCD_IRQHandler(&hhcd_USB_OTG_HS);
    /* USER CODE BEGIN OTG_HS_IRQn 1 */
    traceISR_EXIT();
    /* USER CODE END OTG_HS_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
