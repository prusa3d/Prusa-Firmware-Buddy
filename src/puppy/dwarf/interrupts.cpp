#include <device/cmsis.h>
#include <device/peripherals.h>
#include "hal/HAL_RS485.hpp"
#include "loadcell.hpp"
#include "timing_sys.h"
#include "tool_filament_sensor.hpp"
#include "fanctl.hpp"
#include "task_startup.h"
#include "accelerometer.hpp"

extern "C" {

void SysTick_Handler() {
    // call FreeRtos's systick, as we have disabled its own SysTick_Handler
    extern void xPortSysTickHandler();
    xPortSysTickHandler();
}

void DMA1_Channel1_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma_adc1);
}

// TIM1_CC_IRQHandler is implemented by Marlin
//
// void TIM1_CC_IRQHandler() {
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

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM14) {
        HAL_IncTick();

        if (dwarf_init_done) {
            dwarf::tool_filament_sensor::tool_filament_sensor_irq();
            Fans::tick();
        }
    }
}

} // extern "C"

// Handle interrupts defined in PIN_TABLE
#include "hwio_pindef.h"
using namespace buddy::hw;

static constexpr uint16_t getIoHalPin(IoPort, IoPin ioPin) {
    return Pin::IoPinToHal(ioPin);
}

static constexpr bool isEXTI2_3Pin(IoPort, IoPin ioPin) {
    switch (ioPin) {
    case IoPin::p2:
    case IoPin::p3:
        return true;
    default:
        return false;
    }
}

#define HANDLE_EXTI2_3_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                              \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI2_3Pin(PORTPIN) \
        && (__HAL_GPIO_EXTI_GET_IT(getIoHalPin(PORTPIN)) != RESET)) {                                        \
        __HAL_GPIO_EXTI_CLEAR_IT(getIoHalPin(PORTPIN));                                                      \
        traceISR_ENTER();                                                                                    \
        INTERRUPT_HANDLER();                                                                                 \
        traceISR_EXIT();                                                                                     \
    }

extern "C" void EXTI2_3_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI2_3_PINS)
    return;
}

static constexpr bool isEXTI0_1Pin(IoPort, IoPin ioPin) {
    switch (ioPin) {
    case IoPin::p0:
    case IoPin::p1:
        return true;
    default:
        return false;
    }
}

#define HANDLE_EXTI0_1_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                              \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI0_1Pin(PORTPIN) \
        && (__HAL_GPIO_EXTI_GET_IT(getIoHalPin(PORTPIN)) != RESET)) {                                        \
        __HAL_GPIO_EXTI_CLEAR_IT(getIoHalPin(PORTPIN));                                                      \
        traceISR_ENTER();                                                                                    \
        INTERRUPT_HANDLER();                                                                                 \
        traceISR_EXIT();                                                                                     \
    }

extern "C" void EXTI0_1_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI0_1_PINS)
    return;
}
