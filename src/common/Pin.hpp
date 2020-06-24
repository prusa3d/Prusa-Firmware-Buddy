/**
 * @file
 * @date Jun 23, 2020
 * @author Marek Bel
 */

#pragma once

#include "stm32f4xx_hal.h"

class LinkedListItem {
public:
    LinkedListItem()
        : m_previous(s_last) { s_last = this; }
    virtual ~LinkedListItem() = default;
    virtual void configure() = 0;
    static void configure_all();

private:
    LinkedListItem *const m_previous;
    static LinkedListItem *s_last;
};

enum class IoPort {
    A = 0,
    B,
    C,
    D,
    E,
    F,
    G,
};

enum class IoPin {
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

enum class Pull {
    none = GPIO_NOPULL,
    up = GPIO_PULLUP,
    down = GPIO_PULLDOWN,
};

/**
 * @brief output speed
 *
 * see chapter 8.4.3 in RM0090 Reference Manual
 */
enum class OSpeed {
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

template <IoPort ioPort, IoPin ioPin>
class Pin : LinkedListItem {
protected:
    static constexpr GPIO_TypeDef *m_HalPort = IoPortToHal(ioPort);
    static constexpr uint16_t m_HalPin = IoPinToHal(ioPin);
};

template <IoPort ioPort, IoPin ioPin, IMode iMode, Pull pull>
class InputPin : Pin<ioPort, ioPin> {
public:
    void configure() {
        GPIO_InitTypeDef GPIO_InitStruct = { 0 };
        GPIO_InitStruct.Pin = Pin<ioPort, ioPin>::m_HalPin;
        GPIO_InitStruct.Mode = m_iMode;
        GPIO_InitStruct.Pull = m_pull;
        HAL_GPIO_Init(Pin<ioPort, ioPin>::m_HalPort, GPIO_InitStruct);
    }

private:
    static constexpr IMode m_iMode = iMode;
    static constexpr Pull m_pull = pull;
};
