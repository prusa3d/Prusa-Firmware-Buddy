/**
 * Compile time check of main.h
 *
 * @file
 */

#include "../../include/main.h"

static constexpr RCC_OscInitTypeDef RCC_OscTest = [] {
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

static constexpr RCC_ClkInitTypeDef RCC_ClkTest = {
    .ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
    .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
    .AHBCLKDivider = RCC_SYSCLK_DIV1,
    .APB1CLKDivider = RCC_HCLK_DIV4,
    .APB2CLKDivider = RCC_HCLK_DIV2,
};

static_assert(168000000ul == ConstexprSystemCoreClock_impl(RCC_OscTest, RCC_ClkTest), "Test failed.");
