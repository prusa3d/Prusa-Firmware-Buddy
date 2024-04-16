#pragma once

#include <common/concepts.hpp>
#include <common/rw_mutex.h>

namespace freertos {

/**
 * Implements Lockable and SharedLockable concept using internal RWMutex_t
 */
class SharedMutex {
private:
    RWMutex_t rw_mutex;

public:
    SharedMutex(uint8_t max_readers) { rw_mutex_init(&rw_mutex, max_readers); }
    void lock_shared() { rw_mutex_reader_take(&rw_mutex); }
    [[nodiscard]] bool try_lock_shared() { return rw_mutex_reader_try_take(&rw_mutex); }
    void unlock_shared() { rw_mutex_reader_give(&rw_mutex); }

    void lock() { rw_mutex_writer_take(&rw_mutex); }
    [[nodiscard]] bool try_lock() { return rw_mutex_writer_try_take(&rw_mutex); }
    void unlock() { rw_mutex_writer_give(&rw_mutex); }
};
static_assert(concepts::Lockable<SharedMutex> && concepts::SharedLockable<SharedMutex>);

/**
 * Implements Lockable and SharedLockable concept using a pointer to already existing RWMutex_t
 */
class SharedMutexProxy {
private:
    RWMutex_t *rw_mutex;

public:
    SharedMutexProxy() { abort(); };
    SharedMutexProxy(RWMutex_t *rx_mutex)
        : rw_mutex(rx_mutex) {}

    void lock_shared() { rw_mutex_reader_take(rw_mutex); }
    [[nodiscard]] bool try_lock_shared() { return rw_mutex_reader_try_take(rw_mutex); }
    void unlock_shared() { rw_mutex_reader_give(rw_mutex); }

    void lock() { rw_mutex_writer_take(rw_mutex); }
    [[nodiscard]] bool try_lock() { return rw_mutex_writer_try_take(rw_mutex); }
    void unlock() { rw_mutex_writer_give(rw_mutex); }
};
static_assert(concepts::Lockable<SharedMutexProxy> && concepts::SharedLockable<SharedMutexProxy>);

} // namespace freertos
