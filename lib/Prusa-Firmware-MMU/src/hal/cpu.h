/// @file cpu.h
#pragma once

namespace hal {

/// Hardware Abstraction Layer for the CPU
namespace cpu {

#ifndef F_CPU
/// Main clock frequency
#define F_CPU (16000000ul)
#endif

extern bool resetPending;

/// CPU init routines (not really necessary for the AVR)
void Init();
void Reset();
void Step();

} // namespace cpu
} // namespace hal
