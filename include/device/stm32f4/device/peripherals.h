#pragma once
#include <device/hal.h>
#include <device/board.h>
#include "printers.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    #define HAS_ADC3
#endif

//
// I2C
//

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c3;

//
// SPI
//

extern SPI_HandleTypeDef hspi2;
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern SPI_HandleTypeDef hspi3;
extern DMA_HandleTypeDef hdma_spi3_rx;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern SPI_HandleTypeDef hspi4;
extern DMA_HandleTypeDef hdma_spi4_tx;
extern SPI_HandleTypeDef hspi5;
extern DMA_HandleTypeDef hdma_spi5_tx;
extern DMA_HandleTypeDef hdma_spi5_rx;
extern SPI_HandleTypeDef hspi6;
extern DMA_HandleTypeDef hdma_spi6_tx;
extern DMA_HandleTypeDef hdma_spi6_rx;

//
// UART
//

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart6_rx;
extern DMA_HandleTypeDef hdma_usart6_tx;
extern UART_HandleTypeDef huart8;
extern DMA_HandleTypeDef hdma_uart8_rx;
extern DMA_HandleTypeDef hdma_uart8_tx;

//
// ADCs
//

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern ADC_HandleTypeDef hadc2;
extern DMA_HandleTypeDef hdma_adc2;
extern ADC_HandleTypeDef hadc3;
extern DMA_HandleTypeDef hdma_adc3;

//
// Timers
//

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim14;

//
// GPIO
//
// TODO: Migrate GPIO to Pin.hpp

#define USB_HS_N_Pin       GPIO_PIN_14
#define USB_HS_N_GPIO_Port GPIOB
#define USB_HS_P_Pin       GPIO_PIN_15
#define USB_HS_P_GPIO_Port GPIOB
#define THERM_0_Pin        GPIO_PIN_0
#define THERM_0_GPIO_Port  GPIOC

#if (BOARD_IS_XBUDDY && BOARD_VER_EQUAL_TO(0, 3, 4))
    #define BUZZER_Pin       GPIO_PIN_5
    #define BUZZER_GPIO_Port GPIOD
#else
    #define BUZZER_Pin       GPIO_PIN_0
    #define BUZZER_GPIO_Port GPIOA
#endif

#define HW_IDENTIFY_Pin       GPIO_PIN_3
#define HW_IDENTIFY_GPIO_Port GPIOA
#define THERM_1_Pin           GPIO_PIN_4
#define THERM_1_GPIO_Port     GPIOA

#if (BOARD_IS_BUDDY)
    #define ESP_TX_Pin        GPIO_PIN_6
    #define ESP_TX_GPIO_Port  GPIOC
    #define ESP_RX_Pin        GPIO_PIN_7
    #define ESP_RX_GPIO_Port  GPIOC
    #define THERM_2_Pin       GPIO_PIN_5
    #define THERM_2_GPIO_Port GPIOA
#else
    #define THERM_2_Pin       GPIO_PIN_10
    #define THERM_2_GPIO_Port GPIOF
#endif

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
#define MMU_TX_Pin            GPIO_PIN_6
#define MMU_TX_GPIO_Port      GPIOC
#define MMU_RX_Pin            GPIO_PIN_7
#define MMU_RX_GPIO_Port      GPIOC

#define BED_MON_Pin       GPIO_PIN_3
#define BED_MON_GPIO_Port GPIOA

#define HEATER_CURRENT_Pin       GPIO_PIN_3
#define HEATER_CURRENT_GPIO_Port GPIOF

#define INPUT_CURRENT_Pin       GPIO_PIN_4
#define INPUT_CURRENT_GPIO_Port GPIOF
#define THERM3_Pin              GPIO_PIN_5
#define THERM3_GPIO_Port        GPIOF
#define MMU_CURRENT_Pin         GPIO_PIN_6
#define MMU_CURRENT_GPIO_Port   GPIOF

#if (BOARD_IS_XBUDDY && BOARD_VER_EQUAL_TO(0, 3, 4))
    #define HEATER_VOLTAGE_Pin       GPIO_PIN_5
    #define HEATER_VOLTAGE_GPIO_Port GPIOA
#else
    #define HEATER_VOLTAGE_Pin       GPIO_PIN_3
    #define HEATER_VOLTAGE_GPIO_Port GPIOA

    #define BED_VOLTAGE_Pin       GPIO_PIN_5
    #define BED_VOLTAGE_GPIO_Port GPIOA
#endif

