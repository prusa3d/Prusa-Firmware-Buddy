#pragma once

#include <cstddef>

namespace stdx {
inline namespace v1 {
template <std::size_t N> struct priority_t : priority_t<N - 1> {};
template <> struct priority_t<0> {};

template <std::size_t N> constexpr inline auto priority = priority_t<N>{};
} // namespace v1
} // namespace stdx
