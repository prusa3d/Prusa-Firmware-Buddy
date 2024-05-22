#pragma once

#include <cstdint>

namespace hal::System {

void SystemClock_Config();

uint32_t GetMicroeconds();
void WaitMicroseconds(uint32_t microseconds);

} // namespace hal::System
