/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#pragma once

#include "stm32f4xx_hal.h"

/**
 * @name Use these macros only for pins used from Marlin.
 * @{
 */
#define MARLIN_PIN_NR_0  0
#define MARLIN_PIN_NR_1  1
#define MARLIN_PIN_NR_2  2
#define MARLIN_PIN_NR_3  3
#define MARLIN_PIN_NR_4  4
#define MARLIN_PIN_NR_5  5
#define MARLIN_PIN_NR_6  6
#define MARLIN_PIN_NR_7  7
#define MARLIN_PIN_NR_8  8
#define MARLIN_PIN_NR_9  9
#define MARLIN_PIN_NR_10 10
#define MARLIN_PIN_NR_11 11
#define MARLIN_PIN_NR_12 12
#define MARLIN_PIN_NR_13 13
#define MARLIN_PIN_NR_14 14
#define MARLIN_PIN_NR_15 15

#define MARLIN_PORT_A 0
#define MARLIN_PORT_B 1
#define MARLIN_PORT_C 2
#define MARLIN_PORT_D 3
#define MARLIN_PORT_E 4
#define MARLIN_PORT_F 5

#define MARLIN_PORT_PIN(port, pin) ((16 * (port)) + pin)

#define MARLIN_PIN(name) MARLIN_PORT_PIN(MARLIN_PORT_##name, MARLIN_PIN_NR_##name)
/**@}*/
/**
 * @brief Convert Marlin style defined pin to be used in constructor of Pin
 */
#define BUDDY_PIN(name) static_cast<IoPort>(MARLIN_PORT_##name), static_cast<IoPin>(MARLIN_PIN_NR_##name)

#define COMMA ,

#define DECLARE_PINS(TYPE, NAME, PORTPIN, PARAMETERS) inline constexpr TYPE NAME(PORTPIN, PARAMETERS);
#define DEFINE_PINS(TYPE, NAME, PORTPIN, PARAMETERS)
#define CONFIGURE_PINS(TYPE, NAME, PORTPIN, PARAMETERS) NAME.configure();
#define PINS_TO_CHECK(TYPE, NAME, PORTPIN, PARAMETERS)  { PORTPIN },

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

class Pin {
public:
    constexpr Pin(IoPort ioPort, IoPin ioPin)
        : m_halPortBase(IoPortToHalBase(ioPort))
        , m_halPin(IoPinToHal(ioPin)) {}
    static constexpr uint32_t IoPortToHalBase(IoPort ioPort) {
        return (GPIOA_BASE + (static_cast<uint32_t>(ioPort) * (GPIOB_BASE - GPIOA_BASE)));
    }
    GPIO_TypeDef *halPort() const {
        return reinterpret_cast<GPIO_TypeDef *>(m_halPortBase);
    }
    static constexpr uint16_t IoPinToHal(IoPin ioPin) {
        return (0x1U << static_cast<uint16_t>(ioPin));
    }
    const uint32_t m_halPortBase;
    const uint16_t m_halPin;
};

static_assert(Pin::IoPortToHalBase(IoPort::A) == GPIOA_BASE, "IoPortToHalBase broken.");
static_assert(Pin::IoPortToHalBase(IoPort::B) == GPIOB_BASE, "IoPortToHalBase broken.");
static_assert(Pin::IoPortToHalBase(IoPort::G) == GPIOG_BASE, "IoPortToHalBase broken.");
static_assert(Pin::IoPinToHal(IoPin::p0) == GPIO_PIN_0, "IoPinToHal broken");
static_assert(Pin::IoPinToHal(IoPin::p1) == GPIO_PIN_1, "IoPinToHal broken");
static_assert(Pin::IoPinToHal(IoPin::p15) == GPIO_PIN_15, "IoPinToHal broken");

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

class InputPin : protected Pin {
public:
    constexpr InputPin(IoPort ioPort, IoPin ioPin, IMode iMode, Pull pull)
        : Pin(ioPort, ioPin)
        , m_mode(iMode)
        , m_pull(pull) {}
    GPIO_PinState read() const {
        return HAL_GPIO_ReadPin(reinterpret_cast<GPIO_TypeDef *>(m_halPortBase), m_halPin);
    }
    void pullUp() const { configure(Pull::up); }
    void pullDown() const { configure(Pull::down); }
    void configure() const { configure(m_pull); }

private:
    void configure(Pull pull) const;

public:
    IMode m_mode;
    Pull m_pull;
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

class OutputPin : protected Pin {
public:
    constexpr OutputPin(IoPort ioPort, IoPin ioPin, InitState initState, OMode oMode, OSpeed oSpeed)
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
        if ((reinterpret_cast<GPIO_TypeDef *>(m_halPortBase)->ODR & m_halPin) != static_cast<uint32_t>(GPIO_PIN_RESET)) {
            bitstatus = GPIO_PIN_SET;
        } else {
            bitstatus = GPIO_PIN_RESET;
        }
        return bitstatus;
    }
    void write(GPIO_PinState pinState) const {
        HAL_GPIO_WritePin(reinterpret_cast<GPIO_TypeDef *>(m_halPortBase), m_halPin, pinState);
    }
    void configure() const;

public:
    InitState m_initState;
    OMode m_mode;
    OSpeed m_speed;
};

class OutputInputPin : public OutputPin {
public:
    constexpr OutputInputPin(IoPort ioPort, IoPin ioPin, InitState initState, OMode oMode, OSpeed oSpeed)
        : OutputPin(ioPort, ioPin, initState, oMode, oSpeed) {}

private:
    GPIO_PinState read() const {
        return HAL_GPIO_ReadPin(reinterpret_cast<GPIO_TypeDef *>(m_halPortBase), m_halPin);
    }
    void enableInput(Pull pull) const;
    void enableOutput() const {
        configure();
    }
    friend class InputEnabler;
};

class InputEnabler {
public:
    InputEnabler(const OutputInputPin &outputInputPin, Pull pull)
        : m_outputInputPin(outputInputPin) {
        outputInputPin.enableInput(pull);
    }
    ~InputEnabler() {
        m_outputInputPin.enableOutput();
    }
    GPIO_PinState read() {
        return m_outputInputPin.read();
    }

private:
    const OutputInputPin &m_outputInputPin;
};
