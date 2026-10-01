#pragma once

// FreeRTOS.h must be included before any other FreeRTOS header
#include <FreeRTOS.h>

namespace freertos {

constexpr TickType_t ms_to_ticks(TickType_t milliseconds) {
    // When passing timeouts to FreeRTOS you should use pdMS_TO_TICKS()
    // but the compiler generated multiplication and division combo.
    // Let's use the value as is, but ensure it is doing the same thing.
    static_assert(configTICK_RATE_HZ == 1000);
    static_assert(pdMS_TO_TICKS(42) == 42);
    return milliseconds;
}

constexpr TickType_t ticks_to_ms(TickType_t ticks) {
    // See comment in ms_to_ticks()
    static_assert(configTICK_RATE_HZ == 1000);
    return ticks;
}

} // namespace freertos

#undef pdMS_TO_TICKS
#pragma GCC poison pdMS_TO_TICKS
