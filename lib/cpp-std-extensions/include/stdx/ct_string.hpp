#pragma once

#if __cplusplus >= 202002L

#include <stdx/compiler.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <utility>

namespace stdx {
inline namespace v1 {
template <std::size_t N> struct ct_string {
    // NOLINTNEXTLINE(*-avoid-c-arrays, google-explicit-constructor)
    CONSTEVAL explicit(false) ct_string(char const (&str)[N]) {
        for (auto i = std::size_t{}; i < N; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            value[i] = str[i];
        }
    }

    CONSTEVAL explicit(true) ct_string(char const *str, std::size_t sz) {
        for (auto i = std::size_t{}; i < sz; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-*)
            value[i] = str[i];
        }
    }

    constexpr static std::integral_constant<std::size_t, N - 1U> size{};
    constexpr static std::integral_constant<bool, N == 1U> empty{};

    constexpr explicit(true) operator std::string_view() const {
        return std::string_view{std::cbegin(value), size()};
    }

    std::array<char, N> value{};
};

template <std::size_t N, std::size_t M>
constexpr auto operator==(ct_string<N> const &lhs, ct_string<M> const &rhs)
    -> bool {
    return static_cast<std::string_view>(lhs) ==
           static_cast<std::string_view>(rhs);
}

template <template <typename, char...> typename T, char... Cs>
CONSTEVAL auto ct_string_from_type(T<char, Cs...>) {
    return ct_string<sizeof...(Cs) + 1U>{{Cs..., 0}};
}

template <ct_string S, template <typename, char...> typename T>
CONSTEVAL auto ct_string_to_type() {
    return [&]<auto... Is>(std::index_sequence<Is...>) {
        return T<char, std::get<Is>(S.value)...>{};
    }(std::make_index_sequence<S.size()>{});
}

template <ct_string S, char C> [[nodiscard]] consteval auto split() {
    constexpr auto it = [] {
        for (auto i = std::cbegin(S.value); i != std::cend(S.value); ++i) {
            if (*i == C) {
                return i;
            }
        }
        return std::cend(S.value);
    }();
    if constexpr (it == std::cend(S.value)) {
        return std::pair{S, ct_string{""}};
    } else {
        constexpr auto prefix_size =
            static_cast<std::size_t>(std::distance(std::cbegin(S.value), it));
        constexpr auto suffix_size = S.size() - prefix_size;
        return std::pair{
            ct_string<prefix_size + 1U>{std::cbegin(S.value), prefix_size},
            ct_string<suffix_size>{std::next(it), suffix_size - 1U}};
    }
}

namespace ct_string_literals {
template <typename T, T... Cs> CONSTEVAL auto operator""_cts() {
    return ct_string<sizeof...(Cs) + 1U>{{Cs..., 0}};
}
} // namespace ct_string_literals
} // namespace v1
} // namespace stdx

#endif
