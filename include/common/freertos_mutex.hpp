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

#include <mutex>
#include <FreeRTOS.h>
#include "semphr.h"

class FreeRTOS_Mutex {
public:
    void unlock();
    bool try_lock();
    void lock();
    FreeRTOS_Mutex() noexcept;
    FreeRTOS_Mutex(const FreeRTOS_Mutex &) = delete;
    ~FreeRTOS_Mutex();

private:
    SemaphoreHandle_t xSemaphore = nullptr;
    StaticSemaphore_t xSemaphoreData;
};

// we need our own lock, because GCC uses try and back off which internally uses try_lock and because FreeRTOS does not use priority inheritance when trying to lock mutex without blocking
// this results in deadlock (inverse priority problem), because thread with lower priority does not get CPU time while still holding mutex.
namespace buddy {

// not template, because someone could try to lock mutex once in wrapper and once not in wrapper => order could be inconsistent
void lock(std::unique_lock<FreeRTOS_Mutex> &l1, std::unique_lock<FreeRTOS_Mutex> &l2);

} // namespace buddy
