/**
 * @file freertos_mutex.hpp
 * @brief we lack mutex support due undefined _GLIBCXX_HAS_GTHREADS, so there is mutex using FreeRTOS api
 *
 * The mutex class is a synchronization primitive that can be used to protect
 * shared data from being simultaneously accessed by multiple threads.
 *
 * mutex offers exclusive, non-recursive ownership semantics:
 *
 * A calling thread owns a mutex from the time that it successfully calls either lock or try_lock until it calls unlock.
 * When a thread owns a mutex, all other threads will block (for calls to lock) or receive a false return value (for try_lock)
 * if they attempt to claim ownership of the mutex.
 * A calling thread must not own the mutex prior to calling lock or try_lock.
 * The behavior of a program is undefined if a mutex is destroyed while still owned by any threads, or a thread terminates while owning a mutex.
 * The mutex class satisfies all requirements of Mutex and StandardLayoutType.
 *
 * mutex is neither copyable nor movable.
 */
#pragma once

#include <array>
#include <cstddef>
#include <freertos/config.hpp>

// As tempting as that may be, do not #include <mutex> here because it pulls in
// a bunch of std::crap which breaks XL debug build due to FLASH inflation.

namespace freertos {

class Mutex {
public:
    // We use erased storage in order to not pollute the scope with FreeRTOS internals.
    // The actual size and alignment are statically asserted in implementation file.
    using Storage = std::array<std::byte, mutex_storage_size>;

private:
    void *handle;
    alignas(mutex_storage_align) Storage mutex_storage;

public:
    Mutex();
    ~Mutex();
    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    /**
     * Releases the lock acquired by the current task.
     * The lock must have been acquired before.
     */
    void unlock();

    /**
     * Attempts to acquire the lock for the current task without blocking.
     * Return true if the lock was acquired, false otherwise.
     */
    bool try_lock();

    /**
     * Blocks until a lock is acquired for the current task.
     */
    void lock();
};

} // namespace freertos
