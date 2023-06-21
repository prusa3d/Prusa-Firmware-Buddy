#include <device/cmsis.h>
#include <device/peripherals.h>
#include "timing_sys.h"
#include "hal/HAL_RS485.hpp"
#include "loadcell.hpp"
#include "SEGGER_SYSVIEW_FreeRTOS.h"
#include "timing_sys.h"
#include "tool_filament_sensor.hpp"
#include "fanctl.hpp"
#include "task_startup.h"

extern "C" {

void SysTick_Handler() {
    // call FreeRtos's systick, as we have disabled its own SysTick_Handler
    extern void xPortSysTickHandler();
    xPortSysTickHandler();
}

void DMA1_Channel1_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_adc1);
}

// TIM6_IRQHandler is implemented by Marlin
//
// void TIM6_IRQHandler() {
// }

// TIM7_IRQHandler is implemented by Marlin
//
// void TIM7_IRQHandler() {
// }

void TIM14_IRQHandler() {
    // traceISR_ENTER();
    HAL_TIM_IRQHandler(&TimerSysHandle);
    // traceISR_EXIT();
}

void DMA1_Channel2_3_IRQHandler() {
    hal::RS485Driver::DMA_IRQHandler();
}

void USART1_IRQHandler() {
    hal::RS485Driver::USART_IRQHandler();
}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM14) {
        HAL_IncTick();

        if (dwarf_init_done) {
            dwarf::loadcell::loadcell_irq();
            dwarf::tool_filament_sensor::tool_filament_sensor_irq();
            Fans::tick();
        }
    }
}
