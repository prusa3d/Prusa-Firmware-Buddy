#ifndef CTHASH_INTERNAL_BIT_HPP
#define CTHASH_INTERNAL_BIT_HPP

#include <bit>

namespace cthash::internal {

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L

template <std::unsigned_integral T>
[[gnu::always_inline]] constexpr auto byteswap(T val) noexcept {
    return std::byteswap(val);
}

#else

template <typename T, size_t N>
concept unsigned_integral_of_size = (sizeof(T) == N) && std::unsigned_integral<T>;

template <unsigned_integral_of_size<1> T>
constexpr auto byteswap(T val) noexcept {
    return val;
}

template <unsigned_integral_of_size<2> T>
constexpr auto byteswap(T val) noexcept {
    return static_cast<T>(__builtin_bswap16(val));
}

template <unsigned_integral_of_size<4> T>
constexpr auto byteswap(T val) noexcept {
    return static_cast<T>(__builtin_bswap32(val));
}

template <unsigned_integral_of_size<8> T>
constexpr auto byteswap(T val) noexcept {
    return static_cast<T>(__builtin_bswap64(val));
}

#endif

} // namespace cthash::internal

#endif
