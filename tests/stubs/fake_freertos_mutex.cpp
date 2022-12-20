#include <freertos_mutex.hpp>
#include <mutex>

using std::mutex;

/*
 * Here we fake the freertos mutex by putting a common C++ mutex inside.
 *
 * It's a bit of a stretch, we abuse the semaphore pointer in there to hold it.
 * It's probably OK for tests.
 */

FreeRTOS_Mutex::FreeRTOS_Mutex() noexcept
    : xSemaphore(reinterpret_cast<void *>(new mutex())) {}

FreeRTOS_Mutex::~FreeRTOS_Mutex() {
    delete reinterpret_cast<mutex *>(xSemaphore);
}

void FreeRTOS_Mutex::lock() {
    reinterpret_cast<mutex *>(xSemaphore)->lock();
}

void FreeRTOS_Mutex::unlock() {
    reinterpret_cast<mutex *>(xSemaphore)->unlock();
}
