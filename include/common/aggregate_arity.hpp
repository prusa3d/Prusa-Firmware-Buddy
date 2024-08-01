#pragma once

#include <type_traits>
#include <concepts>

namespace detail {

// helper type that is convertible to anything
struct UniversalType {
    template <typename T>
    operator T() {}
};

// helper type that is convertible to anything that isn't a base class of Derived
template <typename Derived>
struct NoBaseUniversalType {
    template <typename NotBaseT>
    operator NotBaseT()
        requires(not std::is_base_of_v<NotBaseT, Derived>)
    {}
};

/**
 * @brief Counts the number of members via trying to build the aggregate with increasingly more arguments until it can't be built anymore and stopping there
 *
 * @tparam T Struct to be counted
 */
template <typename T>
consteval auto number_of_members(auto... members) {
    if constexpr (not requires { T { members... }; }) { // end recursion when can't build anymore
        return (0 + ... + std::same_as<NoBaseUniversalType<T>, decltype(members)>); // count how many non-base types are used in construction
    } else {
        if constexpr (requires { T { members..., NoBaseUniversalType<T> {} }; }) { // if can be built with previous args + one that cannot be a base class
            return number_of_members<T>(members..., NoBaseUniversalType<T> {}); // 'save' the arg as 'not a base class one'
        } else {
            return number_of_members<T>(members..., UniversalType {}); // 'save' the arg as 'anything' (including base class arg)
        }
    }
}
} // namespace detail

/**
 * @brief Returns the number of members in an aggregate (struct, without any base classes)
 *
 * @tparam T
 */
template <typename T>
consteval auto aggregate_arity() {
    return detail::number_of_members<T>();
}
