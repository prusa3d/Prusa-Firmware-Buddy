#include <common/freertos_mutex.hpp>
#include <mutex>

namespace freertos {

Mutex::Mutex() noexcept {
}

Mutex::~Mutex() {
}

void Mutex::lock() {
    xSemaphoreData.lock();
}

void Mutex::unlock() {
    xSemaphoreData.unlock();
}

} // namespace freertos
