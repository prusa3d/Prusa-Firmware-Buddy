#pragma once

#if __cplusplus >= 202002L

#include <stdx/ct_conversions.hpp>
#include <stdx/tuple.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace stdx {
inline namespace v1 {
template <typename... Ts> [[nodiscard]] constexpr auto tuple_cat(Ts &&...ts) {
    if constexpr (sizeof...(Ts) == 0) {
        return stdx::tuple<>{};
    } else if constexpr (sizeof...(Ts) == 1) {
        return (ts, ...);
    } else {
        constexpr auto total_num_elements =
            (std::size_t{} + ... + stdx::tuple_size_v<std::remove_cvref_t<Ts>>);

        [[maybe_unused]] constexpr auto element_indices = [&] {
            std::array<detail::index_pair, total_num_elements> indices{};
            auto p = std::data(indices);
            ((p = std::remove_cvref_t<Ts>::fill_inner_indices(p)), ...);
            auto q = std::data(indices);
            std::size_t n{};
            ((q = std::remove_cvref_t<Ts>::fill_outer_indices(q, n++)), ...);
            return indices;
        }();

        [[maybe_unused]] auto outer_tuple =
            stdx::tuple<Ts &&...>{std::forward<Ts>(ts)...};
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            using T = stdx::tuple<stdx::tuple_element_t<
                element_indices[Is].inner,
                std::remove_cvref_t<
                    decltype(std::move(outer_tuple)
                                 .ugly_iGet_rvr(
                                     index<element_indices[Is].outer>))>>...>;
            return T{std::move(outer_tuple)
                         .ugly_iGet_rvr(index<element_indices[Is].outer>)
                             [index<element_indices[Is].inner>]...};
        }(std::make_index_sequence<total_num_elements>{});
    }
}

template <template <typename T> typename Pred, typename T>
[[nodiscard]] constexpr auto filter(T &&t) {
    using tuple_t = std::remove_cvref_t<T>;
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        constexpr auto num_matches =
            (std::size_t{} + ... +
             (Pred<stdx::tuple_element_t<Is, tuple_t>>::value ? std::size_t{1}
                                                              : std::size_t{}));
        constexpr auto indices = [] {
            auto a = std::array<std::size_t, num_matches>{};
            [[maybe_unused]] auto it = std::begin(a);
            [[maybe_unused]] auto copy_index =
                [&]<std::size_t I, typename Elem> {
                    if constexpr (Pred<Elem>::value) {
                        *it++ = I;
                    }
                };
            (copy_index
                 .template operator()<Is, stdx::tuple_element_t<Is, tuple_t>>(),
             ...);
            return a;
        }();

        return [&]<std::size_t... Js>(std::index_sequence<Js...>) {
            using R =
                stdx::tuple<stdx::tuple_element_t<indices[Js], tuple_t>...>;
            return R{std::forward<T>(t)[index<indices[Js]>]...};
        }(std::make_index_sequence<num_matches>{});
    }(std::make_index_sequence<stdx::tuple_size_v<tuple_t>>{});
}

namespace detail {
template <std::size_t I, typename... Ts>
constexpr auto invoke_at(auto &&op, Ts &&...ts) -> decltype(auto) {
    return op(std::forward<Ts>(ts)[index<I>]...);
}
} // namespace detail

template <template <typename> typename... Fs, typename Op, typename T,
          typename... Ts>
constexpr auto transform(Op &&op, T &&t, Ts &&...ts) {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        if constexpr (sizeof...(Fs) == 0) {
            return stdx::tuple<decltype(detail::invoke_at<Is>(
                std::forward<Op>(op), std::forward<T>(t),
                std::forward<Ts>(ts)...))...>{
                detail::invoke_at<Is>(std::forward<Op>(op), std::forward<T>(t),
                                      std::forward<Ts>(ts)...)...};
        } else {
            return stdx::make_indexed_tuple<Fs...>(
                detail::invoke_at<Is>(std::forward<Op>(op), std::forward<T>(t),
                                      std::forward<Ts>(ts)...)...);
        }
    }(std::make_index_sequence<stdx::tuple_size_v<std::remove_cvref_t<T>>>{});
}

template <typename Op, typename T>
constexpr auto apply(Op &&op, T &&t) -> decltype(auto) {
    return std::forward<T>(t).apply(std::forward<Op>(op));
}

template <typename Op, typename T, typename... Ts>
constexpr auto for_each(Op &&op, T &&t, Ts &&...ts) -> Op {
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (detail::invoke_at<Is>(op, std::forward<T>(t), std::forward<Ts>(ts)...),
         ...);
    }(std::make_index_sequence<stdx::tuple_size_v<std::remove_cvref_t<T>>>{});
    return op;
}

template <typename F, typename T, typename... Ts>
constexpr auto all_of(F &&f, T &&t, Ts &&...ts) -> bool {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... and detail::invoke_at<Is>(f, std::forward<T>(t),
                                              std::forward<Ts>(ts)...));
    }(std::make_index_sequence<stdx::tuple_size_v<std::remove_cvref_t<T>>>{});
}

