#pragma once
#include "printers.h"
#include "board.h"
#include "stm32f4xx_hal.h"
#include "../src/common/uartrxbuff.h"
#include "../src/common/config_buddy_2209_02.h"

// Do not use HAL external interrupt handlers, use PIN_TABLE to setup and handle external interrupts instead
#pragma GCC poison HAL_GPIO_EXTI_IRQHandler HAL_GPIO_EXTI_Callback

#ifdef __cplusplus

/**Initializes the CPU, AHB and APB busses clocks  */
inline constexpr RCC_OscInitTypeDef RCC_OscInitStruct = [] {
    RCC_OscInitTypeDef rcc_OscInit {};
    rcc_OscInit.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
    rcc_OscInit.HSEState = RCC_HSE_ON;
    rcc_OscInit.LSIState = RCC_LSI_ON;
    rcc_OscInit.PLL = {
        .PLLState = RCC_PLL_ON,
        .PLLSource = RCC_PLLSOURCE_HSE,
        .PLLM = 6,
        .PLLN = 168,
        .PLLP = RCC_PLLP_DIV2,
        .PLLQ = 7
    };
    return rcc_OscInit;
}();

inline constexpr RCC_ClkInitTypeDef RCC_ClkInitStruct = {
    .ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
    .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
    .AHBCLKDivider = RCC_SYSCLK_DIV1,
    .APB1CLKDivider = RCC_HCLK_DIV4,
    .APB2CLKDivider = RCC_HCLK_DIV2,
};

constexpr unsigned long ConstexprSystemCoreClock_impl(RCC_OscInitTypeDef rcc_OscInit, RCC_ClkInitTypeDef rcc_ClkInit) {
    static_assert(2 == RCC_PLLP_DIV2);
    static_assert(4 == RCC_PLLP_DIV4);
    static_assert(6 == RCC_PLLP_DIV6);
    static_assert(8 == RCC_PLLP_DIV8);

    uint32_t clock_before_AHB_prescaler = 0;

    switch (rcc_ClkInit.SYSCLKSource) {
    case RCC_SYSCLKSOURCE_HSI:
        clock_before_AHB_prescaler = HSI_VALUE;
        break;
    case RCC_SYSCLKSOURCE_HSE:
        clock_before_AHB_prescaler = HSE_VALUE;
        break;
    case RCC_SYSCLKSOURCE_PLLCLK: {
        uint32_t src_clk = (RCC_PLLSOURCE_HSE == rcc_OscInit.PLL.PLLSource) ? HSE_VALUE : HSI_VALUE;
        uint32_t pllvco = (src_clk / rcc_OscInit.PLL.PLLM) * RCC_OscInitStruct.PLL.PLLN;
        clock_before_AHB_prescaler = pllvco / RCC_OscInitStruct.PLL.PLLP;
        break;
    }
    default:
        clock_before_AHB_prescaler = HSI_VALUE;
        break;
    }

    switch (rcc_ClkInit.AHBCLKDivider) {
    default:
    case RCC_SYSCLK_DIV1:
        return (clock_before_AHB_prescaler >>= 0);
    case RCC_SYSCLK_DIV2:
        return (clock_before_AHB_prescaler >>= 1);
    case RCC_SYSCLK_DIV4:
        return (clock_before_AHB_prescaler >>= 2);
    case RCC_SYSCLK_DIV8:
        return (clock_before_AHB_prescaler >>= 3);
    case RCC_SYSCLK_DIV16:
        return (clock_before_AHB_prescaler >>= 4);
    case RCC_SYSCLK_DIV64:
        return (clock_before_AHB_prescaler >>= 6);
    case RCC_SYSCLK_DIV128:
        return (clock_before_AHB_prescaler >>= 7);
    case RCC_SYSCLK_DIV256:
        return (clock_before_AHB_prescaler >>= 8);
    case RCC_SYSCLK_DIV512:
        return (clock_before_AHB_prescaler >>= 9);
    }
}

constexpr unsigned long ConstexprSystemCoreClock() {
    return ConstexprSystemCoreClock_impl(RCC_OscInitStruct, RCC_ClkInitStruct);
}

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
