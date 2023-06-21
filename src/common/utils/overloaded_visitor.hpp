#pragma once

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
