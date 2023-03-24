/// @file watchdog.h
#pragma once
#include <stdint.h>

namespace hal {

/// Hardware Abstraction Layer for the CPU's internal watchdog
namespace watchdog {

#if defined(__AVR__)
constexpr uint32_t F_WDT = 128000; //frequency of the watchdog unit in Hz
constexpr uint32_t basePrescaler = 2048; //what prescalerBits==0 actually does.
constexpr uint8_t maxPrescaler = 9; //the maximum value prescalerBits can take
constexpr uint8_t reloadBits = 0; //number of bits in the reload register
#elif defined(__STM32__) //@todo to be changed to the final form
constexpr uint32_t F_WDT = 32000; //frequency of the watchdog unit in Hz
constexpr uint32_t basePrescaler = 4; //what prescalerBits==0 actually does.
constexpr uint8_t maxPrescaler = 6; //the maximum value prescalerBits can take
constexpr uint8_t reloadBits = 12; //number of bits in the reload register
#endif

struct configuration {
    uint8_t prescalerBits;
    uint16_t reload;

public:
    static constexpr configuration compute(uint16_t timeout) {
        // uint8_t prescalerBits = 0;
        // uint32_t ticks = timeout * F_WDT / (basePrescaler * (1 << prescalerBits));
        // while ((ticks >= (1 << reloadBits)) && (prescalerBits < maxPrescaler)) {
        //     prescalerBits++;
        //     ticks >>= 1;
        // }
        // if ((prescalerBits == 0) && (ticks == 0))
        //     ticks = 1; //1 tick is minimum
        // configuration config = { prescalerBits, static_cast<uint16_t>(ticks - 1) };
        // return config;
        uint8_t prescalerBits = 0;
        uint32_t ticks = 1;
        switch (timeout) {
        case 250:
            prescalerBits = 4;
            break;
        case 8000:
            prescalerBits = 9;
            break;
        }

        configuration config = { prescalerBits, static_cast<uint16_t>(ticks - 1) };
        return config;
    }
};

/// watchdog interface
void Enable(const configuration &config);
void Disable();
void Reset();

} // namespace watchdog
} // namespace hal

namespace hwd = hal::watchdog;
