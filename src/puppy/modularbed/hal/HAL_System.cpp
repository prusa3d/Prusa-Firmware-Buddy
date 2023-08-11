#include "hal/HAL_System.hpp"
#include "hal/HAL_Common.hpp"

#include "stm32g0xx_hal.h"

extern "C" {

extern TIM_HandleTypeDef htim1;
static uint32_t s_SystemMicroseconds = 0;

// interrupt from ARM-CORE timer
void SysTick_Handler() {
    // call FreeRtos's systick, as we have disabled its own SysTick_Handler
    extern void xPortSysTickHandler();
    xPortSysTickHandler();
}

/**
 * @brief This function handles TIM1 break, update, trigger and commutation interrupts.
 */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim1);
    s_SystemMicroseconds += 1000;
}

} // extern "C"

namespace hal::System {

void SystemClock_Config() {
    RCC_OscInitTypeDef RCC_OscInitStruct {};
    RCC_ClkInitTypeDef RCC_ClkInitStruct {};

    // Configure the main internal regulator output voltage
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    // Initializes the RCC Oscillators according to the specified parameters in the RCC_OscInitTypeDef structure.
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
    RCC_OscInitStruct.PLL.PLLN = 70;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV28;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV5;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

uint32_t GetMicroeconds() {
    CRITICAL_SECTION_START;

    uint32_t micros;
    uint32_t cnt = TIM1->CNT;

    if (cnt & 0x80000000) {
        micros = s_SystemMicroseconds + (cnt & 0x7FFFFFFF) + 1000;
    } else {
        micros = s_SystemMicroseconds + cnt;
    }

    CRITICAL_SECTION_END;

    return micros;
}

void WaitMicroseconds(uint32_t microseconds) {
    uint32_t start = GetMicroeconds();

    while ((GetMicroeconds() - start) < microseconds) {
    }
}

} // namespace hal::System
