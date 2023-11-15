#pragma once

#include <bit>
#include <cstdint>

/// Compact pointer using just 2 bytes instead of 4 bytes.
/// Assuming 4-byte aligned addresses, we're able to fit the entire span of the 256 KB RAM addresses into 16 bits if we strip the lowest 2 bits.
/// We only support RAM & CCRAM addresses
template <typename T>
class CompactRAMPointer final {

#ifndef UNITTESTS
public:
    /// I failed to find a macro for RAM size, if anyone knows about it, please replace this
    /// On Prusa Mini, this is less, but that's fine, we just end up covering a bit of addresses outside
    /// It would be a problem if we got a MCU with larger RAM than this
    static constexpr uintptr_t ram_addresses_span = 192 * 1024;
    static constexpr uintptr_t ram_addresses_begin = SRAM1_BASE;
    static constexpr uintptr_t ram_addresses_end = ram_addresses_begin + ram_addresses_span;

    static constexpr uintptr_t ccram_addresses_span = CCMDATARAM_END - 1 - CCMDATARAM_BASE;
    static constexpr uintptr_t ccram_addresses_begin = CCMDATARAM_BASE;
    static constexpr uintptr_t ccram_addresses_end = ccram_addresses_begin + ccram_addresses_span;

    static_assert(((ram_addresses_span + ccram_addresses_span) >> 2) - 1 <= std::numeric_limits<uint16_t>::max());
#endif

public:
    inline CompactRAMPointer() = default;
    inline CompactRAMPointer(const CompactRAMPointer &) = default;
    CompactRAMPointer(T *ptr);

public:
    inline CompactRAMPointer &operator=(const CompactRAMPointer &) = default;

    inline CompactRAMPointer &operator=(T *ptr) {
        *this = CompactRAMPointer(ptr);
        return *this;
    }

public:
    T *ptr() const;

public:
    // Cast to original pointer type
    inline operator T *() const {
        return ptr();
    }

public:
    // Dereference operator
    inline T &operator*() const {
        return *ptr();
    }

    // Pointer access operator
    inline T *operator->() const {
        return ptr();
    }

public:
    inline bool operator==(const CompactRAMPointer &) const = default;
    inline bool operator!=(const CompactRAMPointer &) const = default;

    inline bool operator==(std::remove_const_t<T> *o) const {
        return ptr() == o;
    }
    inline bool operator!=(std::remove_const_t<T> *o) const {
        return ptr() != o;
    }

    inline bool operator==(const T *o) const {
        return ptr() == o;
    }
    inline bool operator!=(const T *o) const {
        return ptr() != o;
    }

protected:
#ifndef UNITTESTS
    uint16_t data = 0;
#else
    /// Unittests do not run on the MCU - the RAM is completely different, use standard pointer
    T *data = nullptr;
#endif
};

#ifndef UNITTESTS

template <typename T>
CompactRAMPointer<T>::CompactRAMPointer(T *ptr) {
    static_assert(sizeof(CompactRAMPointer) == 2);

    const auto ptr_val = reinterpret_cast<uintptr_t>(ptr);
    assert(ptr_val % 4 == 0); // Check that the pointer is aligned to 4 bytes

    if (ptr_val == 0) {
        data = 0;
    }

    // We cannot consider the exact supported_addresses_begin address, because it would be stored the same way as nullptr
    else if (ptr_val > ram_addresses_begin && ptr_val < ram_addresses_end) {
        data = ptr_val >> 2;
    }

    else if (ptr_val >= ccram_addresses_begin && ptr_val < ccram_addresses_end) {
        static_assert(ram_addresses_span % 4 == 0);
        data = (ptr_val >> 2) | (ram_addresses_span >> 2);
    }

    else {
        assert(0);
    }
}

template <typename T>
inline T *CompactRAMPointer<T>::ptr() const {
    const auto sdata = (static_cast<uintptr_t>(data) << 2);

    if (data == 0) {
        return nullptr;

    } else if (sdata < ram_addresses_span) {
        return reinterpret_cast<T *>(sdata | ram_addresses_begin);

    } else {
        return reinterpret_cast<T *>(sdata + ccram_addresses_begin - ram_addresses_span);
    }
}

#else

// UNITTEST VERSION
template <typename T>
CompactRAMPointer<T>::CompactRAMPointer(T *ptr)
    : data(ptr) {
}

// UNITTEST VERSION
template <typename T>
inline T *CompactRAMPointer<T>::ptr() const {
    return data;
}

#endif

template <typename T>
inline bool operator==(const T *ptr, const CompactRAMPointer<T> &cp) {
    return ptr == cp.ptr();
}

template <typename T>
inline bool operator!=(const T *ptr, const CompactRAMPointer<T> &cp) {
    return ptr != cp.ptr();
}
