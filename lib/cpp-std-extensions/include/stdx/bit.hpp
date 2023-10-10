#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

#if __has_include(<bit>)
#include <bit>
#endif

namespace stdx {
inline namespace v1 {

// endian

#if __cpp_lib_endian < 201907L
enum struct endian {
    little = __ORDER_LITTLE_ENDIAN__,
    big = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
};
#else
using endian = std::endian;
#endif

// bit_cast

#if __cpp_lib_bit_cast < 201806L
template <typename To, typename From>
[[nodiscard]] constexpr auto bit_cast(From &from) noexcept -> To {
    return __builtin_bit_cast(To, from);
}
#else
using std::bit_cast;
#endif

// byteswap

#if __cpp_lib_byteswap < 202110L
template <typename T>
[[nodiscard]] constexpr auto byteswap(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, T> {
    if constexpr (sizeof(T) == sizeof(std::uint16_t)) {
        return __builtin_bswap16(x);
    } else if constexpr (sizeof(T) == sizeof(std::uint32_t)) {
        return __builtin_bswap32(x);
    } else if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
        return __builtin_bswap64(x);
    } else {
        return x;
    }
}
#else
using std::byteswap;
#endif

// bit ops

#if __cpp_lib_bitops < 201907L
template <typename T>
[[nodiscard]] constexpr auto countl_zero(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, int> {
    if (x == 0) {
        return std::numeric_limits<T>::digits;
    }
    if constexpr (sizeof(T) == sizeof(unsigned int)) {
        return __builtin_clz(x);
    } else if constexpr (sizeof(T) ==
                         sizeof(unsigned long)) { // NOLINT(google-runtime-int)
        return __builtin_clzl(x);
    } else if constexpr (
        sizeof(T) == sizeof(unsigned long long)) { // NOLINT(google-runtime-int)
        return __builtin_clzll(x);
    } else {
        return __builtin_clzll(x) + std::numeric_limits<T>::digits -
               std::numeric_limits<
                   unsigned long long>::digits; // NOLINT(google-runtime-int)
    }
}

template <typename T>
[[nodiscard]] constexpr auto countr_zero(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, int> {
    if (x == 0) {
        return std::numeric_limits<T>::digits;
    }
    if constexpr (sizeof(T) == sizeof(unsigned int)) {
        return __builtin_ctz(x);
    } else if constexpr (sizeof(T) ==
                         sizeof(unsigned long)) { // NOLINT(google-runtime-int)
        return __builtin_ctzl(x);
    } else {
        return __builtin_ctzll(x);
    }
}

template <typename T>
[[nodiscard]] constexpr auto countl_one(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, int> {
    return countl_zero(T(~x));
}

template <typename T>
[[nodiscard]] constexpr auto countr_one(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, int> {
    return countr_zero(T(~x));
}

template <typename T>
[[nodiscard]] constexpr auto popcount(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, int> {
    if constexpr (sizeof(T) <= sizeof(unsigned int)) {
        return __builtin_popcount(x);
    } else if constexpr (sizeof(T) <=
                         sizeof(unsigned long)) { // NOLINT(google-runtime-int)
        return __builtin_popcountl(x);
    } else {
        return __builtin_popcountll(x);
    }
}

namespace detail {
template <typename T>
[[nodiscard]] constexpr auto rotl(T x, T s) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, T> {
#ifdef __clang__
    if constexpr (sizeof(T) == sizeof(std::uint8_t)) {
        return __builtin_rotateleft8(x, s);
    } else if constexpr (sizeof(T) == sizeof(std::uint16_t)) {
        return __builtin_rotateleft16(x, s);
    } else if constexpr (sizeof(T) == sizeof(std::uint32_t)) {
        return __builtin_rotateleft32(x, s);
    } else {
        return __builtin_rotateleft64(x, s);
    }
#else
    return (x << s) | (x >> (std::numeric_limits<T>::digits - s));
#endif
}
template <typename T>
[[nodiscard]] constexpr auto rotr(T x, T s) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, T> {
#ifdef __clang__
    if constexpr (sizeof(T) == sizeof(std::uint8_t)) {
        return __builtin_rotateright8(x, s);
    } else if constexpr (sizeof(T) == sizeof(std::uint16_t)) {
        return __builtin_rotateright16(x, s);
    } else if constexpr (sizeof(T) == sizeof(std::uint32_t)) {
        return __builtin_rotateright32(x, s);
    } else {
        return __builtin_rotateright64(x, s);
    }
#else
    return (x >> s) | (x << (std::numeric_limits<T>::digits - s));
#endif
}
} // namespace detail

template <typename T>
[[nodiscard]] constexpr auto rotl(T x, int s) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, T> {
    if (s == 0) {
        return x;
    }
    if (s < 0) {
        return detail::rotr(x, T(-s));
    }
    return detail::rotl(x, T(s));
}

template <typename T>
[[nodiscard]] constexpr auto rotr(T x, int s) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, T> {
    if (s == 0) {
        return x;
    }
    if (s < 0) {
        return detail::rotl(x, T(-s));
    }
    return detail::rotr(x, T(s));
}
#else
using std::countl_one;
using std::countl_zero;
using std::countr_one;
using std::countr_zero;
using std::popcount;
using std::rotl;
using std::rotr;
#endif

// pow2

#if __cpp_lib_int_pow2 < 202002L
template <typename T>
[[nodiscard]] constexpr auto has_single_bit(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, bool> {
    return x and not(x & (x - 1));
}

template <typename T>
[[nodiscard]] constexpr auto bit_width(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, int> {
    return std::numeric_limits<T>::digits - countl_zero(x);
}

template <typename T>
[[nodiscard]] constexpr auto bit_ceil(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, T> {
    if (x <= 1U) {
        return 1U;
    }
    return T(1U << bit_width(x));
}

template <typename T>
[[nodiscard]] constexpr auto bit_floor(T x) noexcept
    -> std::enable_if_t<std::is_unsigned_v<T>, T> {
    if (x == 0) {
        return x;
    }
    return T(1U << (bit_width(x) - 1));
}
#else
using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
using std::has_single_bit;
#endif

} // namespace v1
} // namespace stdx
