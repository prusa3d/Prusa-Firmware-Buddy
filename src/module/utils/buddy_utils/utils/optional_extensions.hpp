/// @file
#pragma once

#include <optional>

/// Converts std::expected to std::optional
/// Probably works on other types as well
/// @tparam Optional expected return type - defaults to std::optional<T>
template <typename Optional = void, typename Expected>
auto to_optional(Expected &&e) {
    using Result = std::conditional_t<
        !std::is_same_v<Optional, void>,
        Optional,
        std::optional<typename std::remove_cvref_t<Expected>::value_type> //
        >;

    if (e.has_value()) {
        return Result { *std::forward<Expected>(e) };
    }
    return Result { std::nullopt };
}
