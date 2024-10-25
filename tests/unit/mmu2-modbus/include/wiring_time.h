#pragma once
#include "timing.h"

inline uint32_t millis() { return ticks_ms(); }
