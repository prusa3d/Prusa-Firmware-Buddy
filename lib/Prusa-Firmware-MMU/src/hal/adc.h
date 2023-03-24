/// @file adc.h
#pragma once
#include <stdint.h>

namespace hal {

/// Hardware Abstraction Layer for the ADC's
namespace adc {

/// ADC access routines
void Init();
uint16_t ReadADC(uint8_t channel);

} // namespace adc
} // namespace hal

namespace ha = hal::adc;
