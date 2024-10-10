#include <freertos/delay.hpp>

// FreeRTOS.h must be included before task.h
#include <FreeRTOS.h>
#include <task.h>

namespace freertos {

void delay(size_t milliseconds) {
    // When calling vTaskDelay() you should use pdMS_TO_TICKS()
    // but the compiler generated multiplication and division combo.
    // Let's use the value as is, but ensure it is doing the same thing.
    static_assert(configTICK_RATE_HZ == 1000);
    static_assert(pdMS_TO_TICKS(42) == 42);
    vTaskDelay(milliseconds);
}

} // namespace freertos
