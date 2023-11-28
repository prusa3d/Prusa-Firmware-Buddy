#pragma once
#include "common/aggregate_arity.hpp"
#include "common/to_tie.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include "bsod.h"
#include <span>
#include "concepts.hpp"

namespace journal {
namespace detail {

    template <class U>
    struct Index {
        uint16_t id;
        U fnc;

        constexpr Index(uint16_t id, U fnc)
            : id(id)
            , fnc(fnc) {};

        consteval bool operator<(const Index &other) const {
            return id < other.id;
        }
    };

    template <class U>
    using CurrentItemIndex = Index<void (*)(std::span<const uint8_t>, U &)>;

    template <typename U, size_t... I>
    consteval auto get_current_indices(std::index_sequence<I...>) {
        return std::to_array<CurrentItemIndex<U>>({ (CurrentItemIndex<U>(
            std::remove_cvref_t<stdx::tuple_element_t<I, U>>::hashed_id,
            [](std::span<const uint8_t> raw_data, U &tuple) {
                using ItemT = std::remove_cvref_t<stdx::tuple_element_t<I, U>>;
                if (raw_data.size() != ItemT::data_size) {
                    bsod("unexpected size difference");
                }
                typename ItemT ::value_type data;
                memcpy(&data, raw_data.data(), raw_data.size());
                stdx::get<I>(tuple).init(data);
            }))... });
    }
} // namespace detail

/**
 * Creates sorted array of ids and functions, the function takes data and tuple of references to items in configuration store and loads the data to the item
 * @tparam T struct of current items in store
 */
template <class T>
auto constexpr get_current_indices() {
    using TupleT = typename std::invoke_result<decltype(to_tie<T>), T &>::type;
    auto index = detail::get_current_indices<TupleT>(std::make_index_sequence<stdx::tuple_size_v<TupleT>> {});
    std::sort(std::begin(index), std::end(index), [](const auto &a, const auto &b) { return a.id < b.id; });
    return index;
}
} // namespace journal
