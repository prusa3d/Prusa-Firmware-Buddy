#pragma once

#include <variant>

/**
 * @brief Used to overload std::visit with multiple lambdas, usage: std::visit(Overloaded { lambda1, lambda2, ...}, variant);
 *
 * @tparam Ts
 */
template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>; // CTAD

/// Usage:
/// match(variant,
///     [](Type1 t1){ do_something(t1) },
///     [](Type2 t2){ do_something(t2) },
///     ...
/// );
template <typename Variant, typename... Matchers>
constexpr decltype(auto) match(Variant &&v, Matchers &&...matchers) {
    return std::visit(
        Overloaded { std::forward<Matchers>(matchers)... },
        std::forward<Variant>(v));
}
