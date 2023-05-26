#pragma once
#include "common/aggregate_arity.hpp"
#include "common/to_tie.hpp"
#include <array>
#include <cstdint>
#include "bsod.h"

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
using CurrentItemIndex = Index<void (*)(std::span<uint8_t>, U &)>;
template <typename U, size_t... I>
consteval auto get_current_indices(std::index_sequence<I...>) {
    return std::to_array<CurrentItemIndex<U>>({ (CurrentItemIndex<U>(
        std::remove_cvref_t<std::tuple_element_t<I, U>>::hashed_id,
        [](std::span<uint8_t> raw_data, U &tuple) {
            using ItemT = std::remove_cvref_t<std::tuple_element_t<I, U>>;
            if (raw_data.size() != ItemT::data_size) {
                bsod("unexpected size difference");
            }
            typename ItemT ::type data;
            memcpy(&data, raw_data.data(), raw_data.size());
            std::get<I>(tuple).init(data);
        }))... });
}
using DeprecatedItemIndex = Index<std::pair<uint16_t, std::span<uint8_t>> (*)(std::array<uint8_t, 512> &, uint16_t)>;
template <typename U, size_t... I>
consteval auto get_deprecated_indices(std::index_sequence<I...>) {
    return std::to_array<DeprecatedItemIndex, sizeof...(I)>({ DeprecatedItemIndex(
        std::remove_cvref_t<std::tuple_element_t<I, U>>::hashed_id,
        [](std::array<uint8_t, 512> &buffer, uint16_t used_bytes) {
            using ItemT = std::remove_cvref_t<std::tuple_element_t<I, U>>;
            if (used_bytes != ItemT::data_size) {
                bsod("unexpected size difference");
            }
            typename ItemT::type data;
            memcpy(&data, buffer.data(), used_bytes);

            typename ItemT::next_type::type new_data(data);
            memcpy(buffer.data(), &new_data, sizeof(new_data));
            return std::make_pair(ItemT::next_type::hashed_id, std::span<uint8_t>(buffer.data(), sizeof(new_data)));
        }

        )... });
}

}
/**
 * Creates sorted array of ids and functions, the function takes data, updates them to the new data and then serializes them and returns them with id of next item
 * @tparam T struct of deprecated items
 */
template <class T>
auto constexpr get_deprecated_indices() {
    using TupleT = std::invoke_result<decltype(detail::to_tie<T>), T &>::type;
    auto index = detail::get_deprecated_indices<TupleT>(std::make_index_sequence<std::tuple_size_v<TupleT>> {});
    std::sort(begin(index), end(index), [](const auto &a, const auto &b) { return a.id < b.id; });
    return index;
}

/**
 * Creates sorted array of ids and functions, the function takes data and tuple of references to items in configuration store and loads the data to the item
 * @tparam T struct of current items in store
 */
template <class T>
auto constexpr get_current_indices() {
    using TupleT = std::invoke_result<decltype(detail::to_tie<T>), T &>::type;
    auto index = detail::get_current_indices<TupleT>(std::make_index_sequence<std::tuple_size_v<TupleT>> {});
    std::sort(begin(index), end(index), [](const auto &a, const auto &b) { return a.id < b.id; });
    return index;
}
