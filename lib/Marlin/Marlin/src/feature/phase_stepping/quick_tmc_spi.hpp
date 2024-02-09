#pragma once
#include "lut.hpp"

// This module has one responsibility: set XDirect register for given TMC driver
// as quick as possible with as little CPU intervention as possible.
//
// To do so we perform minimalistic and direct setup of the peripherals (SPI &
// DMA). We avoid any interrupts. Also, to ensure precise timing with minimal
// jitter, the CS line for 2130 is released by output compare of the main phase
// stepping timer. This ensures that no matter how long does it take to compute
// position, the command is latched into TMC driver at a constant frequency and
// phase. This is why also leave some slack between transmission complete and
// setting CS high.
//
// The contract is a follows:
// - start the transmission via set_xdirect. The invocation may fail if the SPI
//   line is busy. The function expects for the SPI to be configured for TMC
//   drivers (clock polarity/speed, etc.)
// - invoke finish_transmission after sufficient delay from set_xdirect. It is
//   safe to invoke this even when the previous set_xdirect failed.

namespace phase_stepping::spi {
bool initialize_transaction();
void set_xdirect(int axis, const CoilCurrents &currents);
void finish_transmission();
bool busy();
} // namespace phase_stepping::spi
