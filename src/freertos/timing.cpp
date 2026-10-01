#include <freertos/timing.hpp>

// FreeRTOS.h must be included before task.h
#include <FreeRTOS.h>
#include <task.h>

#include "ticks.hpp"

namespace freertos {

void yield() {
    taskYIELD();
}

void delay(size_t milliseconds) {
    vTaskDelay(ms_to_ticks(milliseconds));
}

size_t millis() {
    return ticks_to_ms(xTaskGetTickCount());
}

} // namespace freertos
