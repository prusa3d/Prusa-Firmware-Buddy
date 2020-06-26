/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#pragma once

#include "stm32f4xx_hal.h"

/**
 * @brief Container for all instances of its class
 *
 * Use only for objects of static storage duration. It is not possible meaningfully destroy object
 * of this class. (Do not allocate on stack or heap.)
 * Implicitly creates linked list of all created objects of its class. To conserve RAM, pointers to
 * previous items in list are constant.
 */
class ConfigurableIndestructible {
public:
    static void configure_all();
    ConfigurableIndestructible()
        : m_previous(s_last) { s_last = this; }
    virtual void configure() = 0;

private:
    ConfigurableIndestructible *const m_previous; //!< previous instance, nullptr if this is first instance
    static ConfigurableIndestructible *s_last;
};

enum class IoPort : uint8_t {
    A = 0,
    B,
    C,
    D,
    E,
    F,
    G,
};

enum class IoPin : uint8_t {
    p0 = 0,
    p1,
    p2,
    p3,
    p4,
    p5,
    p6,
    p7,
    p8,
    p9,
    p10,
    p11,
    p12,
    p13,
    p14,
    p15,
};

enum class IMode {
    input = GPIO_MODE_INPUT,
    IT_rising = GPIO_MODE_IT_RISING,
    IT_faling = GPIO_MODE_IT_FALLING,
};

enum class Pull : uint8_t {
    none = GPIO_NOPULL,
    up = GPIO_PULLUP,
    down = GPIO_PULLDOWN,
};

/**
 * @brief output speed
 *
 * see chapter 8.4.3 in RM0090 Reference Manual
 */
enum class OSpeed : uint8_t {
    low = GPIO_SPEED_FREQ_LOW,
    medium = GPIO_SPEED_FREQ_MEDIUM,
    high = GPIO_SPEED_FREQ_HIGH,
    very_high = GPIO_SPEED_FREQ_VERY_HIGH,
};

constexpr GPIO_TypeDef *IoPortToHal(IoPort ioPort) {
    return reinterpret_cast<GPIO_TypeDef *>(static_cast<uint16_t>(ioPort) * (GPIOB_BASE - GPIOA_BASE));
}

constexpr uint16_t IoPinToHal(IoPin ioPin) {
    return (0x1U << static_cast<uint16_t>(ioPin));
}

static_assert(IoPinToHal(IoPin::p0) == 0x0001U, "IoPinToHal broken");
static_assert(IoPinToHal(IoPin::p15) == 0x8000U, "IoPinToHal broken");

void InputPinGenericConfigure(uint16_t ioPin, IMode iMode, Pull pull, GPIO_TypeDef *Halport);

class InputPin : ConfigurableIndestructible {
public:
    InputPin(IoPort ioPort, IoPin ioPin, IMode iMode, Pull pull)
        : m_Halport(IoPortToHal(ioPort))
        , m_HalPin(IoPinToHal(ioPin))
        , m_iMode(iMode)
        , m_pull(pull) {}
    void configure() {
        InputPinGenericConfigure(m_HalPin, m_iMode, m_pull, m_Halport);
    }

private:
    GPIO_TypeDef *const m_Halport;
    const uint16_t m_HalPin;
    const IMode m_iMode;
    const Pull m_pull;
};
