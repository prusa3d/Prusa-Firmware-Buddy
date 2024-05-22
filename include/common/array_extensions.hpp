#pragma once
#include <array>

/**
 * @brief Used to discern whether template parameter is std::array<..., ...> or not
 */
template <typename T>
struct is_std_array : std::false_type {};
template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template <typename T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;
