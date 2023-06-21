#ifndef CTHASH_INTERNAL_ALGORITHM_HPP
#define CTHASH_INTERNAL_ALGORITHM_HPP

#include <numeric>
#include <compare>
#include <cstddef>
#include <cstdint>

namespace cthash::internal {

template <typename It1, typename It2>
constexpr auto threeway_compare_of_same_size(It1 lhs, It2 rhs, size_t length) -> std::strong_ordering {
    for (size_t i = 0; i != length; ++i) {
        if (const auto r = (*lhs++ <=> *rhs++); r != 0) {
            return r;
        }
    }

    return std::strong_ordering::equal;
}

template <typename T, typename It1, typename It2, typename Stream>
constexpr auto &push_to_stream_as(It1 f, It2 l, Stream &stream) {
    constexpr auto cast_and_shift = [](Stream *s, const auto &rhs) { (*s) << T{rhs}; return s; };
    return *std::accumulate(f, l, &stream, cast_and_shift);
}

} // namespace cthash::internal

#endif
