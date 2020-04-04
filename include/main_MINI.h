/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "../src/common/uartrxbuff.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern uartrxbuff_t uart1rxbuff;
extern SPI_HandleTypeDef hspi2;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define X_DIAG_Pin            GPIO_PIN_2
#define X_DIAG_GPIO_Port      GPIOE
#define Z_DIAG_Pin            GPIO_PIN_3
#define Z_DIAG_GPIO_Port      GPIOE
#define USB_OVERC_Pin         GPIO_PIN_4
#define USB_OVERC_GPIO_Port   GPIOE
#define USB_EN_Pin            GPIO_PIN_5
#define USB_EN_GPIO_Port      GPIOE
#define ESP_GPIO0_Pin         GPIO_PIN_6
#define ESP_GPIO0_GPIO_Port   GPIOE
#define ESP_RST_Pin           GPIO_PIN_13
#define ESP_RST_GPIO_Port     GPIOC
#define THERM_0_Pin           GPIO_PIN_0
#define THERM_0_GPIO_Port     GPIOC
#define BUZZER_Pin            GPIO_PIN_0
#define BUZZER_GPIO_Port      GPIOA
#define HW_IDENTIFY_Pin       GPIO_PIN_3
#define HW_IDENTIFY_GPIO_Port GPIOA
#define THERM_1_Pin           GPIO_PIN_4
#define THERM_1_GPIO_Port     GPIOA
#define THERM_2_Pin           GPIO_PIN_5
#define THERM_2_GPIO_Port     GPIOA
#define THERM_PINDA_Pin       GPIO_PIN_6
#define THERM_PINDA_GPIO_Port GPIOA
#define BED_HEAT_Pin          GPIO_PIN_0
#define BED_HEAT_GPIO_Port    GPIOB
#define HEAT0_Pin             GPIO_PIN_1
#define HEAT0_GPIO_Port       GPIOB
#define BED_MON_Pin           GPIO_PIN_7
#define BED_MON_GPIO_Port     GPIOE
#define FAN1_Pin              GPIO_PIN_9
#define FAN1_GPIO_Port        GPIOE
#define FAN0_TACH_Pin         GPIO_PIN_10
#define FAN0_TACH_GPIO_Port   GPIOE
#define FAN0_TACH_EXTI_IRQn   EXTI15_10_IRQn
#define FAN0_Pin              GPIO_PIN_11
#define FAN0_GPIO_Port        GPIOE
#define LCD_SW1_Pin           GPIO_PIN_12
#define LCD_SW1_GPIO_Port     GPIOE
#define LCD_SW3_Pin           GPIO_PIN_13
#define LCD_SW3_GPIO_Port     GPIOE
#define FAN1_TACH_Pin         GPIO_PIN_14
#define FAN1_TACH_GPIO_Port   GPIOE
#define FAN1_TACH_EXTI_IRQn   EXTI15_10_IRQn
#define LCD_SW2_Pin           GPIO_PIN_15
#define LCD_SW2_GPIO_Port     GPIOE
#define USB_HS_N_Pin          GPIO_PIN_14
#define USB_HS_N_GPIO_Port    GPIOB
#define USB_HS_P_Pin          GPIO_PIN_15
#define USB_HS_P_GPIO_Port    GPIOB
#define E_DIR_Pin             GPIO_PIN_8
#define E_DIR_GPIO_Port       GPIOD
#define E_STEP_Pin            GPIO_PIN_9
#define E_STEP_GPIO_Port      GPIOD
#define E_ENA_Pin             GPIO_PIN_10
#define E_ENA_GPIO_Port       GPIOD
#define LCD_RS_Pin            GPIO_PIN_11
#define LCD_RS_GPIO_Port      GPIOD
#define Y_DIR_Pin             GPIO_PIN_12
#define Y_DIR_GPIO_Port       GPIOD
#define Y_STEP_Pin            GPIO_PIN_13
#define Y_STEP_GPIO_Port      GPIOD
#define Y_ENA_Pin             GPIO_PIN_14
#define Y_ENA_GPIO_Port       GPIOD
#define Z_DIR_Pin             GPIO_PIN_15
#define Z_DIR_GPIO_Port       GPIOD
#define ESP_TX_Pin            GPIO_PIN_6
#define ESP_TX_GPIO_Port      GPIOC
#define ESP_RX_Pin            GPIO_PIN_7
#define ESP_RX_GPIO_Port      GPIOC
#define LCD_RST_Pin           GPIO_PIN_8
#define LCD_RST_GPIO_Port     GPIOC
#define LCD_CS_Pin            GPIO_PIN_9
#define LCD_CS_GPIO_Port      GPIOC
#define Z_MIN_Pin             GPIO_PIN_8
#define Z_MIN_GPIO_Port       GPIOA
#define Z_MIN_EXTI_IRQn       EXTI9_5_IRQn
#define USB_FS_N_Pin          GPIO_PIN_11
#define USB_FS_N_GPIO_Port    GPIOA
#define USB_FS_P_Pin          GPIO_PIN_12
#define USB_FS_P_GPIO_Port    GPIOA
#define SWDIO_Pin             GPIO_PIN_13
#define SWDIO_GPIO_Port       GPIOA
#define SWCLK_Pin             GPIO_PIN_14
#define SWCLK_GPIO_Port       GPIOA
#define E_DIAG_Pin            GPIO_PIN_15
#define E_DIAG_GPIO_Port      GPIOA
#define FLASH_SCK_Pin         GPIO_PIN_10
#define FLASH_SCK_GPIO_Port   GPIOC
#define FLASH_MISO_Pin        GPIO_PIN_11
#define FLASH_MISO_GPIO_Port  GPIOC
#define FLASH_MOSI_Pin        GPIO_PIN_12
#define FLASH_MOSI_GPIO_Port  GPIOC
#define X_DIR_Pin             GPIO_PIN_0
#define X_DIR_GPIO_Port       GPIOD
#define X_STEP_Pin            GPIO_PIN_1
#define X_STEP_GPIO_Port      GPIOD
#define Z_ENA_Pin             GPIO_PIN_2
#define Z_ENA_GPIO_Port       GPIOD
#define X_ENA_Pin             GPIO_PIN_3
#define X_ENA_GPIO_Port       GPIOD
#define Z_STEP_Pin            GPIO_PIN_4
#define Z_STEP_GPIO_Port      GPIOD
#define FLASH_CSN_Pin         GPIO_PIN_7
#define FLASH_CSN_GPIO_Port   GPIOD
#define FIL_SENSOR_Pin        GPIO_PIN_4
#define FIL_SENSOR_GPIO_Port  GPIOB
#define WP2_Pin               GPIO_PIN_5
#define WP2_GPIO_Port         GPIOB
#define TX1_Pin               GPIO_PIN_6
#define TX1_GPIO_Port         GPIOB
#define RX1_Pin               GPIO_PIN_7
#define RX1_GPIO_Port         GPIOB
#define WP1_Pin               GPIO_PIN_0
#define WP1_GPIO_Port         GPIOE
#define Z_DIAGE1_Pin          GPIO_PIN_1
#define Z_DIAGE1_GPIO_Port    GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
