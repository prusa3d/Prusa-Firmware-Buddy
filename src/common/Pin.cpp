/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#include "Pin.hpp"
#include "bsod.h"
#include <device/mcu.h>
#include <device/cmsis.h>

namespace buddy::hw {

void InputPin::configure(Pull pull) const {
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Pull = static_cast<uint32_t>(pull);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

void OutputPin::configure() const {
    HAL_GPIO_WritePin(getHalPort(), m_halPin, static_cast<GPIO_PinState>(m_initState));
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Speed = static_cast<uint32_t>(m_speed);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

void OutputPin_Inverted::configure() const {
    HAL_GPIO_WritePin(getHalPort(), m_halPin, static_cast<GPIO_PinState>(m_initState));
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(m_mode);
    GPIO_InitStruct.Speed = static_cast<uint32_t>(m_speed);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

void OutputInputPin::enableInput(Pull pull) const {
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = static_cast<uint32_t>(pull);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

void InterruptPin::configure() const {
    InputPin::configure();
    if (!isIRQEnabled()) {
        HAL_NVIC_SetPriority(getIRQn(), m_priority.preemptPriority, m_priority.subPriority);
        if (m_startEnabled) {
            enableIRQ();
        }
    } else {
#if MCU_IS_STM32F4()
        uint32_t priorityGroup = HAL_NVIC_GetPriorityGrouping();
        uint32_t preemptPriority;
        uint32_t subPriority;
        HAL_NVIC_GetPriority(getIRQn(), priorityGroup, &preemptPriority, &subPriority);
        if ((preemptPriority != m_priority.preemptPriority) || (subPriority != m_priority.subPriority)) {
            bsod("IRQ priority mismatch."); // The same IRQ was already enabled, but with different priority.
        }
#elif MCU_IS_STM32G0()
        // TODO: this is not yet implemented
        system_core_error_handler();
#else
    #error InterruptPin::configure() is not defined for this MCU
#endif
    }
}

IRQn_Type InterruptPin::getIRQn() const {
    switch (m_halPin) {
#if MCU_IS_STM32F4()
    case GPIO_PIN_0:
        return EXTI0_IRQn;
    case GPIO_PIN_1:
        return EXTI1_IRQn;
    case GPIO_PIN_2:
        return EXTI2_IRQn;
    case GPIO_PIN_3:
        return EXTI3_IRQn;
    case GPIO_PIN_4:
        return EXTI4_IRQn;
    case GPIO_PIN_5:
    case GPIO_PIN_6:
    case GPIO_PIN_7:
    case GPIO_PIN_8:
    case GPIO_PIN_9:
        return EXTI9_5_IRQn;
    case GPIO_PIN_10:
    case GPIO_PIN_11:
    case GPIO_PIN_12:
    case GPIO_PIN_13:
    case GPIO_PIN_14:
    case GPIO_PIN_15:
        return EXTI15_10_IRQn;
#elif MCU_IS_STM32G0()
    case GPIO_PIN_0:
    case GPIO_PIN_1:
        return EXTI0_1_IRQn;
    case GPIO_PIN_2:
    case GPIO_PIN_3:
        return EXTI2_3_IRQn;
    case GPIO_PIN_4:
    case GPIO_PIN_5:
    case GPIO_PIN_6:
    case GPIO_PIN_7:
    case GPIO_PIN_8:
    case GPIO_PIN_9:
    case GPIO_PIN_10:
    case GPIO_PIN_11:
    case GPIO_PIN_12:
    case GPIO_PIN_13:
    case GPIO_PIN_14:
    case GPIO_PIN_15:
        return EXTI4_15_IRQn;
#else
    #error InterruptPin::getIRQn() is not defined for this MCU
#endif
    default:
        bsod("Unexpected PIN.");
    }
}

void InputOutputPin::enableOutput(State pinState, OMode mode, OSpeed speed) const {
    HAL_GPIO_WritePin(getHalPort(), m_halPin, static_cast<GPIO_PinState>(pinState));
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = m_halPin;
    GPIO_InitStruct.Mode = static_cast<uint32_t>(mode);
    GPIO_InitStruct.Speed = static_cast<uint32_t>(speed);
    HAL_GPIO_Init(getHalPort(), &GPIO_InitStruct);
}

} // namespace buddy::hw
