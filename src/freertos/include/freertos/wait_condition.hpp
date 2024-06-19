#pragma once

#include <atomic>
#include <freertos/binary_semaphore.hpp>
#include <mutex>

namespace freertos {

/// Introduces a condition variable
class WaitCondition {

public:
    template <typename T>
    void wait(std::unique_lock<T> &locked_mutex) {
        // Check that the mutex is locked
        assert(locked_mutex);

        waiter_count++;
        locked_mutex.unlock();

        // We cannot have this inside the mutex, because acquire will likely lock and then we would have a deadlock.
        // But that is okay, two things can happen:
        // - if notify() is called correctly after acquire, the notify will simple release the semaphore, so we will get notified
        // - if notify() happens to be called between unlock() and acquire(), it will hang on a bit on release_blocking(), waiting the acquire()
        semaphore.acquire();

        locked_mutex.lock();
    }

    void notify_one();

    void notify_all();

private:
    BinarySemaphore semaphore;
    std::atomic<int> waiter_count = 0;
};
}; // namespace freertos
