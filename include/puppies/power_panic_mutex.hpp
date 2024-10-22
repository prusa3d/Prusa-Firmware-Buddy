#pragma once

#include <array>
#include <cstddef>

// As tempting as that may be, do not #include <mutex> here because it pulls in
// a bunch of std::crap which breaks XL debug build due to FLASH inflation.

namespace buddy::puppies {

/// A custom mutex that doesn't lock during power panic.
///
/// This is a mostly copy of freertos::Mutex, but it is resilient against use
/// during power panic. Mutexes are mutilated during power panic and don't
/// work, which makes the freertos wrapper abort. We instead run with unlocked
/// mutex instead, which is technically wrong, but better situation than we
/// were before.
class PowerPanicMutex {
public:
// We use erased storage in order to not pollute the scope with FreeRTOS internals.
// The actual size and alignment are statically asserted in implementation file.
#ifdef UNITTESTS
    static constexpr size_t storage_size = 168;
    static constexpr size_t storage_align = 8;
#else
    static constexpr size_t storage_size = 80;
    static constexpr size_t storage_align = 4;
#endif
    // We use erased storage in order to not pollute the scope with FreeRTOS internals.
    // The actual size and alignment are statically asserted in implementation file.
    using Storage = std::array<std::byte, storage_size>;

private:
    alignas(storage_align) Storage mutex_storage;
    bool locked = false;

public:
    PowerPanicMutex();
    ~PowerPanicMutex();
    PowerPanicMutex(const PowerPanicMutex &) = delete;
    PowerPanicMutex &operator=(const PowerPanicMutex &) = delete;

    /**
     * Releases the lock acquired by the current task.
     * The lock must have been acquired before.
     */
    void unlock();

    /**
     * Blocks until a lock is acquired for the current task.
     */
    void lock();
};

} // namespace buddy::puppies
