#include "freertos_mutex.hpp"
#include "rtos_api.hpp"
#include "semphr.h"

// uncomment this to get buffer size as compile time error message
// char (*__give_me_buffere_size)[sizeof( StaticSemaphore_t )] = 1;
// use that size in header as buffer_size

static_assert(FreeRTOS_Mutex::buffer_size == sizeof(StaticSemaphore_t), "wrong mutex buffer size");

void FreeRTOS_Mutex::unlock() { xSemaphoreGive(xSemaphore); }
bool FreeRTOS_Mutex::try_lock() { return xSemaphoreTake(xSemaphore, 0) == pdTRUE; }
void FreeRTOS_Mutex::lock() { xSemaphoreTake(xSemaphore, portMAX_DELAY); }
FreeRTOS_Mutex::FreeRTOS_Mutex() noexcept //ctor should be constexpr, but cannot due C code
    : xSemaphore(xSemaphoreCreateMutexStatic(reinterpret_cast<StaticQueue_t *>(xMutexBuffer))) {
    configASSERT(xSemaphore); // The pxMutexBuffer was not NULL, so it is expected that the handle will not be NULL.
};
