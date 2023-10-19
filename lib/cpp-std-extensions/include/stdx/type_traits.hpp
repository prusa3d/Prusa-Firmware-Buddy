#pragma once

#include <type_traits>
#include <utility>

namespace stdx {
inline namespace v1 {

template <typename E>
constexpr auto to_underlying(E e) noexcept -> std::underlying_type_t<E> {
    return static_cast<std::underlying_type_t<E>>(e);
}

template <typename T> struct remove_cvref {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};
template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

namespace detail {
template <bool> struct conditional;

template <> struct conditional<true> {
    template <typename T, typename> using choice_t = T;
};
template <> struct conditional<false> {
    template <typename, typename U> using choice_t = U;
};
} // namespace detail

template <bool B, typename T, typename U>
using conditional_t = typename detail::conditional<B>::template choice_t<T, U>;

template <typename...> constexpr bool always_false_v = false;

template <typename T>
constexpr bool is_function_v =
    not std::is_reference_v<T> and not std::is_const_v<std::add_const_t<T>>;

namespace detail {
struct call_base {
    auto operator()() -> void;
};

template <typename, bool> struct callable_test : call_base {};
template <typename F> struct callable_test<F, true> : F, call_base {};

template <typename F, typename = void> constexpr auto is_func_obj = true;
template <typename F>
constexpr auto is_func_obj<
    F,
    std::void_t<decltype(&callable_test<F, std::is_class_v<F>>::operator())>> =
    false;
} // namespace detail
template <typename T>
constexpr bool is_function_object_v = detail::is_func_obj<T>;

template <typename T>
constexpr bool is_callable_v = is_function_v<T> or is_function_object_v<T>;

constexpr auto is_constant_evaluated() noexcept -> bool {
    return __builtin_is_constant_evaluated();
}

namespace detail {
template <template <typename...> typename T> struct detect_specialization {
    template <typename U> constexpr auto operator()(U &&) -> std::false_type;

    template <typename... Us>
    constexpr auto operator()(T<Us...> &&) -> std::true_type;
};
} // namespace detail

template <typename U, template <typename...> typename T>
constexpr bool is_specialization_of_v =
    decltype(std::declval<detail::detect_specialization<T>>()(
        std::declval<U>()))::value;

} // namespace v1
} // namespace stdx
