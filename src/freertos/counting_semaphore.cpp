#include <freertos/counting_semaphore.hpp>

#include <cstdlib>

// FreeRTOS.h must be included before semphr.h
#include <FreeRTOS.h>
#include <semphr.h>

namespace freertos {

static SemaphoreHandle_t handle_cast(CountingSemaphore::Storage &semaphore_storage) {
    return static_cast<SemaphoreHandle_t>(static_cast<void *>(semaphore_storage.data()));
}

CountingSemaphore::CountingSemaphore(size_t max_count, size_t initial_count) {
    // If these asserts start failing, go fix the constants.
    static_assert(semaphore_storage_size == sizeof(StaticSemaphore_t));
    static_assert(semaphore_storage_align == alignof(StaticSemaphore_t));

    SemaphoreHandle_t semaphore = xSemaphoreCreateCountingStatic(max_count, initial_count, reinterpret_cast<StaticSemaphore_t *>(&semaphore_storage));
    // We are creating static FreeRTOS object here, supplying our own buffer
    // to be used by FreeRTOS. FreeRTOS constructs an object in that memory
    // and gives back a handle, which in current version is just a pointer
    // to the same buffer we provided. If this ever changes, we will have to
    // store the handle separately, but right now we can just use the pointer
    // to the buffer instead of the handle and save 4 bytes per instance.
    // Also, since we are using static semaphore, this should never be nullptr.
    configASSERT(semaphore == handle_cast(semaphore_storage));
}

CountingSemaphore::~CountingSemaphore() {
    vSemaphoreDelete(handle_cast(semaphore_storage));
}

void CountingSemaphore::release() {
    if (xSemaphoreGive(handle_cast(semaphore_storage)) != pdTRUE) {
        // Since the semaphore was obtained correctly, this should never happen.
        std::abort();
    }
}

void CountingSemaphore::acquire() {
    if (xSemaphoreTake(handle_cast(semaphore_storage), portMAX_DELAY) != pdTRUE) {
        static_assert(INCLUDE_vTaskSuspend);
        // Since we are waiting forever and have task suspension, this should never happen.
        std::abort();
    }
}

bool CountingSemaphore::try_acquire_for(size_t duration_ms) {
    return xSemaphoreTake(handle_cast(semaphore_storage), pdMS_TO_TICKS(duration_ms)) == pdTRUE;
}

} // namespace freertos
