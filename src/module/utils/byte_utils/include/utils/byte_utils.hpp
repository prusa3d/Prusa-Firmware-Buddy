/// @file
#pragma once

#include <cstddef>
#include <span>
#include <type_traits>

using WritableBytes = std::span<std::byte>;
using Bytes = std::span<const std::byte>;

/// Return byte-representation of trivially-copyable object.
template <typename T>
    requires std::is_trivially_copyable_v<T>
inline Bytes trivial_as_bytes(const T &obj) noexcept {
    return std::as_bytes(std::span<const T, 1> { &obj, 1 });
}

/// Return writable byte-representation of trivially-copyable object.
template <typename T>
    requires std::is_trivially_copyable_v<T>
inline WritableBytes trivial_as_writable_bytes(T &obj) noexcept {
    return std::as_writable_bytes(std::span<T, 1> { &obj, 1 });
}
