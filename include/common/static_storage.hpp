#pragma once

#include <cstddef>
#include <array>

/**
 * Convenience type for static untyped storage of a given size.
 *
 * Allows you to create, destroy and refer to some type created
 * at this storage.
 *
 * It is caller's responsibility to destroy constructed elements
 * and to track what type (if any) is currently in the storage.
 *
 * This is basically std::aligned_union_t which doesn't require
 * knowledge of the types upfront with some convenience functions.
 */
template <size_t Size>
class StaticStorage {
private:
    alignas(std::max_align_t) std::array<std::byte, Size> bytes;

public:
    /** Access the value of type T previously created at this storage */
    template <class T>
    constexpr T *as() {
        return static_cast<T *>(static_cast<void *>(bytes.data()));
    }

    /**
     * Call constructor of T. It must be preceded either by construction of this storage
     * or by call to destroy() (possibly of some other type)
     */
    template <class T, class... Args>
    constexpr T *create(Args &&...args) {
        static_assert(sizeof(T) <= Size);
        return std::construct_at(as<T>(), std::forward<Args>(args)...);
    }

    /**
     * Call destructor of T. It must have been previously created at this storage.
     */
    template <class T>
    constexpr void destroy() {
        return std::destroy_at(as<T>());
    }

    /**
     * Return true if the storage has just the right size to accomodate largest of given types.
     */
    template <class... T>
    static constexpr bool has_ideal_size_for() {
        return std::max({ sizeof(T)... }) == Size;
    }
};
