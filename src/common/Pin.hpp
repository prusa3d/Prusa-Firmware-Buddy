/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#pragma once

#include "stm32f4xx_hal.h"

/**
 * @name Macros manipulating PIN_TABLE macro
 *
 * Define @p PIN_TABLE macro containing all physical pins used in project.
 * When defining @p PIN_TABLE use @p COMMA macro to separate parameters inside sections PORTPIN and PARAMETERS,
 * use ordinary comma (,) to separate sections (TYPE, NAME, PORTPIN, PARAMETERS).
 * @par Sections:
 * @n @p TYPE pin type e.g. InputPin, OutputPin, OutputInputPin, ...
 * @n @p NAME Name used to access pin. E.g. fastBoot, later accessed as e.g. fastboot.read()
 * @n @p PORTPIN Physical location of pin. E.g. IoPort::C COMMA IoPin::p7 or BUDDY_PIN(E0_DIR) for pin defined for MARLIN earlier.
 * @n @p PARAMETERS Parameters passed to pin constructor. Number and type of parameters varies between Pins @p TYPE
 *
 * @par Example usage:
 * @code
 * #define PIN_TABLE(MACRO_FUNCTION) \
 *      MACRO_FUNCTION(buddy::hw::OutputPin, e0Dir, BUDDY_PIN(E0_DIR), InitState::reset COMMA OMode::pushPull COMMA OSpeed::low) \
 *      MACRO_FUNCTION(buddy::hw::InputPin, fastBoot, IoPort::C COMMA IoPin::p7, IMode::input COMMA Pull::up)
 *
 * namespace buddy::hw {
 * DECLARE_PINS(PIN_TABLE)
 * }
 *
 * CONFIGURE_PINS(PIN_TABLE)
 *
 * constexpr PinChecker pinsToCheck[] = {
 *   PINS_TO_CHECK(PIN_TABLE)
 * };
 *
 * @endcode
 *
 * @{
 */
/**
 * @brief Separate parameters inside PIN_TABLE section.
 */
#define COMMA ,
/**
 * @brief Declare all pins supplied in PIN_TABLE parameter
 * @par Usage:
 * @code
 * DECLARE_PINS(PIN_TABLE)
 * @endcode
 */
#define DECLARE_PINS(TYPE, NAME, PORTPIN, PARAMETERS) inline constexpr TYPE NAME(PORTPIN, PARAMETERS);
/**
 * @brief Configure all pins supplied in PIN_TABLE parameter
 * @par Usage:
 * @code
 * CONFIGURE_PINS(PIN_TABLE)
 * @endcode
 */
#define CONFIGURE_PINS(TYPE, NAME, PORTPIN, PARAMETERS) buddy::hw::NAME.configure();
/**
 * @brief Generate array of physical location of all pins supplied in PIN_TABLE parameter
 * @par Usage:
 * @code
 * constexpr PinChecker pinsToCheck[] = {
 *   PINS_TO_CHECK(PIN_TABLE)
 * };
 * @endcode
 */
#define PINS_TO_CHECK(TYPE, NAME, PORTPIN, PARAMETERS) { PORTPIN },
/**@}*/

namespace buddy::hw {

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

/**
 * @brief Base class for all types of pins. Stores pin physical location.
 */
class Pin {
public:
    enum class State {
        low = GPIO_PinState::GPIO_PIN_RESET,
        high = GPIO_PinState::GPIO_PIN_SET,
    };

protected:
    constexpr Pin(IoPort ioPort, IoPin ioPin)
        : m_halPortBase(IoPortToHalBase(ioPort))
        , m_halPin(IoPinToHal(ioPin)) {}

    GPIO_TypeDef *getHalPort() const {
        return reinterpret_cast<GPIO_TypeDef *>(m_halPortBase);
    }

private:
    static constexpr uint32_t IoPortToHalBase(IoPort ioPort) {
        return (GPIOA_BASE + (static_cast<uint32_t>(ioPort) * (GPIOB_BASE - GPIOA_BASE)));
    }
    static constexpr uint16_t IoPinToHal(IoPin ioPin) {
        return (0x1U << static_cast<uint16_t>(ioPin));
    }

