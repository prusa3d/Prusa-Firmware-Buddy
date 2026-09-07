/// @file
#pragma once

#include <cstdint>

#include "option/extension_variant.h"
static_assert(HAS_GPIO_EXPANDER());

namespace hal::gpio_expander {

/// GPIO expander pin definitions.
enum class Pin : uint8_t {
    mmu_power = 1 << 2,
    fan3 = 1 << 3,
    fan2 = 1 << 4,
    fan1 = 1 << 5,
};

/// Initialize GPIO expander to safe state.
void init();

/// Write a pin state.
void write(Pin, bool state);

/// Read a pin state.
bool read(Pin);

} // namespace hal::gpio_expander
