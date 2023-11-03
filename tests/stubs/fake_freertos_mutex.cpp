#include <common/freertos_mutex.hpp>
#include <mutex>

/*
 * Here we fake the freertos mutex by putting a common C++ mutex inside.
 *
 * It's a bit of a stretch, we abuse the semaphore pointer in there to hold it.
 * It's probably OK for tests.
 */
namespace freertos {

Mutex::Mutex() noexcept
    : xSemaphore(reinterpret_cast<void *>(new std::mutex())) {}

Mutex::~Mutex() {
    delete reinterpret_cast<std::mutex *>(xSemaphore);
}

void Mutex::lock() {
    reinterpret_cast<std::mutex *>(xSemaphore)->lock();
}

void Mutex::unlock() {
    reinterpret_cast<std::mutex *>(xSemaphore)->unlock();
}

} // namespace freertos
