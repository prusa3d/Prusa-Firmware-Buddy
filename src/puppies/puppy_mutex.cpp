#include <puppy_mutex.hpp>

#include <cassert>
#include <cstdlib>

// FreeRTOS.h must be included before semphr.h
#include <FreeRTOS.h>
#include <semphr.h>

namespace buddy::puppies {

static SemaphoreHandle_t handle_cast(PowerPanicMutex::Storage &mutex_storage) {
    return static_cast<SemaphoreHandle_t>(static_cast<void *>(mutex_storage.data()));
}

PowerPanicMutex::PowerPanicMutex() {
    // If these asserts start failing, go fix the constants.
    static_assert(storage_size == sizeof(StaticSemaphore_t));
    static_assert(storage_align == alignof(StaticSemaphore_t));

    SemaphoreHandle_t semaphore = xSemaphoreCreateMutexStatic(reinterpret_cast<StaticSemaphore_t *>(&mutex_storage));
    // We are creating static FreeRTOS object here, supplying our own buffer
    // to be used by FreeRTOS. FreeRTOS constructs an object in that memory
    // and gives back a handle, which in current version is just a pointer
    // to the same buffer we provided. If this ever changes, we will have to
    // store the handle separately, but right now we can just use the pointer
    // to the buffer instead of the handle and save 4 bytes per instance.
    configASSERT(semaphore == handle_cast(mutex_storage));
}

PowerPanicMutex::~PowerPanicMutex() {
    vSemaphoreDelete(handle_cast(mutex_storage));
}

void PowerPanicMutex::unlock() {
    if (locked) {
        locked = false;
        xSemaphoreGive(handle_cast(mutex_storage));
        // We do _not_ check the return value there, on purpose. It can happen we
        // acquired it before power panic and now we are power panicking and are
        // unable to unlock. At that point, it doesn't matter anyway.
    }
}

void PowerPanicMutex::lock() {
    if (xSemaphoreTake(handle_cast(mutex_storage), portMAX_DELAY) == pdTRUE) {
        locked = true;
    }
}

} // namespace buddy::puppies
