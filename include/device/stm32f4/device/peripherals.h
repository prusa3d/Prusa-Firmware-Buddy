#pragma once
#include <device/hal.h>
#include <device/board.h>
#include "printers.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
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
extern SPI_HandleTypeDef hspi3;
extern SPI_HandleTypeDef hspi4;
extern SPI_HandleTypeDef hspi5;
extern SPI_HandleTypeDef hspi6;

//
// ADCs
//

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;

//
// Timers
//

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim13;
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

#define BUZZER_Pin       GPIO_PIN_0
#define BUZZER_GPIO_Port GPIOA

#define HW_IDENTIFY_Pin       GPIO_PIN_3
#define HW_IDENTIFY_GPIO_Port GPIOA
#define THERM_1_Pin           GPIO_PIN_4
#define THERM_1_GPIO_Port     GPIOA

#if (BOARD_IS_BUDDY())
    #define ESP_TX_Pin            GPIO_PIN_6
    #define ESP_TX_GPIO_Port      GPIOC
    #define ESP_RX_Pin            GPIO_PIN_7
    #define ESP_RX_GPIO_Port      GPIOC
    #define THERM_2_Pin           GPIO_PIN_5
    #define THERM_2_GPIO_Port     GPIOA
    #define THERM_PINDA_Pin       GPIO_PIN_6
    #define THERM_PINDA_GPIO_Port GPIOA
#else
    #define THERM_2_Pin       GPIO_PIN_10
    #define THERM_2_GPIO_Port GPIOF
#endif

#if (BOARD_IS_XBUDDY() && !PRINTER_IS_PRUSA_MK3_5())
    #define THERM_HEATBREAK_Pin       GPIO_PIN_6
    #define THERM_HEATBREAK_GPIO_Port GPIOA
#endif

#define BED_HEAT_Pin         GPIO_PIN_0
#define BED_HEAT_GPIO_Port   GPIOB
#define HEAT0_Pin            GPIO_PIN_1
#define HEAT0_GPIO_Port      GPIOB
#define USB_FS_N_Pin         GPIO_PIN_11
#define USB_FS_N_GPIO_Port   GPIOA
#define USB_FS_P_Pin         GPIO_PIN_12
#define USB_FS_P_GPIO_Port   GPIOA
#define FLASH_SCK_Pin        GPIO_PIN_10
#define FLASH_SCK_GPIO_Port  GPIOC
#define FLASH_MISO_Pin       GPIO_PIN_11
#define FLASH_MISO_GPIO_Port GPIOC
#define FLASH_MOSI_Pin       GPIO_PIN_12
#define FLASH_MOSI_GPIO_Port GPIOC
#define TX1_Pin              GPIO_PIN_6
#define TX1_GPIO_Port        GPIOB
#define RX1_Pin              GPIO_PIN_7
#define RX1_GPIO_Port        GPIOB
#define MMU_TX_Pin           GPIO_PIN_6
#define MMU_TX_GPIO_Port     GPIOC
#define MMU_RX_Pin           GPIO_PIN_7
#define MMU_RX_GPIO_Port     GPIOC

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

#define HEATER_VOLTAGE_Pin       GPIO_PIN_3
#define HEATER_VOLTAGE_GPIO_Port GPIOA

#define BED_VOLTAGE_Pin       GPIO_PIN_5
#define BED_VOLTAGE_GPIO_Port GPIOA

#define USB_OVERC_Pin       GPIO_PIN_4
#define USB_OVERC_GPIO_Port GPIOE
#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    #define ESP_GPIO0_Pin GPIO_PIN_15
#else
    #define ESP_GPIO0_Pin GPIO_PIN_6
#endif
#define ESP_GPIO0_GPIO_Port         GPIOE
#define ESP_RST_Pin                 GPIO_PIN_13
#define ESP_RST_GPIO_Port           GPIOC
#define BED_MON_Pin                 GPIO_PIN_3
#define BED_MON_GPIO_Port           GPIOA
#define FANPRINT_TACH_Pin           GPIO_PIN_10
#define FANPRINT_TACH_GPIO_Port     GPIOE
#define FANHEATBREAK_TACH_Pin       GPIO_PIN_14
#define FANHEATBREAK_TACH_GPIO_Port GPIOE
#define SWDIO_Pin                   GPIO_PIN_13
#define SWDIO_GPIO_Port             GPIOA
#define SWCLK_Pin                   GPIO_PIN_14
#define SWCLK_GPIO_Port             GPIOA
#define WP2_Pin                     GPIO_PIN_5
#define WP2_GPIO_Port               GPIOB
#define WP1_Pin                     GPIO_PIN_0
#define WP1_GPIO_Port               GPIOE

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    #define i2c2_SDA_PORT_BASE GPIOF_BASE
    #define i2c2_SCL_PORT_BASE GPIOF_BASE
    #define i2c2_SDA_PORT      ((GPIO_TypeDef *)i2c2_SDA_PORT_BASE)
    #define i2c2_SCL_PORT      ((GPIO_TypeDef *)i2c2_SCL_PORT_BASE)
    #define i2c2_SDA_PIN       GPIO_PIN_0
    #define i2c2_SCL_PIN       GPIO_PIN_1

    // iX uses the I2C3 pins for back door filament sensor - BFW-4746
    #if !PRINTER_IS_PRUSA_iX()
        #define i2c3_SDA_PORT_BASE GPIOC_BASE
        #define i2c3_SCL_PORT_BASE GPIOA_BASE
        #define i2c3_SDA_PORT      ((GPIO_TypeDef *)i2c3_SDA_PORT_BASE)
        #define i2c3_SCL_PORT      ((GPIO_TypeDef *)i2c3_SCL_PORT_BASE)
        #define i2c3_SDA_PIN       GPIO_PIN_9
        #define i2c3_SCL_PIN       GPIO_PIN_8
    #endif
