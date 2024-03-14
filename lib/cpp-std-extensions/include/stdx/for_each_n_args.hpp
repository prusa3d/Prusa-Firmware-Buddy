#pragma once

#include <stdx/function_traits.hpp>

#if __cplusplus >= 202002L
#include <stdx/tuple.hpp>
#define TUPLE_T stdx::tuple
#define TUPLE_GET stdx::get
#else
#include <tuple>
#define TUPLE_T std::tuple
#define TUPLE_GET std::get
#endif

#include <cstddef>
#include <functional>
#include <utility>

namespace stdx {
inline namespace v1 {

namespace detail {
template <typename, typename> struct for_each_n_args;

template <std::size_t... Rows, std::size_t... Columns>
struct for_each_n_args<std::index_sequence<Rows...>,
                       std::index_sequence<Columns...>> {
    template <typename F, typename T> static auto apply(F &&f, T &&t) -> void {
        (exec<Rows * sizeof...(Columns)>(f, std::forward<T>(t)), ...);
    }

  private:
    template <std::size_t RowIndex, typename F, typename T>
    static auto exec(F &&f, T &&t) -> void {
        std::invoke(f, TUPLE_GET<RowIndex + Columns>(std::forward<T>(t))...);
    }
};
} // namespace detail

template <std::size_t N = 0, typename F, typename... Args>
void for_each_n_args(F &&f, Args &&...args) {
    constexpr auto batch_size = [] {
        if constexpr (N == 0) {
            return arity_t<F>::value;
        } else {
            return N;
        }
    }();
    static_assert(sizeof...(Args) % batch_size == 0,
                  "for_each_n_args: number of args must be a multiple of the "
                  "given N (or function arity)");

    using tuple_t = TUPLE_T<Args &&...>;
    detail::for_each_n_args<
        std::make_index_sequence<sizeof...(Args) / batch_size>,
        std::make_index_sequence<batch_size>>::apply(std::forward<F>(f),
                                                     tuple_t{std::forward<Args>(
                                                         args)...});
}
} // namespace v1
} // namespace stdx

#undef TUPLE_T
#undef TUPLE_GET
