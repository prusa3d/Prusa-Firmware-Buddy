/**
 * @file
 * @date Nov 9, 2021
 * @author Marek Bel
 */

#include "inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/endstop_ISR.h"
#include <type_traits>

static constexpr uint16_t getIoHalPin(buddy::hw::IoPort, buddy::hw::IoPin ioPin) {
    return buddy::hw::Pin::IoPinToHal(ioPin);
}

static constexpr bool isEXTI0Pin(buddy::hw::IoPort, buddy::hw::IoPin ioPin) {
    if (buddy::hw::IoPin::p0 == ioPin) {
        return true;
    } else {
        return false;
    }
}
static constexpr bool isEXTI1Pin(buddy::hw::IoPort, buddy::hw::IoPin ioPin) {
    if (buddy::hw::IoPin::p1 == ioPin) {
        return true;
    } else {
        return false;
    }
}
static constexpr bool isEXTI2Pin(buddy::hw::IoPort, buddy::hw::IoPin ioPin) {
    if (buddy::hw::IoPin::p2 == ioPin) {
        return true;
    } else {
        return false;
    }
}
static constexpr bool isEXTI3Pin(buddy::hw::IoPort, buddy::hw::IoPin ioPin) {
    if (buddy::hw::IoPin::p3 == ioPin) {
        return true;
    } else {
        return false;
    }
}
static constexpr bool isEXTI4Pin(buddy::hw::IoPort, buddy::hw::IoPin ioPin) {
    if (buddy::hw::IoPin::p4 == ioPin) {
        return true;
    } else {
        return false;
    }
}

static constexpr bool isEXTI9_5Pin(buddy::hw::IoPort, buddy::hw::IoPin ioPin) {
    switch (ioPin) {
    case buddy::hw::IoPin::p5:
    case buddy::hw::IoPin::p6:
    case buddy::hw::IoPin::p7:
    case buddy::hw::IoPin::p8:
    case buddy::hw::IoPin::p9:
        return true;
    default:
        return false;
    }
}

static constexpr bool isEXTI15_10Pin(buddy::hw::IoPort ioPort, buddy::hw::IoPin ioPin) {
    switch (ioPin) {
    case buddy::hw::IoPin::p10:
    case buddy::hw::IoPin::p11:
    case buddy::hw::IoPin::p12:
    case buddy::hw::IoPin::p13:
    case buddy::hw::IoPin::p14:
    case buddy::hw::IoPin::p15:
        return true;
    default:
        return false;
    }
}

#define HANDLE_EXTI0_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)   \
    if (std::is_same_v<TYPE, buddy::hw::InterruptPin> && isEXTI0Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);                                   \
        INTERRUPT_HANDLER();                                                    \
    }

#define HANDLE_EXTI1_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)   \
    if (std::is_same_v<TYPE, buddy::hw::InterruptPin> && isEXTI1Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);                                   \
        INTERRUPT_HANDLER();                                                    \
    }

#define HANDLE_EXTI2_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)   \
    if (std::is_same_v<TYPE, buddy::hw::InterruptPin> && isEXTI2Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);                                   \
        INTERRUPT_HANDLER();                                                    \
    }

#define HANDLE_EXTI3_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)   \
    if (std::is_same_v<TYPE, buddy::hw::InterruptPin> && isEXTI3Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);                                   \
        INTERRUPT_HANDLER();                                                    \
    }

#define HANDLE_EXTI4_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)   \
    if (std::is_same_v<TYPE, buddy::hw::InterruptPin> && isEXTI4Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);                                   \
        INTERRUPT_HANDLER();                                                    \
    }

#define HANDLE_EXTI9_5_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) \
    if (std::is_same_v<TYPE, buddy::hw::InterruptPin> && isEXTI9_5Pin(PORTPIN)  \
        && (__HAL_GPIO_EXTI_GET_IT(getIoHalPin(PORTPIN)) != RESET)) {           \
        __HAL_GPIO_EXTI_CLEAR_IT(getIoHalPin(PORTPIN));                         \
        INTERRUPT_HANDLER();                                                    \
    }

#define HANDLE_EXTI15_10_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) \
    if (std::is_same_v<TYPE, buddy::hw::InterruptPin> && isEXTI15_10Pin(PORTPIN)  \
        && (__HAL_GPIO_EXTI_GET_IT(getIoHalPin(PORTPIN)) != RESET)) {             \
        __HAL_GPIO_EXTI_CLEAR_IT(getIoHalPin(PORTPIN));                           \
        INTERRUPT_HANDLER();                                                      \
    }

extern "C" {

void EXTI0_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI0_PINS)
    return;
}

void EXTI1_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI1_PINS)
    return;
}

void EXTI2_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI2_PINS)
    return;
}

void EXTI3_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI3_PINS)
    return;
}

void EXTI4_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI4_PINS)
    return;
}

void EXTI9_5_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI9_5_PINS)
    return;
}

void EXTI15_10_IRQHandler() {
    PIN_TABLE(HANDLE_EXTI15_10_PINS)
    return;
}
} // extern "C"
