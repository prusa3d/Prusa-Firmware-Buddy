/// @file
#pragma once

#include <optional>
#include <variant>
#include <utility>
#include <type_traits>
#include <utils/overloaded_visitor.hpp>

namespace stdext {

/// Helper struct for automatic target type deduction
template <typename SourceType>
struct ToVariantResult {
    SourceType source;

    template <typename TargetType>
    operator TargetType() {
        // If the input is a variant (with T being a subset of Result), return an "expanded" variant
        static constexpr auto f1 = []<typename... T>(const std::variant<T...> &val) {
            return std::visit([](auto &&val) { return TargetType { val }; }, val);
        };

        // Otherwise, the input is assumed to be one of the types of the output variant
        static constexpr auto f2 = []<typename T>(T && val)
            requires requires(T &&val) { TargetType { val }; }
        {
            return TargetType { val };
        };

        return Overloaded { f1, f2 }(std::forward<SourceType>(source));
    }
};

/// Helper funtion for converting variants to superset variants
/// Usage:
/// convert basic type to variant
/// std::variant<TypeA, TypeB> small_variant = to_variant(type_a);
///
/// convert smaller variant to bigger variant
/// std::variant<TypeA, TypeB, TypeC> big_variant = to_variant(small_variant);
auto to_variant(auto &&input) {
    return ToVariantResult<decltype(input)> { std::forward<decltype(input)>(input) };
}

template <typename T, typename... V>
bool holds_value(const std::variant<V...> &variant, const T &value) {
    return std::holds_alternative<T>(variant) && std::get<T>(variant) == value;
}

/// @returns Optionally returns a specified item from std::variant, if the variant holds the item.
template <typename T, typename... V>
std::optional<T> get_optional(const std::variant<V...> &v) {
    const T *r = std::get_if<T>(&v);
    if (r != nullptr) {
        return *r;
    } else {
        return std::nullopt;
    }
}

template <typename T, typename V>
constexpr bool is_in_variant = []<typename... VT>(std::type_identity<std::variant<VT...>>) {
    return (std::is_same_v<T, VT> || ...);
}(std::type_identity<V> {});

} // namespace stdext
