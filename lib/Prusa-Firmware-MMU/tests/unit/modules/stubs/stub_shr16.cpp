#include "shr16.h"

namespace hal {
namespace shr16 {

SHR16 shr16;

uint16_t shr16_v_copy;
uint8_t shr16_tmc_dir;
uint8_t shr16_tmc_ena;

void SHR16::Init() {
    shr16_v_copy = 0;
    shr16_tmc_dir = 0;
    shr16_tmc_ena = 0;
}

void SHR16::SetLED(uint16_t led) {
    shr16_v_copy = ((led & 0x00ff) << 8) | ((led & 0x0300) >> 2);
}

void SHR16::SetTMCEnabled(uint8_t index, bool ena) {
    // this is using another array for testing convenience
    if (ena)
        shr16_tmc_ena |= (1 << index);
    else
        shr16_tmc_ena &= ~(1 << index);
}

void SHR16::SetTMCDir(uint8_t index, bool dir) {
    // this is using another array for testing convenience
    if (dir)
        shr16_tmc_dir |= (1 << index);
    else
        shr16_tmc_dir &= ~(1 << index);
}

void SHR16::Write(uint16_t v) {
    // do nothing right now
}

} // namespace shr16
} // namespace hal