    const uint32_t m_halPortBase;

protected:
    const uint16_t m_halPin;
    friend class PinChecker;
};

/**
 * @brief Compile time checks Pin class, allows implementation of compile time checking of pin uniqueness.
 */
class PinChecker : public Pin {
public:
    constexpr PinChecker(IoPort ioPort, IoPin ioPin)
        : Pin(ioPort, ioPin) {}
    static_assert(Pin::IoPortToHalBase(IoPort::A) == GPIOA_BASE, "IoPortToHalBase broken.");
    static_assert(Pin::IoPortToHalBase(IoPort::B) == GPIOB_BASE, "IoPortToHalBase broken.");
    static_assert(Pin::IoPortToHalBase(IoPort::G) == GPIOG_BASE, "IoPortToHalBase broken.");
    static_assert(Pin::IoPinToHal(IoPin::p0) == GPIO_PIN_0, "IoPinToHal broken");
    static_assert(Pin::IoPinToHal(IoPin::p1) == GPIO_PIN_1, "IoPinToHal broken");
    static_assert(Pin::IoPinToHal(IoPin::p15) == GPIO_PIN_15, "IoPinToHal broken");
    constexpr uint32_t getPort() const { return m_halPortBase; }
    constexpr uint16_t getPin() const { return m_halPin; }
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

class InputPin : protected Pin {
public:
    constexpr InputPin(IoPort ioPort, IoPin ioPin, IMode iMode, Pull pull)
        : Pin(ioPort, ioPin)
        , m_mode(iMode)
        , m_pull(pull) {}
    State read() const {
        if ((getHalPort()->IDR & m_halPin) != (uint32_t)GPIO_PIN_RESET) {
            return State::high;
        } else {
            return State::low;
        }
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
    constexpr OutputPin(IoPort ioPort, IoPin ioPin, State initState, OMode oMode, OSpeed oSpeed)
        : Pin(ioPort, ioPin)
        , m_initState(initState)
        , m_mode(oMode)
        , m_speed(oSpeed) {}
    /**
     * @brief  Read output pin.
     *
     * Reads output data register. Can not work for alternate function pin.
     * @retval State::high
     * @retval State::low
     */
    State read() {
        if ((getHalPort()->ODR & m_halPin) != static_cast<uint32_t>(GPIO_PIN_RESET)) {
            return State::high;
        } else {
            return State::low;
        }
    }
    void write(State pinState) const {
        if (pinState != State::low) {
            getHalPort()->BSRR = m_halPin;
        } else {
            getHalPort()->BSRR = static_cast<uint32_t>(m_halPin) << 16U;
        }
    }
    void configure() const;

public:
    State m_initState;
    OMode m_mode;
    OSpeed m_speed;
};

/**
 * @brief This type of OutputPin allows runtime change of pin direction.
 *
 * Use InputEnabler to switch output pin to input and to get value.
 */
class OutputInputPin : public OutputPin {
public:
    constexpr OutputInputPin(IoPort ioPort, IoPin ioPin, State initState, OMode oMode, OSpeed oSpeed)
        : OutputPin(ioPort, ioPin, initState, oMode, oSpeed) {}

private:
    State read() const {
        if ((getHalPort()->IDR & m_halPin) != (uint32_t)GPIO_PIN_RESET) {
            return State::high;
        } else {
            return State::low;
        }
    }
    void enableInput(Pull pull) const;
    void enableOutput() const {
        configure();
    }
    friend class InputEnabler;
};

/**
 * @brief Enable OutputInputPin input mode when constructed, implements read(), revert OutputInputPin to output when destroyed.
 */
class InputEnabler {
public:
    InputEnabler(const OutputInputPin &outputInputPin, Pull pull)
        : m_outputInputPin(outputInputPin) {
        outputInputPin.enableInput(pull);
    }
    ~InputEnabler() {
        m_outputInputPin.enableOutput();
    }
    Pin::State read() {
        return m_outputInputPin.read();
    }

private:
    const OutputInputPin &m_outputInputPin;
};

} // namespace buddy::hw
