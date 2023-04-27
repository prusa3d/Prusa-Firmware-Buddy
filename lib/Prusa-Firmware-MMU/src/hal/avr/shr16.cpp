/// @file shr16.cpp
#include "../shr16.h"
#include "../gpio.h"
#include "../../pins.h"
#include <util/delay.h>
#include <util/atomic.h>

#define SHR16_LED_MSK 0xffc0
#define SHR16_DIR_MSK 0x0015
#define SHR16_ENA_MSK 0x002A

namespace hal {
namespace shr16 {

SHR16 shr16;

void SHR16::Init() {
    using namespace hal::gpio;
    gpio::Init(SHR16_DATA, GPIO_InitTypeDef(Mode::output, Level::low));
    gpio::Init(SHR16_LATCH, GPIO_InitTypeDef(Mode::output, Level::low));
    gpio::Init(SHR16_CLOCK, GPIO_InitTypeDef(Mode::output, Level::low));
    Write(SHR16_ENA_MSK);
}

void SHR16::Write(uint16_t v) {
    using namespace hal::gpio;
    for (uint16_t m = 0x8000; m; m >>= 1) {
        WritePin(SHR16_CLOCK, Level::low);
        WritePin(SHR16_DATA, (Level)((m & v) != 0));
        // _delay_us(1);
        WritePin(SHR16_CLOCK, Level::high);
        // _delay_us(1);
    }
    WritePin(SHR16_CLOCK, Level::low);
    WritePin(SHR16_LATCH, Level::high);
    _delay_us(15);
    WritePin(SHR16_LATCH, Level::low);
    _delay_us(15);

    shr16_v = v;
}

void SHR16::SetLED(uint16_t led) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        led = ((led & 0x00ff) << 8) | ((led & 0x0300) >> 2);
        Write((shr16_v & ~SHR16_LED_MSK) | led);
    }
}

void SHR16::SetTMCEnabled(uint8_t index, bool ena) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const uint16_t mask = 1 << (2 * index + 1);
        if (ena)
            Write(shr16_v & ~mask);
        else
            Write(shr16_v | mask);
    }
}

void SHR16::SetTMCDir(uint8_t index, bool dir) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        const uint16_t mask = 1 << (2 * index);
        if (dir)
            Write(shr16_v & ~mask);
        else
            Write(shr16_v | mask);
    }
}

} // namespace shr16
} // namespace hal
