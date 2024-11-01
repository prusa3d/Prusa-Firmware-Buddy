#include <freertos/counting_semaphore.hpp>

#include <cstdlib>

// FreeRTOS.h must be included before semphr.h
#include <FreeRTOS.h>
#include <semphr.h>

namespace freertos {

CountingSemaphore::CountingSemaphore(size_t max_count, size_t initial_count) {
    // If these asserts start failing, go fix the constants.
    static_assert(semaphore_storage_size == sizeof(StaticSemaphore_t));
    static_assert(semaphore_storage_align == alignof(StaticSemaphore_t));

    handle = xSemaphoreCreateCountingStatic(max_count, initial_count, reinterpret_cast<StaticSemaphore_t *>(&semaphore_storage));
}

CountingSemaphore::~CountingSemaphore() {
    vSemaphoreDelete(SemaphoreHandle_t(handle));
}

void CountingSemaphore::release() {
    if (xSemaphoreGive(SemaphoreHandle_t(handle)) != pdTRUE) {
        // Since the semaphore was obtained correctly, this should never happen.
        std::abort();
    }
}

void CountingSemaphore::acquire() {
    if (xSemaphoreTake(SemaphoreHandle_t(handle), portMAX_DELAY) != pdTRUE) {
        static_assert(INCLUDE_vTaskSuspend);
        // Since we are waiting forever and have task suspension, this should never happen.
        std::abort();
    }
}

bool CountingSemaphore::try_acquire_for(size_t duration_ms) {
    return xSemaphoreTake(SemaphoreHandle_t(handle), pdMS_TO_TICKS(duration_ms)) == pdTRUE;
}

} // namespace freertos
