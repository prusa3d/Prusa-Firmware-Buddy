/// @file
#pragma once

#include <utility>
#include <cstddef>

/// Useful for fold expressions
/// @example FoldHelper{ 0, std::max<size_t> } + ... + index_sequence).result
template <typename T, typename F>
struct FoldHelper {
    constexpr auto operator+(auto o) const {
        const auto new_result = f(result, o);
        return FoldHelper<decltype(new_result), F> { new_result, f };
    }

    T result;
    F f;
};

/// Similar to std::accumulate, but the items for accumulation are not taken from a range, but passed as arguments
/// The arguments can be of different types, as long as @p op accepts them
/// @p op accumulating operation can also return different type each time
/// @example accumulate_arguments(std::plus{}, 0, 1, 2, 3, 4, 5) == 15
template <typename BinaryOperation, typename Arg1, typename... Args>
constexpr auto accumulate_arguments(BinaryOperation op, Arg1 &&arg1, Args &&...args) {
    return (FoldHelper { .result = std::forward<Arg1>(arg1), .f = op } + ... + std::forward<Args>(args)).result;
}

/// @returns value of the nth argument
template <size_t n, typename Arg0, typename... Args>
[[gnu::always_inline]] constexpr auto nth_argument(Arg0 &&arg0, Args &&...args) {
    if constexpr (n == 0) {
        return std::forward<Arg0>(arg0);
    } else {
        return nth_argument<n - 1>(std::forward<Args>(args)...);
    }
}

/// A class that is implicitly castable to any other class using the class default constructor
struct AnyDefaultConstructor {
    template <typename T>
    constexpr operator T() const {
        return T {};
    }
};
