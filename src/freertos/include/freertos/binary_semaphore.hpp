#pragma once

#include <type_traits>
#include <array>
#include <cstdint>
#include <stddef.h>

namespace freertos {

// C++ wrapper for FreeRTOS binary semaphore.
class BinarySemaphore {
public:
    // We use erased storage in order to not pollute the scope with FreeRTOS internals.
    // The actual size and alignment are statically asserted in implementation file.
#ifdef UNITTESTS
    static constexpr size_t semaphore_storage_size = 168;
    static constexpr size_t semaphore_storage_align = 8;
#else
    static constexpr size_t semaphore_storage_size = 80;
    static constexpr size_t semaphore_storage_align = 4;
#endif
    using Storage = std::array<uint8_t, semaphore_storage_size>;

private:
    alignas(semaphore_storage_align) Storage semaphore_storage;

public:
    BinarySemaphore();
    ~BinarySemaphore();
    BinarySemaphore(const BinarySemaphore &) = delete;
    BinarySemaphore &operator=(const BinarySemaphore &) = delete;

    /// Increments the internal counter and unblocks one acquirer
    /// Raises a bsod if you try to call this when the semaphore is not acquired
    void release();

    /// Same as \p release, but instead of raising a bsod when the semaphore is not acquired,
    /// blocks the thread calling \p release, until someone tries to acquire the semaphore.
    void release_blocking();

    /// Decrements the internal counter or blocks until it can.
    void acquire();
};

} // namespace freertos
