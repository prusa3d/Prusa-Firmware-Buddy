#include <freertos/mutex.hpp>

#include <cassert>
#include <cstdlib>

// FreeRTOS.h must be included before semphr.h
#include <FreeRTOS.h>
#include <semphr.h>

// Do not check the concept on boards where #include <mutex> fills FLASH
#ifndef UNITTESTS
    #include <device/board.h>
    #if !BOARD_IS_MODULARBED() && !BOARD_IS_DWARF()
        #include <common/concepts.hpp>
static_assert(concepts::Lockable<freertos::Mutex>);
    #endif
#endif

namespace freertos {

// If these asserts start failing, go fix the Storage definition
static_assert(Mutex::storage_size == sizeof(StaticSemaphore_t));
static_assert(Mutex::storage_align == alignof(StaticSemaphore_t));

static SemaphoreHandle_t handle_cast(Mutex::Storage &mutex_storage) {
    return static_cast<SemaphoreHandle_t>(static_cast<void *>(mutex_storage.data()));
}

std::atomic<bool> Mutex::power_panic_mode_removeme = false;

Mutex::Mutex() {
    SemaphoreHandle_t semaphore = xSemaphoreCreateMutexStatic(reinterpret_cast<StaticSemaphore_t *>(&mutex_storage));
    // We are creating static FreeRTOS object here, supplying our own buffer
    // to be used by FreeRTOS. FreeRTOS constructs an object in that memory
    // and gives back a handle, which in current version is just a pointer
    // to the same buffer we provided. If this ever changes, we will have to
    // store the handle separately, but right now we can just use the pointer
    // to the buffer instead of the handle and save 4 bytes per instance.
    configASSERT(semaphore == handle_cast(mutex_storage));
}

Mutex::~Mutex() {
#if INCLUDE_xSemaphoreGetMutexHolder == 1
    assert(xSemaphoreGetMutexHolder(handle_cast(mutex_storage)) == nullptr);
#endif
    vSemaphoreDelete(handle_cast(mutex_storage));
}

void Mutex::unlock() {
    SemaphoreHandle_t handle = handle_cast(mutex_storage);

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
    return xSemaphoreTake(handle_cast(mutex_storage), 0) == pdTRUE;
}

void Mutex::lock() {
    // REMOVEME BFW-6418
    if (power_panic_mode_removeme) {
        // In power panic mode, the defaultTask gets delays periodically aborted, which can result in mutexes takes failing
        xSemaphoreTake(handle_cast(mutex_storage), portMAX_DELAY);

    } else {
        if (xSemaphoreTake(handle_cast(mutex_storage), portMAX_DELAY) != pdTRUE) {
            static_assert(INCLUDE_vTaskSuspend);
            // Since we are waiting forever and have task suspension, this should never happen.
            std::abort();
        }
    }
}

} // namespace freertos
