#include <device/hal.h>
#include <device/cmsis.h>

inline constexpr RCC_OscInitTypeDef RCC_OscInitStruct = [] {
    RCC_OscInitTypeDef rcc_OscInit {};
#if (BOARD_IS_BUDDY())
    rcc_OscInit.OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE;
#else
    rcc_OscInit.OscillatorType = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_HSE;
#endif
    rcc_OscInit.HSEState = RCC_HSE_ON;
#if (BOARD_IS_BUDDY())
    rcc_OscInit.LSIState = RCC_LSI_ON;
#else
    rcc_OscInit.LSEState = RCC_LSE_ON;
#endif
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

void system_core_init(void) {
    // Configure the main internal regulator output voltage
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Initializes the CPU, AHB and APB busses clocks
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        system_core_error_handler();
    }

    // Initializes the CPU, AHB and APB busses clocks
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        system_core_error_handler();
    }

    // TODO: shouldn't we call SystemCoreClockUpdate instead?
    SystemCoreClock = SYSTEM_CORE_CLOCK;

    RCC_PeriphCLKInitTypeDef periph_clk_init {};
    periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periph_clk_init.RTCClockSelection =
#if (BOARD_IS_BUDDY())
        RCC_RTCCLKSOURCE_LSI
#else
        RCC_RTCCLKSOURCE_LSE
#endif
        ;
    if (HAL_RCCEx_PeriphCLKConfig(&periph_clk_init) != HAL_OK) {
        system_core_error_handler();
    }
}
