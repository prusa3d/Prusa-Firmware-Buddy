#pragma once

#include <type_traits>

namespace freertos {

// C++ wrapper for FreeRTOS binary semaphore.
class BinarySemaphore {
public:
    // We use erased storage in order to not pollute the scope with FreeRTOS internals.
    // The actual size and alignment are statically asserted in implementation file.
#ifdef UNITTESTS
    using Storage = std::aligned_storage_t<168, 8>;
#else
    using Storage = std::aligned_storage_t<80, 4>;
#endif

private:
    Storage semaphore_storage;

public:
    BinarySemaphore();
    ~BinarySemaphore();
    BinarySemaphore(const BinarySemaphore &) = delete;
    BinarySemaphore &operator=(const BinarySemaphore &) = delete;

    /// Increments the internal counter and unblocks acquirers.
    void release();

    /// Decrements the internal counter or blocks until it can.
    void acquire();
};

} // namespace freertos
