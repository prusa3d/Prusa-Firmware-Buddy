#ifndef CTHASH_INTERNAL_CONVERT_HPP
#define CTHASH_INTERNAL_CONVERT_HPP

#include "bit.hpp"
#include "concepts.hpp"
#include <span>
#include <type_traits>
#include <cstddef>

namespace cthash {

template <typename It1, typename It2, typename It3>
constexpr auto byte_copy(It1 first, It2 last, It3 destination) {
    return std::transform(first, last, destination, [](byte_like auto v) { return static_cast<std::byte>(v); });
}

template <std::unsigned_integral T, byte_like Byte>
constexpr auto cast_from_bytes(std::span<const Byte, sizeof(T)> in) noexcept -> T {
    if (std::is_constant_evaluated()) {
        return [&]<size_t... Idx>(std::index_sequence<Idx...>)->T {
            return static_cast<T>(((static_cast<T>(in[Idx]) << ((sizeof(T) - 1u - Idx) * 8u)) | ...));
        }
        (std::make_index_sequence<sizeof(T)>());
    } else {
        T t;
        std::memcpy(&t, in.data(), sizeof(T));
        if constexpr (std::endian::native == std::endian::little) {
            return internal::byteswap(t);
        } else {
            return t;
        }
    }
}

template <std::unsigned_integral T, byte_like Byte>
constexpr auto cast_from_le_bytes(std::span<const Byte, sizeof(T)> in) noexcept -> T {
    if (std::is_constant_evaluated()) {
        return [&]<size_t... Idx>(std::index_sequence<Idx...>)->T {
            return static_cast<T>(((static_cast<T>(in[Idx]) << static_cast<T>(Idx * 8u)) | ...));
        }
        (std::make_index_sequence<sizeof(T)>());
    } else {
        T t;
        std::memcpy(&t, in.data(), sizeof(T));
        if constexpr (std::endian::native == std::endian::big) {
            return internal::byteswap(t);
        } else {
            return t;
        }
    }
}

template <std::unsigned_integral T>
struct unwrap_littleendian_number {
    static constexpr size_t bytes = sizeof(T);
    static constexpr size_t bits = bytes * 8u;

    std::span<std::byte, bytes> ref;

    constexpr void operator=(T value) noexcept {
        [&]<size_t... Idx>(std::index_sequence<Idx...>) {
            ((ref[Idx] = static_cast<std::byte>(value >> (Idx * 8u))), ...);
        }
        (std::make_index_sequence<bytes>());
    }
};

unwrap_littleendian_number(std::span<std::byte, 8>)->unwrap_littleendian_number<uint64_t>;
unwrap_littleendian_number(std::span<std::byte, 4>)->unwrap_littleendian_number<uint32_t>;

template <std::unsigned_integral T>
struct unwrap_bigendian_number {
    static constexpr size_t bytes = sizeof(T);
    static constexpr size_t bits = bytes * 8u;

    std::span<std::byte, bytes> ref;

    constexpr void operator=(T value) noexcept {
        [&]<size_t... Idx>(std::index_sequence<Idx...>) {
            ((ref[Idx] = static_cast<std::byte>(value >> ((bits - 8u) - 8u * Idx))), ...);
        }
        (std::make_index_sequence<bytes>());
    }
};

unwrap_bigendian_number(std::span<std::byte, 8>)->unwrap_bigendian_number<uint64_t>;
unwrap_bigendian_number(std::span<std::byte, 4>)->unwrap_bigendian_number<uint32_t>;

} // namespace cthash

#endif
