/**
 * @file
 * @date Nov 9, 2021
 * @author Marek Bel
 */

#include "inc/MarlinConfig.h"
#include "../../lib/Marlin/Marlin/src/HAL/HAL_STM32_F4_F7/endstop_ISR.h"
#include "log.h"
#include <type_traits>

#include "power_panic.hpp"
#if DISABLED(POWER_PANIC)
namespace power_panic {
std::atomic_bool ac_fault_triggered = false;

bool is_ac_fault_active() {
    return false;
}

// stub definition due to usage in the board pin macro table
void ac_fault_isr() {
    // Mark ac_fault as triggered
    ac_fault_triggered = true;
}
} // namespace power_panic
#endif

using namespace buddy::hw;

static constexpr uint16_t getIoHalPin(IoPort, IoPin ioPin) {
    return Pin::IoPinToHal(ioPin);
}

static constexpr bool isEXTI0Pin(IoPort, IoPin ioPin) {
    if (IoPin::p0 == ioPin) {
        return true;
    } else {
        return false;
    }
}
static constexpr bool isEXTI1Pin(IoPort, IoPin ioPin) {
    if (IoPin::p1 == ioPin) {
        return true;
    } else {
        return false;
    }
}
static constexpr bool isEXTI2Pin(IoPort, IoPin ioPin) {
    if (IoPin::p2 == ioPin) {
        return true;
    } else {
        return false;
    }
}
static constexpr bool isEXTI3Pin(IoPort, IoPin ioPin) {
    if (IoPin::p3 == ioPin) {
        return true;
    } else {
        return false;
    }
}
static constexpr bool isEXTI4Pin(IoPort, IoPin ioPin) {
    if (IoPin::p4 == ioPin) {
        return true;
    } else {
        return false;
    }
}

static constexpr bool isEXTI9_5Pin(IoPort, IoPin ioPin) {
    switch (ioPin) {
    case IoPin::p5:
    case IoPin::p6:
    case IoPin::p7:
    case IoPin::p8:
    case IoPin::p9:
        return true;
    default:
        return false;
    }
}

static constexpr bool isEXTI15_10Pin([[maybe_unused]] IoPort ioPort, IoPin ioPin) {
    switch (ioPin) {
    case IoPin::p10:
    case IoPin::p11:
    case IoPin::p12:
    case IoPin::p13:
    case IoPin::p14:
    case IoPin::p15:
        return true;
    default:
        return false;
    }
}

#define HANDLE_EXTI0_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                                 \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI0Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);                                                                 \
        traceISR_ENTER();                                                                                     \
        INTERRUPT_HANDLER();                                                                                  \
        traceISR_EXIT();                                                                                      \
    }

#define HANDLE_EXTI1_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                                 \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI1Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);                                                                 \
        traceISR_ENTER();                                                                                     \
        INTERRUPT_HANDLER();                                                                                  \
        traceISR_EXIT();                                                                                      \
    }

#define HANDLE_EXTI2_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                                 \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI2Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_2);                                                                 \
        traceISR_ENTER();                                                                                     \
        INTERRUPT_HANDLER();                                                                                  \
        traceISR_EXIT();                                                                                      \
    }

#define HANDLE_EXTI3_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                                 \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI3Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);                                                                 \
        traceISR_ENTER();                                                                                     \
        INTERRUPT_HANDLER();                                                                                  \
        traceISR_EXIT();                                                                                      \
    }

#define HANDLE_EXTI4_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                                 \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI4Pin(PORTPIN)) { \
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4);                                                                 \
        traceISR_ENTER();                                                                                     \
        INTERRUPT_HANDLER();                                                                                  \
        traceISR_EXIT();                                                                                      \
    }

#define HANDLE_EXTI9_5_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                              \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI9_5Pin(PORTPIN) \
        && (__HAL_GPIO_EXTI_GET_IT(getIoHalPin(PORTPIN)) != RESET)) {                                        \
        __HAL_GPIO_EXTI_CLEAR_IT(getIoHalPin(PORTPIN));                                                      \
        traceISR_ENTER();                                                                                    \
        INTERRUPT_HANDLER();                                                                                 \
        traceISR_EXIT();                                                                                     \
    }

#define HANDLE_EXTI15_10_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)                              \
    if ((std::is_same_v<TYPE, InterruptPin> || std::is_base_of_v<InterruptPin, TYPE>)&&isEXTI15_10Pin(PORTPIN) \
        && (__HAL_GPIO_EXTI_GET_IT(getIoHalPin(PORTPIN)) != RESET)) {                                          \
        __HAL_GPIO_EXTI_CLEAR_IT(getIoHalPin(PORTPIN));                                                        \
        traceISR_ENTER();                                                                                      \
        INTERRUPT_HANDLER();                                                                                   \
        traceISR_EXIT();                                                                                       \
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
