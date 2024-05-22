#pragma once

#include <cstdint>

namespace hal::PWMDriver {

bool Init();

void AddPWMPulse(uint32_t heatbedletIndex, uint32_t pulseStartEdge, uint32_t pulseLength);
void ApplyPWMPattern();
void TurnOffAll();

} // namespace hal::PWMDriver
