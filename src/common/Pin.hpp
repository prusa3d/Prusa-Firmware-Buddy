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
#define DECLARE_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) inline constexpr TYPE NAME(PORTPIN, PARAMETERS);

/**
 * @brief Declare all pins supplied in VIRTUAL_PIN_TABLE parameter
 * @par Usage:
 * @code
 * DECLARE_VIRTUAL_PINS(VIRTUAL_PIN_TABLE)
 * @endcode
 */
#define DECLARE_VIRTUAL_PINS(TYPE, READ_FN, ISR_FN, NAME, PORTPIN, PARAMETERS) inline constexpr TYPE<READ_FN, ISR_FN> NAME(PARAMETERS);

/**
 * @brief Configure all pins supplied in PIN_TABLE parameter
 * @par Usage:
 * @code
 * CONFIGURE_PINS(PIN_TABLE)
 * @endcode
 */
#define CONFIGURE_PINS(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) buddy::hw::NAME.configure();
/**
 * @brief Generate array of physical location of all pins supplied in PIN_TABLE parameter
 * @par Usage:
 * @code
 * constexpr PinChecker pinsToCheck[] = {
 *   PINS_TO_CHECK(PIN_TABLE)
 * };
 * @endcode
 */
#define PINS_TO_CHECK(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) { PORTPIN },
/**@}*/

namespace buddy::hw {

inline void noHandler() {
}

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

    static constexpr uint16_t IoPinToHal(IoPin ioPin) {
        return (0x1U << static_cast<uint16_t>(ioPin));
    }

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
    IT_falling = GPIO_MODE_IT_FALLING,
    IT_rising_falling = GPIO_MODE_IT_RISING_FALLING,
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

protected:
    IMode m_mode;
    Pull m_pull;
};

/**
 *
 */
class InterruptPin : public InputPin {
public:
    /**
     * @brief InterruptPin constructor
     *
     * Priorities are shared between pins in a group. See InterruptPin::getIRQn() for grouping.
     * If it is impossible to fulfill requested priority, you will get bsod("IRQ priority mismatch.")
     * during configure() call.
     *
     * @param ioPort
     * @param ioPin
     * @param iMode
     * @param pull
     * @param preemptPriority Priority, highest priority is 0,
     *          maximum priority from which RTOS ISR API calls are allowed is configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
     *          minimum priority depends on processor and how available priority bits are assigned between
     *          priority and sub-priority.
     * @param subPriority Highest sub-priority is 0 (recommended as it is always available).
     *          Lowest sub-priority depends on how available priority bits are assigned between
     *          priority and sub-priority.
     */
    constexpr InterruptPin(IoPort ioPort, IoPin ioPin, IMode iMode, Pull pull, uint8_t preemptPriority, uint8_t subPriority)
        : InputPin(ioPort, ioPin, iMode, pull)
        , m_priority { preemptPriority, subPriority } {}
    void configure() const;
    IRQn_Type getIRQn() const;

private:
    struct Priority {
        uint8_t preemptPriority : 4;
        uint8_t subPriority : 4;
    };
    Priority m_priority;
};

typedef Pin::State (*ReadFunction)();
typedef void (*InterruptRoutine)();

template <ReadFunction readFunction, InterruptRoutine interruptRoutine>
class VirtualInterruptPin {
public:
    constexpr VirtualInterruptPin(IMode) {}
    void configure() const {}
    Pin::State read() const { return readFunction(); }
    void isr() const { interruptRoutine(); }
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

class DummyOutputPin : protected Pin {
public:
    constexpr DummyOutputPin(IoPort ioPort, IoPin ioPin, State initState, OMode oMode, OSpeed oSpeed)
        : Pin(ioPort, ioPin)
        , m_state(initState) {}
    /**
     * @brief  Read output pin.
     *
     * @return initState
     */
    State read() const { return m_state; }
    void write(State) const {}
    void configure() const {}

private:
    State m_state;
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
