#pragma once
#include <device/hal.h>
#include <device/board.h>

//
// ADCs
//

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

//
// Timers
//
extern TIM_HandleTypeDef htim14;

//
// SPIs
//
extern SPI_HandleTypeDef hspi2;

//
// Initialization
//

void hw_gpio_init(void);
void hw_dma_init(void);
void hw_adc1_init(void);
void hw_tim14_init(void);
void hw_spi2_init(void);

//
// GPIO Assignment
//

#define WHITE_LED_PWM_Pin           GPIO_PIN_11
#define WHITE_LED_PWM_GPIO_Port     GPIOC
#define HWID0_Pin                   GPIO_PIN_0
#define HWID0_GPIO_Port             GPIOC
#define HWID1_Pin                   GPIO_PIN_1
#define HWID1_GPIO_Port             GPIOC
#define HWID2_Pin                   GPIO_PIN_2
#define HWID2_GPIO_Port             GPIOC
#define HWID3_Pin                   GPIO_PIN_3
#define HWID3_GPIO_Port             GPIOC
#define RS485_DE_Pin                GPIO_PIN_12
#define RS485_DE_GPIO_Port          GPIOA
#define RS485_TX_Pin                GPIO_PIN_9
#define RS485_TX_GPIO_Port          GPIOA
#define RS485_RX_Pin                GPIO_PIN_10
#define RS485_RX_GPIO_Port          GPIOA
#define MEAS_24V_Pin                GPIO_PIN_10
#define MEAS_24V_GPIO_Port          GPIOB
#define NTC_Pin                     GPIO_PIN_5
#define NTC_GPIO_Port               GPIOA
#define TFS_GPIO_Port               GPIOA
#define TFS_Pin                     GPIO_PIN_4
#define NTC_INTERNAL_GPIO_Port      GPIOA
#define NTC_INTERNAL_Pin            GPIO_PIN_7
#define HEATER_PWM_Pin              GPIO_PIN_6
#define HEATER_PWM_GPIO_Port        GPIOA
#define PICKED0_Pin                 GPIO_PIN_2
#define PICKED0_GPIO_Port           GPIOA
#define PICKED1_Pin                 GPIO_PIN_1
#define PICKED1_GPIO_Port           GPIOA
#define HEATER_CURRENT_Pin          GPIO_PIN_0
#define HEATER_CURRENT_GPIO_Port    GPIOB
#define NTC2_GPIO_Port              GPIOB
#define NTC2_Pin                    GPIO_PIN_2
#define BUTT1_Pin                   GPIO_PIN_15
#define BUTT1_GPIO_Port             GPIOA
#define BUTT2_Pin                   GPIO_PIN_10
#define BUTT2_GPIO_Port             GPIOC
#define RGB_LED_Pin                 GPIO_PIN_6
#define RGB_LED_GPIO_Port           GPIOB
#define FAN0_PWM_Pin                GPIO_PIN_6
#define FAN0_PWM_GPIO_Port          GPIOC
#define FAN1_PWM_Pin                GPIO_PIN_7
#define FAN1_PWM_GPIO_Port          GPIOC
#define PANIC_Pin                   GPIO_PIN_11
#define PANIC_GPIO_Port             GPIOA
#define FAN0_TACH_Pin               GPIO_PIN_8
#define FAN0_TACH_GPIO_Port         GPIOC
#define FAN1_TACH_Pin               GPIO_PIN_9
#define FAN1_TACH_GPIO_Port         GPIOC
#define DIR_LOCAL_Pin               GPIO_PIN_0
#define DIR_LOCAL_GPIO_Port         GPIOD
#define STEP_LOCAL_Pin              GPIO_PIN_1
#define STEP_LOCAL_GPIO_Port        GPIOD
#define STEPPER_EN_Pin              GPIO_PIN_2
#define STEPPER_EN_GPIO_Port        GPIOD
#define STEPPER_DIAG_Pin            GPIO_PIN_3
#define STEPPER_DIAG_GPIO_Port      GPIOD
#define CTRL_LOCAL_REMOTE_Pin       GPIO_PIN_4
#define CTRL_LOCAL_REMOTE_GPIO_Port GPIOD
#define HX717_SCK_Pin               GPIO_PIN_0
#define HX717_SCK_GPIO_Port         GPIOA
#define HX717_DATA_Pin              GPIO_PIN_3
#define HX717_DATA_GPIO_Port        GPIOA
#define STEPPER_MISO_Pin            GPIO_PIN_5
#define STEPPER_MISO_GPIO_Port      GPIOD
#define STEPPER_MOSI_Pin            GPIO_PIN_6
#define STEPPER_MOSI_GPIO_Port      GPIOD
#define STEPPER_SCK_Pin             GPIO_PIN_3
#define STEPPER_SCK_GPIO_Port       GPIOB
#define STEPPER_CSN_Pin             GPIO_PIN_4
#define STEPPER_CSN_GPIO_Port       GPIOB

//
// External Peripherals Assignment
//

#define spi_accelerometer 2

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
