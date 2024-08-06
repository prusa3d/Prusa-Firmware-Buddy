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

namespace stdext {

/// Maps each item \p x in \p array to \p f(x) in the result array
/// \returns array with of the mapped values
template <typename T, size_t size, typename F>
constexpr auto map_array(const std::array<T, size> &array, F &&f) {
    std::array<std::remove_cvref_t<decltype(f(array[0]))>, size> result;
    for (size_t i = 0; i < size; i++) {
        result[i] = f(array[i]);
    }
    return result;
}

} // namespace stdext
