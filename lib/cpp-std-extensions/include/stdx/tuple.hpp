#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace stdx {
inline namespace v1 {

template <std::size_t> struct index_constant;
template <std::size_t I> constexpr static index_constant<I> *index{};
template <typename> struct tag_constant;
template <typename T> constexpr static tag_constant<T> *tag{};

namespace literals {
template <char... Chars>
    requires(... and (Chars >= '0' and Chars <= '9'))
constexpr auto operator""_idx() {
    constexpr auto n = [] {
        auto x = std::size_t{};
        ((x *= 10, x += Chars - '0'), ...);
        return x;
    }();
    return index<n>;
}
} // namespace literals

namespace detail {
template <std::size_t, typename...> struct element;

template <typename T>
concept derivable = std::is_class_v<T>;
template <typename T>
concept nonderivable = not std::is_class_v<T>;

template <std::size_t Index, nonderivable T, typename... Ts>
struct element<Index, T, Ts...> {
    [[nodiscard]] constexpr auto
    ugly_iGet_clvr(index_constant<Index> *) const & noexcept -> T const & {
        return value;
    }
    [[nodiscard]] constexpr auto
    ugly_iGet_lvr(index_constant<Index> *) & noexcept -> T & {
        return value;
    }
    [[nodiscard]] constexpr auto
    ugly_iGet_rvr(index_constant<Index> *) && noexcept -> T && {
        return std::forward<T>(value);
    }

    template <typename U>
        requires(std::same_as<U, T> or ... or std::same_as<U, Ts>)
    [[nodiscard]] constexpr auto
    ugly_tGet_clvr(tag_constant<U> *) const & noexcept -> T const & {
        return value;
    }
    template <typename U>
        requires(std::same_as<U, T> or ... or std::same_as<U, Ts>)
    [[nodiscard]] constexpr auto ugly_tGet_lvr(tag_constant<U> *) & noexcept
        -> T & {
        return value;
    }
    template <typename U>
        requires(std::same_as<U, T> or ... or std::same_as<U, Ts>)
    [[nodiscard]] constexpr auto ugly_tGet_rvr(tag_constant<U> *) && noexcept
        -> T && {
        return std::forward<T>(value);
    }

    constexpr static auto ugly_Value(index_constant<Index> *) -> T;
    constexpr auto ugly_Value_clvr() const & -> T const & { return value; }
    constexpr auto ugly_Value_lvr() & -> T & { return value; }
    constexpr auto ugly_Value_rvr() && -> T && {
        return std::forward<T>(value);
    }

    T value;

  private:
    [[nodiscard]] friend constexpr auto operator==(element x, element y) -> bool
        requires(std::is_reference_v<T>)
    {
        return std::addressof(x.value) == std::addressof(y.value);
    }
    [[nodiscard]] friend constexpr auto operator==(element const &,
                                                   element const &) -> bool
        requires(not std::is_reference_v<T>)
    = default;
    [[nodiscard]] friend constexpr auto operator<=>(element const &,
                                                    element const &) = default;
};

template <std::size_t Index, derivable T, typename... Ts>
struct element<Index, T, Ts...> : T {
    constexpr static auto ugly_Index = Index;

    [[nodiscard]] constexpr auto
    ugly_iGet_clvr(index_constant<Index> *) const & noexcept -> T const & {
        return *this;
    }
    [[nodiscard]] constexpr auto
    ugly_iGet_lvr(index_constant<Index> *) & noexcept -> T & {
        return *this;
    }
    [[nodiscard]] constexpr auto
    ugly_iGet_rvr(index_constant<Index> *) && noexcept -> T && {
        return std::move(*this);
    }

    template <typename U>
        requires(std::is_same_v<U, T> or ... or std::is_same_v<U, Ts>)
    [[nodiscard]] constexpr auto
    ugly_tGet_clvr(tag_constant<U> *) const & noexcept -> T const & {
        return *this;
    }
    template <typename U>
        requires(std::is_same_v<U, T> or ... or std::is_same_v<U, Ts>)
    [[nodiscard]] constexpr auto ugly_tGet_lvr(tag_constant<U> *) & noexcept
        -> T & {
        return *this;
    }
    template <typename U>
        requires(std::is_same_v<U, T> or ... or std::is_same_v<U, Ts>)
    [[nodiscard]] constexpr auto ugly_tGet_rvr(tag_constant<U> *) && noexcept
        -> T && {
        return std::move(*this);
    }

    constexpr static auto ugly_Value(index_constant<Index> *) -> T;
    constexpr auto ugly_Value_clvr() const & -> T const & { return *this; }
    constexpr auto ugly_Value_lvr() & -> T & { return *this; }
    constexpr auto ugly_Value_rvr() && -> T && { return std::move(*this); }
};

template <typename Op, typename Value> struct fold_helper {
    Op op;
    Value value;

  private:
    template <typename Rhs>
    [[nodiscard]] friend constexpr auto operator+(fold_helper &&lhs,
                                                  Rhs &&rhs) {
        using R =
            decltype(lhs.op(std::move(lhs).value, std::forward<Rhs>(rhs)));
        return fold_helper<Op, std::remove_cvref_t<R>>{
            lhs.op, lhs.op(std::move(lhs).value, std::forward<Rhs>(rhs))};
    }

    template <typename Lhs>
    [[nodiscard]] friend constexpr auto operator+(Lhs &&lhs,
                                                  fold_helper &&rhs) {
        using R =
            decltype(rhs.op(std::forward<Lhs>(lhs), std::move(rhs).value));
        return fold_helper<Op, std::remove_cvref_t<R>>{
            rhs.op, rhs.op(std::forward<Lhs>(lhs), std::move(rhs).value)};
    }
};
template <typename Op, typename Value>
fold_helper(Op, Value) -> fold_helper<Op, std::remove_cvref_t<Value>>;

template <typename Op, typename Value> struct join_helper {
    Op op;
    Value value;
};
template <typename Op, typename Value>
join_helper(Op, Value) -> join_helper<Op, std::remove_cvref_t<Value>>;

// Note: operator+ is not a hidden friend of join_helper to avoid template
// instantiation abiguity
template <typename Op, typename T, typename U>
[[nodiscard]] constexpr auto operator+(join_helper<Op, T> &&lhs,
                                       join_helper<Op, U> &&rhs) {
    using R = decltype(lhs.op(std::move(lhs).value, std::move(rhs).value));
    return join_helper<Op, std::remove_cvref_t<R>>{
        lhs.op, lhs.op(std::move(lhs).value, std::move(rhs).value)};
}

template <template <typename> typename...> struct index_function_list;
template <typename...> struct tuple_impl;

template <template <typename> typename... Fs> struct element_helper {
    template <std::size_t I, typename T>
    using element_t = element<I, T, Fs<std::remove_cvref_t<T>>...>;
};

struct index_pair {
    std::size_t outer;
    std::size_t inner;
};

template <typename...> constexpr auto always_false_v = false;

template <std::size_t... Is, template <typename> typename... Fs, typename... Ts>
struct tuple_impl<std::index_sequence<Is...>, index_function_list<Fs...>, Ts...>
    : element_helper<Fs...>::template element_t<Is, Ts>... {
  private:
    template <std::size_t I, typename T>
    using base_t = typename element_helper<Fs...>::template element_t<I, T>;

  public:
    using base_t<Is, Ts>::ugly_iGet_clvr...;
    using base_t<Is, Ts>::ugly_iGet_lvr...;
    using base_t<Is, Ts>::ugly_iGet_rvr...;
    using base_t<Is, Ts>::ugly_tGet_clvr...;
    using base_t<Is, Ts>::ugly_tGet_lvr...;
    using base_t<Is, Ts>::ugly_tGet_rvr...;
    using base_t<Is, Ts>::ugly_Value...;

    template <typename Init, typename Op>
    [[nodiscard]] constexpr inline auto fold_left(Init &&init,
                                                  Op &&op) const & {
        return (fold_helper{op, std::forward<Init>(init)} + ... +
                this->base_t<Is, Ts>::ugly_Value_clvr())
            .value;
    }
    template <typename Init, typename Op>
    [[nodiscard]] constexpr inline auto fold_left(Init &&init, Op &&op) && {
        return (fold_helper{op, std::forward<Init>(init)} + ... +
                std::move(*this).base_t<Is, Ts>::ugly_Value_rvr())
            .value;
    }

    template <typename Init, typename Op>
    [[nodiscard]] constexpr inline auto fold_right(Init &&init,
                                                   Op &&op) const & {
        return (this->base_t<Is, Ts>::ugly_Value_clvr() + ... +
                fold_helper{op, std::forward<Init>(init)})
            .value;
    }
    template <typename Init, typename Op>
    [[nodiscard]] constexpr inline auto fold_right(Init &&init, Op &&op) && {
        return (std::move(*this).base_t<Is, Ts>::ugly_Value_rvr() + ... +
                fold_helper{op, std::forward<Init>(init)})
            .value;
    }

    template <std::size_t I>
    [[nodiscard]] constexpr auto
    operator[](index_constant<I> *i) const & -> decltype(auto) {
        static_assert(I < sizeof...(Ts), "Tuple index out of bounds!");
        return this->ugly_iGet_clvr(i);
    }
    template <std::size_t I>
    [[nodiscard]] constexpr auto
    operator[](index_constant<I> *i) & -> decltype(auto) {
        static_assert(I < sizeof...(Ts), "Tuple index out of bounds!");
        return this->ugly_iGet_lvr(i);
    }
    template <std::size_t I>
    [[nodiscard]] constexpr auto
    operator[](index_constant<I> *i) && -> decltype(auto) {
        static_assert(I < sizeof...(Ts), "Tuple index out of bounds!");
        return std::move(*this).ugly_iGet_rvr(i);
    }

    constexpr auto ugly_tGet_clvr(auto idx) const & -> void {
        static_assert(always_false_v<decltype(idx), Ts...>,
                      "Type not found in tuple!");
    }
    constexpr auto ugly_tGet_lvr(auto idx) & -> void {
        static_assert(always_false_v<decltype(idx), Ts...>,
                      "Type not found in tuple!");
    }
    constexpr auto ugly_tGet_rvr(auto idx) && -> void {
        static_assert(always_false_v<decltype(idx), Ts...>,
                      "Type not found in tuple!");
    }

    [[nodiscard]] constexpr auto get(auto idx) const & -> decltype(auto) {
        return this->ugly_tGet_clvr(idx);
    }
    [[nodiscard]] constexpr auto get(auto idx) & -> decltype(auto) {
        return this->ugly_tGet_lvr(idx);
    }
    [[nodiscard]] constexpr auto get(auto idx) && -> decltype(auto) {
        return std::move(*this).ugly_tGet_rvr(idx);
    }

    template <typename Op>
    constexpr auto apply(Op &&op) const & -> decltype(auto) {
        return std::forward<Op>(op)(this->base_t<Is, Ts>::ugly_Value_clvr()...);
    }
    template <typename Op> constexpr auto apply(Op &&op) & -> decltype(auto) {
        return std::forward<Op>(op)(this->base_t<Is, Ts>::ugly_Value_lvr()...);
    }
    template <typename Op> constexpr auto apply(Op &&op) && -> decltype(auto) {
        return std::forward<Op>(op)(
            std::move(*this).base_t<Is, Ts>::ugly_Value_rvr()...);
    }

    template <typename Op>
        requires(sizeof...(Ts) > 0)
    constexpr auto join(Op &&op) const & -> decltype(auto) {
        return (... + join_helper{op, this->base_t<Is, Ts>::ugly_Value_clvr()})
            .value;
    }
    template <typename Op>
        requires(sizeof...(Ts) > 0)
    constexpr auto join(Op &&op) && -> decltype(auto) {
        return (... +
                join_helper{op,
                            std::move(*this).base_t<Is, Ts>::ugly_Value_rvr()})
            .value;
    }

    constexpr static auto size =
        std::integral_constant<std::size_t, sizeof...(Ts)>{};
    constexpr static auto ugly_Value(...) -> void;

    [[nodiscard]] constexpr static auto fill_inner_indices(index_pair *p)
        -> index_pair * {
        ((p++->inner = Is), ...);
        return p;
    }
    [[nodiscard]] constexpr static auto
    fill_outer_indices(index_pair *p, [[maybe_unused]] std::size_t n)
        -> index_pair * {
        ((p++->outer = (static_cast<void>(Is), n)), ...);
        return p;
    }

  private:
    template <typename Indices, typename Funcs, typename... Us>
    [[nodiscard]] friend constexpr auto
    operator==(tuple_impl const &lhs,
               tuple_impl<Indices, Funcs, Us...> const &rhs) -> bool {
        return (... and (lhs[index<Is>] == rhs[index<Is>]));
    }

    template <typename Indices, typename Funcs, typename... Us>
    [[nodiscard]] friend constexpr auto
    operator<=>(tuple_impl const &lhs,
                tuple_impl<Indices, Funcs, Us...> const &rhs) {
        if constexpr (sizeof...(Is) == 0) {
            return std::strong_ordering::equal;
        } else {
            using C =
                std::common_comparison_category_t<decltype(lhs[index<Is>] <=>
                                                           rhs[index<Is>])...>;
            C result = lhs[index<0>] <=> rhs[index<0>];
            auto const compare_at = [&]<std::size_t I>() {
                result = lhs[index<I>] <=> rhs[index<I>];
                return result != 0;
            };
            [[maybe_unused]] auto b =
                (compare_at.template operator()<Is>() or ...);
            return result;
        }
    }
};

template <typename... Ts>
tuple_impl(Ts...)
    -> tuple_impl<std::index_sequence_for<Ts...>, index_function_list<>, Ts...>;
} // namespace detail

template <typename T> constexpr auto tuple_size_v = T::size();

template <std::size_t I, typename T>
using tuple_element_t = decltype(T::ugly_Value(index<I>));

template <typename... Ts>
struct tuple : detail::tuple_impl<std::index_sequence_for<Ts...>,
                                  detail::index_function_list<>, Ts...> {
  private:
    [[nodiscard]] friend constexpr auto operator==(tuple const &, tuple const &)
        -> bool = default;
    [[nodiscard]] friend constexpr auto operator<=>(tuple const &,
                                                    tuple const &) = default;
};
template <typename... Ts> tuple(Ts...) -> tuple<Ts...>;

template <typename IndexList, typename... Ts>
struct indexed_tuple
    : detail::tuple_impl<std::index_sequence_for<Ts...>, IndexList, Ts...> {
  private:
    [[nodiscard]] friend constexpr auto operator==(indexed_tuple const &,
                                                   indexed_tuple const &)
        -> bool = default;
    [[nodiscard]] friend constexpr auto
    operator<=>(indexed_tuple const &, indexed_tuple const &) = default;
};

template <typename... Ts>
indexed_tuple(Ts...) -> indexed_tuple<detail::index_function_list<>, Ts...>;

template <std::size_t I, typename Tuple>
[[nodiscard]] constexpr auto get(Tuple &&t)
    -> decltype(std::forward<Tuple>(t)[index<I>]) {
    return std::forward<Tuple>(t)[index<I>];
}

template <typename T, typename Tuple>
[[nodiscard]] constexpr auto get(Tuple &&t)
    -> decltype(std::forward<Tuple>(t).get(tag<T>)) {
    return std::forward<Tuple>(t).get(tag<T>);
}

template <typename... Ts> [[nodiscard]] constexpr auto make_tuple(Ts &&...ts) {
    return tuple<std::remove_cvref_t<Ts>...>{std::forward<Ts>(ts)...};
}

template <template <typename> typename... Fs>
constexpr auto make_indexed_tuple = []<typename... Ts>(Ts &&...ts) {
    return indexed_tuple<detail::index_function_list<Fs...>,
                         std::remove_cvref_t<Ts>...>{std::forward<Ts>(ts)...};
};

template <template <typename> typename... Fs, typename T>
constexpr auto apply_indices(T &&t) {
    using tuple_t = std::remove_cvref_t<T>;
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return indexed_tuple<detail::index_function_list<Fs...>,
                             tuple_element_t<Is, tuple_t>...>{
            std::forward<T>(t)[index<Is>]...};
    }(std::make_index_sequence<tuple_size_v<tuple_t>>{});
}

template <typename... Ts> auto forward_as_tuple(Ts &&...ts) {
    return stdx::tuple<Ts &&...>{std::forward<Ts>(ts)...};
}
} // namespace v1
} // namespace stdx