template <typename F, typename T, typename... Ts>
constexpr auto any_of(F &&f, T &&t, Ts &&...ts) -> bool {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... or detail::invoke_at<Is>(f, std::forward<T>(t),
                                             std::forward<Ts>(ts)...));
    }(std::make_index_sequence<stdx::tuple_size_v<std::remove_cvref_t<T>>>{});
}

template <typename... Ts> constexpr auto none_of(Ts &&...ts) -> bool {
    return not any_of(std::forward<Ts>(ts)...);
}

namespace detail {
template <typename T, template <typename> typename F, typename... Us>
constexpr auto is_index_for = (std::is_same_v<F<Us>, T> or ...);

template <typename T, typename IndexSeq, template <typename> typename... Fs,
          typename... Us>
constexpr auto contains_type(
    stdx::detail::tuple_impl<IndexSeq, index_function_list<Fs...>, Us...> const
        &) -> std::bool_constant<(is_index_for<T, Fs, Us...> or ...) or
                                 (std::is_same_v<T, Us> or ...)>;
} // namespace detail

template <typename Tuple, typename T>
constexpr auto contains_type =
    decltype(detail::contains_type<T>(std::declval<Tuple>()))::value;

template <template <typename> typename Proj = std::type_identity_t,
          typename... Ts>
[[nodiscard]] constexpr auto sort(stdx::tuple<Ts...> t) {
    using P = std::pair<std::string_view, std::size_t>;
    constexpr auto indices = []<std::size_t... Is>(std::index_sequence<Is...>) {
        auto a = std::array<P, sizeof...(Is)>{
            P{stdx::type_as_string<Proj<Ts>>(), Is}...};
        std::sort(
            std::begin(a), std::end(a),
            [](auto const &p1, auto const &p2) { return p1.first < p2.first; });
        return a;
    }(std::make_index_sequence<sizeof...(Ts)>{});

    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return stdx::tuple{t[index<indices[Is].second>]...};
    }(std::make_index_sequence<sizeof...(Ts)>{});
}

namespace detail {
template <typename T, template <typename> typename Proj, std::size_t I>
[[nodiscard]] constexpr auto test_adjacent() -> bool {
    return stdx::type_as_string<Proj<stdx::tuple_element_t<I, T>>>() ==
           stdx::type_as_string<Proj<stdx::tuple_element_t<I + 1, T>>>();
}

template <typename T, template <typename> typename Proj = std::type_identity_t>
    requires(tuple_size_v<T> > 1)
[[nodiscard]] constexpr auto count_chunks() {
    auto count = std::size_t{1};
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        ((count +=
          static_cast<std::size_t>(not detail::test_adjacent<T, Proj, Is>())),
         ...);
    }(std::make_index_sequence<stdx::tuple_size_v<T> - 1>{});
    return count;
}

struct chunk {
    std::size_t offset{};
    std::size_t size{};
    friend constexpr auto operator==(chunk const &, chunk const &)
        -> bool = default;
};

template <typename T, template <typename> typename Proj = std::type_identity_t>
    requires(tuple_size_v<T> > 1)
[[nodiscard]] constexpr auto create_chunks() {
    auto index = std::size_t{};
    std::array<chunk, count_chunks<T, Proj>()> chunks{};
    ++chunks[index].size;
    auto check_next_chunk = [&]<std::size_t I>() {
        if (not detail::test_adjacent<T, Proj, I>()) {
            chunks[++index].offset = I + 1;
        }
        ++chunks[index].size;
    };

    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (check_next_chunk.template operator()<Is>(), ...);
    }(std::make_index_sequence<stdx::tuple_size_v<T> - 1>{});

    return chunks;
}
} // namespace detail

template <template <typename> typename Proj = std::type_identity_t,
          typename Tuple>
[[nodiscard]] constexpr auto chunk_by(Tuple &&t) {
    using tuple_t = std::remove_cvref_t<Tuple>;
    if constexpr (tuple_size_v<tuple_t> == 0) {
        return stdx::tuple{};
    } else if constexpr (tuple_size_v<tuple_t> == 1) {
        return stdx::make_tuple(std::forward<Tuple>(t));
    } else {
        constexpr auto chunks = detail::create_chunks<tuple_t, Proj>();
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return stdx::make_tuple(
                [&]<std::size_t... Js>(std::index_sequence<Js...>) {
                    constexpr auto offset = chunks[Is].offset;
                    return stdx::make_tuple(
                        std::forward<Tuple>(t)[index<offset + Js>]...);
                }(std::make_index_sequence<chunks[Is].size>{})...);
        }(std::make_index_sequence<std::size(chunks)>{});
    }
}

template <typename Tuple> [[nodiscard]] constexpr auto chunk(Tuple &&t) {
    return chunk_by(std::forward<Tuple>(t));
}
} // namespace v1
} // namespace stdx

#endif
