#pragma once
#include "printers.h"
#include "board.h"
#include "peripherals.h"
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

void rcc_osc_get_init(RCC_OscInitTypeDef *init);
void rcc_clk_get_init(RCC_ClkInitTypeDef *init);
unsigned long system_core_get_clock();

void main_cpp();

extern int HAL_GPIO_Initialized;
extern int HAL_ADC_Initialized;
extern int HAL_PWM_Initialized;

extern uartrxbuff_t uart1rxbuff;

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
void Error_Handler(void);

void spi_set_prescaler(SPI_HandleTypeDef *hspi, int prescaler_num);

#ifdef __cplusplus
}
#endif //__cplusplus
