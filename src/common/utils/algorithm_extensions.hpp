#pragma once

#include <algorithm>

namespace stdext {

/// \returns index of the first occurence of \p item inside \p container or \p container.size() if the item is not in the container
template <typename T, typename I>
constexpr size_t index_of(T &&container, I &&item) {
    return std::find(container.begin(), container.end(), item) - container.begin();
}

/// Calls \p f(item) over each \p tuple item
template <typename Tuple, typename F>
constexpr void visit_tuple(Tuple &&tuple, F &&f) {
    std::apply([&](auto &&...item) { (f(item), ...); }, tuple);
}

/// Calls \p f.operator()<ix>() over every \p ix in 0 .. \p count
template <size_t count, typename F>
constexpr void visit_sequence(F &&f) {
    [&]<size_t... ix>(std::index_sequence<ix...>) {
        ((f.template operator()<ix>()), ...);
    }(std::make_index_sequence<count>());
}

} // namespace stdext
