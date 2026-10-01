#include "hal.hpp"
#include <bsod.h>
#include <device/peripherals.hpp>
#include <device/hal.h>

// SVC_Handler + PendSV_Handler + SysTick_Handler are defined by FreeRTOS

void hal::panic() {
    NVIC_SystemReset();
}

void hal::reset() {
    NVIC_SystemReset();
}

namespace hal {
void init_clock();
} // namespace hal

void HAL_MspInit(void) {
    // These settings are important for the clock configuration. They would logically fit into
    // init_clock, but the STM32CubeMX generated project puts them here and we keep it that way so
    // it is easier to track possible future changes.
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void hal::init() {
    HAL_Init();
    hal::init_clock();
}

// 12 MHz HSE crystal, PLL: VCO = 12 MHz * 16 = 192 MHz, SYSCLK = VCO / 3 = 64 MHz
void hal::init_clock() {
    {
        RCC_OscInitTypeDef RCC_OscInitStruct = {};
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
        RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
        RCC_OscInitStruct.PLL.PLLN = 16;
        RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
        RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV3;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
            bsod_system();
        }
    }

    {
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
            bsod_system();
        }
    }
}

extern "C" void hal_panic() {
    hal::panic();
}

// Target of the HAL's assert_param
extern "C" void trigger_crash_dump() {
    bsod_system();
}

extern "C" void NMI_Handler() {
    bsod_system();
}

extern "C" void HardFault_Handler() {
    bsod_system();
}
