#pragma once

#include <stdint.h>
#include <vector>

namespace modules {
namespace time {

extern void ReinitTimebase(uint16_t ms = 0);
extern void IncMillis(uint16_t inc = 1);

} // namespace time
} // namespace modules

namespace mt = modules::time;
