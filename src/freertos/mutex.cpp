#include <freertos/mutex.hpp>

#include <cassert>
#include <cstdlib>

// FreeRTOS.h must be included before semphr.h
#include <FreeRTOS.h>
#include <semphr.h>

namespace freertos {

std::atomic<bool> Mutex::power_panic_mode_removeme = false;

Mutex::Mutex() {
    // If these asserts start failing, go fix the constants.
    static_assert(mutex_storage_size == sizeof(StaticSemaphore_t));
    static_assert(mutex_storage_align == alignof(StaticSemaphore_t));

    handle = xSemaphoreCreateMutexStatic(reinterpret_cast<StaticSemaphore_t *>(&mutex_storage));
}

Mutex::~Mutex() {
#if INCLUDE_xSemaphoreGetMutexHolder == 1
    assert(xSemaphoreGetMutexHolder(SemaphoreHandle_t(handle)) == nullptr);
#endif
    vSemaphoreDelete(SemaphoreHandle_t(handle));
}

void Mutex::unlock() {
    SemaphoreHandle_t handle = SemaphoreHandle_t(this->handle);

    // REMOVEME BFW-6418
    if (power_panic_mode_removeme) {
        // Only attempt unlock if we're the owning thread - could have been for example locked before PP by a different one
        // and we might have gotten here sooner due to task switching and lock failing
        if (xQueueGetMutexHolder(handle) == xTaskGetCurrentTaskHandle() && xSemaphoreGive(handle) != pdTRUE) {
            // Since the semaphore was obtained correctly, this should never happen.
            std::abort();
        }

    } else {
        if (xSemaphoreGive(handle) != pdTRUE) {
            // Since the semaphore was obtained correctly, this should never happen.
            std::abort();
        }
    }
}

bool Mutex::try_lock() {
    // Works the same regardless on the PP mode
    (void)power_panic_mode_removeme;

    return xSemaphoreTake(SemaphoreHandle_t(handle), 0) == pdTRUE;
}

void Mutex::lock() {
    if (xSemaphoreTake(SemaphoreHandle_t(handle), portMAX_DELAY) != pdTRUE) {
        // REMOVEME BFW-6418
        if (!power_panic_mode_removeme) {
            static_assert(INCLUDE_vTaskSuspend);
            // Since we are waiting forever and have task suspension, this should never happen.
            std::abort();
        }
    }
}

} // namespace freertos
