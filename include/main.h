#pragma once
#include "printers.h"
#include "board.h"
#include "stm32f4xx_hal.h"
#include "../src/common/uartrxbuff.h"
#include "../src/common/config_buddy_2209_02.h"

// Do not use HAL external interrupt handlers, use PIN_TABLE to setup and handle external interrupts instead
#pragma GCC poison HAL_GPIO_EXTI_IRQHandler HAL_GPIO_EXTI_Callback

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int HAL_GPIO_Initialized;
extern int HAL_ADC_Initialized;
extern int HAL_PWM_Initialized;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
extern RTC_HandleTypeDef hrtc;
#if (BOARD_IS_BUDDY)
extern RNG_HandleTypeDef hrng;
    #if HAS_GUI
extern TIM_HandleTypeDef htim2; //TIM2 is used to generate buzzer PWM. Not needed without display.
extern SPI_HandleTypeDef hspi2; //SPI2 is used to drive display. Not needed without GUI.
        #define LCD_SPI hspi2
    #endif
extern I2C_HandleTypeDef hi2c1;
    #define EEPROM_I2C hi2c1
    #define LCD_I2C    hi2c1
extern SPI_HandleTypeDef hspi3;
    #define EXT_FLASH_SPI hspi3
#else
    #error "Unknown board."
#endif

extern UART_HandleTypeDef huart6;
extern RNG_HandleTypeDef hrng;

#if (1)
extern uartrxbuff_t uart1rxbuff;
#endif

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
void Error_Handler(void);

void spi_set_prescaler(SPI_HandleTypeDef *hspi, int prescaler_num);

#ifdef __cplusplus
}
#endif //__cplusplus

#define USB_HS_N_Pin          GPIO_PIN_14
#define USB_HS_N_GPIO_Port    GPIOB
#define USB_HS_P_Pin          GPIO_PIN_15
#define USB_HS_P_GPIO_Port    GPIOB
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
#define USB_FS_N_Pin          GPIO_PIN_11
#define USB_FS_N_GPIO_Port    GPIOA
#define USB_FS_P_Pin          GPIO_PIN_12
#define USB_FS_P_GPIO_Port    GPIOA
#define FLASH_SCK_Pin         GPIO_PIN_10
#define FLASH_SCK_GPIO_Port   GPIOC
#define FLASH_MISO_Pin        GPIO_PIN_11
#define FLASH_MISO_GPIO_Port  GPIOC
#define FLASH_MOSI_Pin        GPIO_PIN_12
#define FLASH_MOSI_GPIO_Port  GPIOC
#define TX1_Pin               GPIO_PIN_6
#define TX1_GPIO_Port         GPIOB
#define RX1_Pin               GPIO_PIN_7
#define RX1_GPIO_Port         GPIOB
#define ESP_TX_Pin            GPIO_PIN_6
#define ESP_TX_GPIO_Port      GPIOC
#define ESP_RX_Pin            GPIO_PIN_7
#define ESP_RX_GPIO_Port      GPIOC

#define BED_MON_Pin       GPIO_PIN_3
#define BED_MON_GPIO_Port GPIOA
