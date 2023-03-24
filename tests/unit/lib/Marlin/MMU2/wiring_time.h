/// Stub for wiring_time.h from Marlin/Buddy Arduino
/// gets compiled/included instead of the original one due to include path in the CMakeList.txt set to "."

#pragma once
#include <stdint.h>

extern "C" {
extern uint32_t millis(void);
}
