#pragma once

#include <stdx/type_traits.hpp>

#if __has_include(<concepts>)
#include <concepts>
#endif

#if __cpp_lib_concepts < 202002L

#include <type_traits>
#include <utility>

namespace stdx {
inline namespace v1 {

#if __cplusplus < 202002L

// Before C++20 we can't use concepts directly, but we can still allow the
// names to be used in constexpr contexts

template <typename T> constexpr auto integral = std::is_integral_v<T>;
template <typename T>
constexpr auto floating_point = std::is_floating_point_v<T>;

template <typename T>
constexpr auto signed_integral = integral<T> and std::is_signed_v<T>;

template <typename T>
constexpr auto unsigned_integral = integral<T> and std::is_unsigned_v<T>;

template <typename From, typename To>
constexpr auto convertible_to = std::is_convertible_v<From, To>;

template <typename T, typename U>
constexpr auto derived_from =
    std::is_base_of_v<U, T> and
    std::is_convertible_v<T const volatile *, U const volatile *>;

template <typename T, typename U>
constexpr auto same_as = std::is_same_v<T, U> and std::is_same_v<U, T>;

// NOLINTBEGIN(bugprone-macro-parentheses, cppcoreguidelines-macro-usage)
#define DETECTOR(name, expr)                                                   \
    namespace detail::detect {                                                 \
    template <typename T, typename = void> constexpr auto name = false;        \
    template <typename T>                                                      \
    constexpr auto name<T, std::void_t<decltype(expr)>> = true;                \
    }
// NOLINTEND(bugprone-macro-parentheses, cppcoreguidelines-macro-usage)

DETECTOR(eq_compare, (std::declval<T>() == std::declval<T>()))
DETECTOR(neq_compare, (std::declval<T>() != std::declval<T>()))

template <typename T>
constexpr auto equality_comparable =
    detail::detect::eq_compare<T> and detail::detect::neq_compare<T>;

DETECTOR(lt_compare, (std::declval<T>() < std::declval<T>()))
DETECTOR(lte_compare, (std::declval<T>() <= std::declval<T>()))
DETECTOR(gt_compare, (std::declval<T>() > std::declval<T>()))
DETECTOR(gte_compare, (std::declval<T>() >= std::declval<T>()))

template <typename T>
constexpr auto totally_ordered =
    equality_comparable<T> and detail::detect::lt_compare<T> and
    detail::detect::lte_compare<T> and detail::detect::gt_compare<T> and
    detail::detect::gte_compare<T>;

#undef DETECTOR

namespace detail::detect {
template <typename... Args> struct arg_list {
    template <typename F>
    using invoke_result_t = std::invoke_result_t<F, Args...>;
};
template <typename F, typename Args, typename = void>
constexpr auto invocable = false;
template <typename F, typename Args>
constexpr auto invocable<
    F, Args, std::void_t<typename Args::template invoke_result_t<F>>> = true;
} // namespace detail::detect

template <typename F, typename... Args>
constexpr auto invocable =
    detail::detect::invocable<F, detail::detect::arg_list<Args...>>;

template <typename F, typename... Args>
constexpr auto predicate =
    invocable<F, Args...> and
    std::is_convertible_v<std::invoke_result_t<F, Args...>, bool>;

template <typename T> constexpr auto callable = is_callable_v<T>;

#else

// After C++20, we can define concepts that are lacking in the library

template <typename T>
concept integral = std::is_integral_v<T>;

template <typename T>
concept floating_point = std::is_floating_point_v<T>;

template <typename T>
concept signed_integral = integral<T> and std::is_signed_v<T>;

template <typename T>
concept unsigned_integral = integral<T> and std::is_unsigned_v<T>;

template <typename From, typename To>
concept convertible_to = std::is_convertible_v<From, To> and
                         requires { static_cast<To>(std::declval<From>()); };

template <typename T, typename U>
concept derived_from =
    std::is_base_of_v<U, T> and
    std::is_convertible_v<T const volatile *, U const volatile *>;

template <typename T, typename U>
concept same_as = std::is_same_v<T, U> and std::is_same_v<U, T>;

template <typename T>
concept equality_comparable = requires(T const &t) {
    { t == t } -> same_as<bool>;
    { t != t } -> same_as<bool>;
};

namespace detail {
template <typename T>
concept partially_ordered = requires(T const &t) {
    { t < t } -> same_as<bool>;
    { t <= t } -> same_as<bool>;
    { t > t } -> same_as<bool>;
    { t >= t } -> same_as<bool>;
};
}

template <typename T>
concept totally_ordered =
    equality_comparable<T> and detail::partially_ordered<T>;

template <typename F, typename... Args>
concept invocable =
    requires(F &&f, Args &&...args) {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    }

namespace detail {
    template <typename B>
    concept boolean_testable_impl = stdx::convertible_to<B, bool>;

    template <typename B>
    concept boolean_testable = boolean_testable_impl<B> and requires(B &&b) {
        { not std::forward<B>(b) } -> boolean_testable_impl;
    };
} // namespace detail

template <typename F, typename... Args>
concept predicate = invocable<F, Args> and
                    detail::boolean_testable<std::invoke_result_t<F, Args...>>;

template <typename T>
concept callable = is_callable_v<T>;

#endif

} // namespace v1
} // namespace stdx

#else

// C++20 concept library exists, so use that

namespace stdx {
inline namespace v1 {

using std::floating_point;
using std::integral;
using std::signed_integral;
using std::unsigned_integral;

using std::convertible_to;
using std::derived_from;
using std::same_as;

using std::equality_comparable;
using std::totally_ordered;

using std::invocable;
using std::predicate;

template <typename T>
concept callable = is_callable_v<T>;

} // namespace v1
} // namespace stdx

#endif
