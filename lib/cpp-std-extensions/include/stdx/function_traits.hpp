#pragma once

#include <boost/mp11/utility.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace stdx {
inline namespace v1 {
namespace detail {
template <typename...> struct function_traits;

template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
    using return_type = R;

    template <template <typename...> typename List> using args = List<Args...>;
    template <template <typename...> typename List>
    using decayed_args = List<std::decay_t<Args>...>;
    using arity = std::integral_constant<std::size_t, sizeof...(Args)>;
};
} // namespace detail

template <typename F>
using function_traits =
    detail::function_traits<decltype(std::function{std::declval<F>()})>;

template <typename F> using return_t = typename function_traits<F>::return_type;
template <typename F, template <typename...> typename List>
using args_t = typename function_traits<F>::template args<List>;
template <typename F, template <typename...> typename List>
using decayed_args_t = typename function_traits<F>::template decayed_args<List>;
template <typename F>
using nongeneric_arity_t = typename function_traits<F>::arity;

namespace detail {
template <auto> struct any_type {
    // NOLINTNEXTLINE(google-explicit-constructor)
    template <typename T> operator T();
};

template <typename F, std::size_t... Is>
constexpr auto try_invoke_impl(std::index_sequence<Is...>)
    -> std::invoke_result_t<F, any_type<Is>...>;

template <typename F, typename N>
using try_invoke =
    decltype(try_invoke_impl<F>(std::make_index_sequence<N::value>{}));

template <typename F, typename N>
using has_arg_count = boost::mp11::mp_valid<try_invoke, F, N>;

template <typename F, typename N> struct generic_arity;

template <typename F, typename N>
using generic_arity_t = typename generic_arity<F, N>::type;

template <typename F, typename N> struct generic_arity {
    using type = boost::mp11::mp_eval_if<
        has_arg_count<F, N>, N, generic_arity_t, F,
        std::integral_constant<std::size_t, N::value + 1u>>;
};
} // namespace detail

template <typename F>
using arity_t = boost::mp11::mp_eval_or<
    detail::generic_arity_t<F, std::integral_constant<std::size_t, 0u>>,
    nongeneric_arity_t, F>;
template <typename F> constexpr auto arity_v = arity_t<F>::value;
} // namespace v1
} // namespace stdx
