#include <common/freertos_mutex.hpp>

// Do not check the concept on boards where #include <mutex> fills FLASH
#include <device/board.h>
#if !defined(BOARD_IS_MODULARBED) && !defined(BOARD_IS_DWARF)
    #include <common/concepts.hpp>
static_assert(concepts::Lockable<freertos::Mutex>);
#endif

namespace freertos {

Mutex::Mutex() noexcept {
    SemaphoreHandle_t semaphore = xSemaphoreCreateMutexStatic(&xSemaphoreData);
    // We are creating static FreeRTOS object here, supplying our own buffer
    // to be used by FreeRTOS. FreeRTOS constructs an object in that memory
    // and gives back a handle, which in current version is just a pointer
    // to the same buffer we provided. If this ever changes, we will have to
    // store the handle separately, but right now we can just use the pointer
    // to the buffer instead of the handle and save 4 bytes per instance.
    configASSERT(static_cast<void *>(semaphore) == static_cast<void *>(&xSemaphoreData));
}

Mutex::~Mutex() {
    // Empty here. But we need that one for tests.
}

void Mutex::unlock() {
    SemaphoreHandle_t semaphore = static_cast<SemaphoreHandle_t>(static_cast<void *>(&xSemaphoreData));
    BaseType_t result = xSemaphoreGive(semaphore);
    configASSERT(result == pdTRUE);
}

bool Mutex::try_lock() {
    SemaphoreHandle_t semaphore = static_cast<SemaphoreHandle_t>(static_cast<void *>(&xSemaphoreData));
    BaseType_t result = xSemaphoreTake(semaphore, 0);
    return result == pdTRUE;
}

void Mutex::lock() {
    SemaphoreHandle_t semaphore = static_cast<SemaphoreHandle_t>(static_cast<void *>(&xSemaphoreData));
    BaseType_t result = xSemaphoreTake(semaphore, portMAX_DELAY);
    configASSERT(result == pdTRUE);
}

} // namespace freertos
