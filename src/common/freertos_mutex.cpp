#include <common/freertos_mutex.hpp>

#include "rtos_api.hpp"
#include "semphr.h"

namespace freertos {

void Mutex::unlock() { xSemaphoreGive(xSemaphore); }
bool Mutex::try_lock() { return xSemaphoreTake(xSemaphore, 0) == pdTRUE; }
void Mutex::lock() { xSemaphoreTake(xSemaphore, portMAX_DELAY); }
Mutex::Mutex() noexcept // ctor should be constexpr, but cannot due C code
    : xSemaphore(xSemaphoreCreateMutexStatic(&xSemaphoreData)) {
    configASSERT(xSemaphore); // The xSemaphoreData was not NULL, so it is expected that the handle will not be NULL.
};
Mutex::~Mutex() {
    // Empty here. But we need that one for tests.
}

} // namespace freertos

void buddy::lock(std::unique_lock<freertos::Mutex> &l1, std::unique_lock<freertos::Mutex> &l2) {
    if (&l1 < &l2) {
        l1.lock();
        l2.lock();
    } else {
        l2.lock();
        l1.lock();
    }
}
