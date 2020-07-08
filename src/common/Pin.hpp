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

private:
    virtual void configure() = 0;
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

constexpr GPIO_TypeDef *IoPortToHal(IoPort ioPort) {
    return reinterpret_cast<GPIO_TypeDef *>(static_cast<uint16_t>(ioPort) * (GPIOB_BASE - GPIOA_BASE));
}

constexpr uint16_t IoPinToHal(IoPin ioPin) {
    return (0x1U << static_cast<uint16_t>(ioPin));
}

static_assert(IoPinToHal(IoPin::p0) == GPIO_PIN_0, "IoPinToHal broken");
static_assert(IoPinToHal(IoPin::p15) == GPIO_PIN_15, "IoPinToHal broken");

class Pin {
protected:
    Pin(IoPort ioPort, IoPin ioPin)
        : m_halPort(IoPortToHal(ioPort))
        , m_halPin(IoPinToHal(ioPin)) {}
    GPIO_TypeDef *const m_halPort;
    const uint16_t m_halPin;
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

class InputPin : private ConfigurableIndestructible, public Pin {
public:
    InputPin(IoPort ioPort, IoPin ioPin, IMode iMode, Pull pull)
        : Pin(ioPort, ioPin)
        , m_mode(iMode)
        , m_pull(pull) {}
    GPIO_PinState read() {
        return HAL_GPIO_ReadPin(m_halPort, m_halPin);
    }

private:
    void configure() override;
    const IMode m_mode;
    const Pull m_pull;
};

enum class InitState : uint8_t {
    reset = GPIO_PinState::GPIO_PIN_RESET,
    set = GPIO_PinState::GPIO_PIN_SET,
};

enum class OMode : uint8_t {
    pushPull = GPIO_MODE_OUTPUT_PP,
    openDrain = GPIO_MODE_OUTPUT_OD,
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

class OutputPin : private ConfigurableIndestructible, public Pin {
public:
    OutputPin(IoPort ioPort, IoPin ioPin, InitState initState, OMode oMode, OSpeed oSpeed)
        : Pin(ioPort, ioPin)
        , m_initState(initState)
        , m_mode(oMode)
        , m_speed(oSpeed) {}
    /**
     * @brief  Read output pin.
     *
     * Reads output data register. Can not work for alternate function pin.
     * @retval GPIO_PIN_SET
     * @retval GPIO_PIN_RESET
     */
    GPIO_PinState read() {
        GPIO_PinState bitstatus;
        if ((m_halPort->ODR & m_halPin) != static_cast<uint32_t>(GPIO_PIN_RESET)) {
            bitstatus = GPIO_PIN_SET;
        } else {
            bitstatus = GPIO_PIN_RESET;
        }
        return bitstatus;
    }
    void write(GPIO_PinState pinState) {
        HAL_GPIO_WritePin(m_halPort, m_halPin, pinState);
    }

private:
    void configure() override;
    const InitState m_initState;
    const OMode m_mode;
    const OSpeed m_speed;
};