#endif

#if (BOARD_IS_XLBUDDY())
    #define i2c1_SDA_PORT_BASE GPIOB_BASE
    #define i2c1_SCL_PORT_BASE GPIOB_BASE
    #define i2c1_SDA_PORT      ((GPIO_TypeDef *)i2c1_SDA_PORT_BASE)
    #define i2c1_SCL_PORT      ((GPIO_TypeDef *)i2c1_SCL_PORT_BASE)
    #define i2c1_SDA_PIN       GPIO_PIN_7
    #define i2c1_SCL_PIN       GPIO_PIN_6
#endif

#if (BOARD_IS_BUDDY())
    #define i2c1_SDA_PORT_BASE GPIOB_BASE
    #define i2c1_SCL_PORT_BASE GPIOB_BASE
    #define i2c1_SDA_PORT      ((GPIO_TypeDef *)i2c1_SDA_PORT_BASE)
    #define i2c1_SCL_PORT      ((GPIO_TypeDef *)i2c1_SCL_PORT_BASE)
    #define i2c1_SDA_PIN       GPIO_PIN_9
    #define i2c1_SCL_PIN       GPIO_PIN_8
#endif

//
// External Peripherals Assignment
// -1 == don't have, currently i2c only
//

#if BOARD_IS_BUDDY()
    #define i2c_eeprom       1
    #define i2c_usbc         -1
    #define i2c_touch        -1
    #define i2c_gcode        1
    #define i2c_io_expander1 -1
    #define i2c_io_expander2 -1
    #define spi_flash        3
    #define spi_lcd          2
#elif BOARD_IS_XBUDDY()
    #define i2c_eeprom         2
    #define i2c_usbc           2
    #define i2c_gcode          2
    #define i2c_io_expander1   -1
    #define i2c_io_expander2   2
    #define spi_flash          5
    #define spi_lcd            6
    #define spi_tmc            3
    #define spi_accelerometer  2
    #define tim_phase_stepping 13
    #if PRINTER_IS_PRUSA_iX()
        /// iX uses the I2C3 pins for back door filament sensor - BFW-4746
        #define i2c_touch -1
        #define spi_led   4
    #else
        #define i2c_touch 3
        #define spi_led   -1
    #endif
#elif BOARD_IS_XLBUDDY()
    #define i2c_eeprom         2
    #define i2c_usbc           1
    #define i2c_touch          3
    #define i2c_gcode          2
    #define i2c_io_expander1   2
    #define i2c_io_expander2   -1
    #define spi_flash          5
    #define spi_lcd            6
    #define spi_tmc            3
    #define spi_accelerometer  2
    // Side LEDs use either SPI4 or share SPI with LCD, depending on HW revision
    #define spi_led            4
    #define tim_burst_stepping 8
    #define tim_phase_stepping 13
#else
    #error Unknown board
#endif

#define HAS_I2CN(n) ((n == i2c_eeprom) || (n == i2c_touch) || (n == i2c_usbc) || (n == i2c_gcode) || (n == i2c_io_expander1) || (n == i2c_io_expander2))

//
// Other
//

extern RTC_HandleTypeDef hrtc;
extern RNG_HandleTypeDef hrng;

//
// Initialization
//
#if PRINTER_IS_PRUSA_iX()
void hw_preinit_turbine_disable();
#endif

void hw_rtc_init();
void hw_rng_init();

void hw_gpio_init();
void hw_dma_init();

void hw_adc1_init();
void hw_adc3_init();

//
// i2c init may not exist without External Peripherals Assignment
// it contains busy flag clear function, and it requires pyhsical pins to compile
// i2c init is also used to clear BUSY flag
//

#if HAS_I2CN(1)
void hw_i2c1_init();
void hw_i2c1_pins_init();
#endif
#if HAS_I2CN(2)
void hw_i2c2_init();
void hw_i2c2_pins_init();
#endif
#if HAS_I2CN(3)
void hw_i2c3_init();
void hw_i2c3_pins_init();
#endif

void hw_spi2_init();
void hw_spi3_init();
void hw_spi4_init();
void hw_spi5_init();
void hw_spi6_init();

void hw_tim1_init();
void hw_tim2_init();
void hw_tim3_init();
void hw_tim8_init();
void hw_tim13_init();
void hw_tim14_init();

//
// Getters
//

#define __JOIN(prefix, number, suffix) prefix##number##suffix
#define _JOIN(...)                     __JOIN(__VA_ARGS__)

/// Get handle for given peripheral: I2C_HANDLE_FOR(touch) -> hi2c3
#define I2C_HANDLE_FOR(peripheral) _JOIN(hi2c, i2c_##peripheral, )

/// Get handle for given peripheral: SPI_HANDLE_FOR(lcd) -> hspi3
#define SPI_HANDLE_FOR(peripheral) _JOIN(hspi, spi_##peripheral, )

/// Get handle for given peripheral: TIM_HANDLE_FOR(phase_stepping) -> htim12
#define TIM_HANDLE_FOR(peripheral) _JOIN(htim, tim_##peripheral, )

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

#ifdef __cplusplus
}
#endif //__cplusplus
