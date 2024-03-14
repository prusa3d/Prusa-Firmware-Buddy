#include "freertos_mutex.hpp"
#include "rtos_api.hpp"
#include "semphr.h"

void FreeRTOS_Mutex::unlock() { xSemaphoreGive(xSemaphore); }
bool FreeRTOS_Mutex::try_lock() { return xSemaphoreTake(xSemaphore, 0) == pdTRUE; }
void FreeRTOS_Mutex::lock() { xSemaphoreTake(xSemaphore, portMAX_DELAY); }
FreeRTOS_Mutex::FreeRTOS_Mutex() noexcept // ctor should be constexpr, but cannot due C code
    : xSemaphore(xSemaphoreCreateMutexStatic(&xSemaphoreData)) {
    configASSERT(xSemaphore); // The xSemaphoreData was not NULL, so it is expected that the handle will not be NULL.
};
FreeRTOS_Mutex::~FreeRTOS_Mutex() {
    // Empty here. But we need that one for tests.
}
void buddy::lock(std::unique_lock<FreeRTOS_Mutex> &l1, std::unique_lock<FreeRTOS_Mutex> &l2) {
    if (&l1 < &l2) {
        l1.lock();
        l2.lock();
    } else {
        l2.lock();
        l1.lock();
    }
}