#define USB_OVERC_Pin               GPIO_PIN_4
#define USB_OVERC_GPIO_Port         GPIOE
#define ESP_GPIO0_Pin               GPIO_PIN_6
#define ESP_GPIO0_GPIO_Port         GPIOE
#define ESP_RST_Pin                 GPIO_PIN_13
#define ESP_RST_GPIO_Port           GPIOC
#define BED_MON_Pin                 GPIO_PIN_3
#define BED_MON_GPIO_Port           GPIOA
#define FANPRINT_TACH_Pin           GPIO_PIN_10
#define FANPRINT_TACH_GPIO_Port     GPIOE
#define FANPRINT_TACH_EXTI_IRQn     EXTI15_10_IRQn
#define FANHEATBREAK_TACH_Pin       GPIO_PIN_14
#define FANHEATBREAK_TACH_GPIO_Port GPIOE
#define FANHEATBREAK_TACH_EXTI_IRQn EXTI15_10_IRQn
#define SWDIO_Pin                   GPIO_PIN_13
#define SWDIO_GPIO_Port             GPIOA
#define SWCLK_Pin                   GPIO_PIN_14
#define SWCLK_GPIO_Port             GPIOA
#define WP2_Pin                     GPIO_PIN_5
#define WP2_GPIO_Port               GPIOB
#define WP1_Pin                     GPIO_PIN_0
#define WP1_GPIO_Port               GPIOE

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    #define TOUCH_SDA_PORT GPIOC
    #define TOUCH_SDA_PIN  GPIO_PIN_9
    #define TOUCH_SCL_PORT GPIOA
    #define TOUCH_SCL_PIN  GPIO_PIN_8
#endif

// Make our life a bit easier and don't distinguish between UART and USART
#define UART1 USART1
#define UART2 USART2
#define UART3 USART3
#define UART6 USART6

//
// Other
//

extern RTC_HandleTypeDef hrtc;
extern RNG_HandleTypeDef hrng;

//
// Initialization
//

void hw_rtc_init();
void hw_rng_init();

void hw_gpio_init();
void hw_dma_init();

void hw_adc1_init();
void hw_adc3_init();

void hw_uart1_init();
void hw_uart2_init();
void hw_uart3_init();
void hw_uart6_init();
void hw_uart8_init();

void hw_i2c1_init();
void hw_i2c2_init();
void hw_i2c3_init();

void hw_spi2_init();
void hw_spi3_init();
void hw_spi4_init();
void hw_spi5_init();
void hw_spi6_init();

void hw_tim1_init();
void hw_tim2_init();
void hw_tim3_init();
void hw_tim14_init();

//
// External Peripherals Assignment
//

#if BOARD_IS_BUDDY
    #define i2c_eeprom   1
    #define spi_flash    3
    #define spi_lcd      2
    #define uart_tmc     2
    #define uart_esp     6
    #define uart_extconn 1
    #define spi_extconn  1
#elif BOARD_IS_XBUDDY
    #define i2c_eeprom        2
    #define spi_flash         5
    #define spi_lcd           6
    #define spi_tmc           3
    #define uart_esp          8
    #define spi_accelerometer 2
    #define i2c_touch         3
    #define spi_extconn       4
    #define i2c_usbc          2
    #define uart_mmu          6
#elif BOARD_IS_XLBUDDY
    #define i2c_eeprom        2
    #define i2c_io_extender   2
    #define spi_flash         5
    #define spi_lcd           6
    #define spi_tmc           3
    #define uart_esp          8
    #define spi_accelerometer 2
    #define i2c_touch         3
    #define spi_led           4
    #define i2c_usbc          1
    #define uart_puppies      3
    #define uart_reserved     6
#else
    #error Unknown board
#endif

//
// Getters
//

#define __JOIN(prefix, number, suffix) prefix##number##suffix
#define _JOIN(...)                     __JOIN(__VA_ARGS__)

/// Get handle for given peripheral: I2C_HANDLE_FOR(touch) -> hi2c3
#define I2C_HANDLE_FOR(peripheral) _JOIN(hi2c, i2c_##peripheral, )

/// Get handle for given peripheral: SPI_HANDLE_FOR(lcd) -> hspi3
#define SPI_HANDLE_FOR(peripheral) _JOIN(hspi, spi_##peripheral, )

/// Get handle for given peripheral: UART_HANDLE_FOR(esp) -> huart3
#define UART_HANDLE_FOR(peripheral) _JOIN(huart, uart_##peripheral, )

/// Get handle for given DMA peripheral: UART_DMA_HANDLE_FOR(esp, rx) -> hdma_uart3_rx
#define UART_DMA_HANDLE_FOR(peripheral, rx_tx) _JOIN(hdma_uart, uart_##peripheral, _##rx_tx)

/// Call initialization function for given peripheral
/// Example: I2C_INIT(touch)
#define I2C_INIT(peripheral)               \
    _JOIN(hw_i2c, i2c_##peripheral, _init) \
    ()

/// Call initialization function for given peripheral
/// Example: SPI_INIT(lcd)
#define SPI_INIT(peripheral)               \
    _JOIN(hw_spi, spi_##peripheral, _init) \
    ()

/// Call initialization function for given peripheral
/// Example: UART_INIT(esp)
#define UART_INIT(peripheral)                \
    _JOIN(hw_uart, uart_##peripheral, _init) \
    ()

/// Get instance of given peripheral: I2C_INSTANCE_FOR(touch) -> I2C3
#define I2C_INSTANCE_FOR(peripheral) _JOIN(I2C, i2c_##peripheral, )

/// Get instance of given peripheral: SPI_INSTANCE_FOR(lcd) -> SPI3
#define SPI_INSTANCE_FOR(peripheral) _JOIN(SPI, spi_##peripheral, )

/// Get instance of given peripheral: UART_INSTANCE_FOR(esp) -> UART3
#define UART_INSTANCE_FOR(peripheral) _JOIN(UART, uart_##peripheral, )

#ifdef __cplusplus
}
#endif //__cplusplus
