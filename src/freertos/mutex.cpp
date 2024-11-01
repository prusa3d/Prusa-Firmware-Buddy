#include <freertos/mutex.hpp>

#include <cassert>
#include <cstdlib>

// FreeRTOS.h must be included before semphr.h
#include <FreeRTOS.h>
#include <semphr.h>

namespace freertos {

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
    if (xSemaphoreGive(SemaphoreHandle_t(handle)) != pdTRUE) {
        // Since the semaphore was obtained correctly, this should never happen.
        std::abort();
    }
}

bool Mutex::try_lock() {
    return xSemaphoreTake(SemaphoreHandle_t(handle), 0) == pdTRUE;
}

void Mutex::lock() {
    if (xSemaphoreTake(SemaphoreHandle_t(handle), portMAX_DELAY) != pdTRUE) {
        static_assert(INCLUDE_vTaskSuspend);
        // Since we are waiting forever and have task suspension, this should never happen.
        std::abort();
    }
}

} // namespace freertos
