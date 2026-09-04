#pragma once

#include <array>
#include <exception> // for std::terminate
#include <utility>

#include <utils/storage/strong_index_array.hpp>

namespace enum_array {

template <typename Enum>
constexpr size_t default_index_f(Enum e) {
    return std::to_underlying(e);
}

using strong_index_array::AllowWeakIndexing;

} // namespace enum_array

/// A wrapper over a std::array with some small extra features to work better for enum indexing:
/// - Initializer takes std::pair<Enum, Value> and checks that the Enum matches the array index (at compile time)
/// - Constructor is consteval and the initializer must be the same size as \p cnt
/// - Array now indexable with the enum type, no need to cast to_underlying
///
/// Functionally, this is equivalent to StrongIndexArray
template <
    typename Enum,
    typename Value,
    auto cnt,
    enum_array::AllowWeakIndexing allow_weak_indexing = enum_array::AllowWeakIndexing::yes,
    auto index_f = enum_array::default_index_f<Enum> //
    >
struct EnumArray final : public StrongIndexArray<Value, static_cast<size_t>(cnt), Enum, index_f, allow_weak_indexing> {
    using Array = std::array<Value, static_cast<size_t>(cnt)>;

    constexpr EnumArray() noexcept {}

    explicit consteval EnumArray(std::initializer_list<std::pair<Enum, Value>> items) noexcept {
        // Check that the sizes match
        if (items.size() != static_cast<size_t>(cnt)) {
            std::terminate();
        }

        size_t i = 0;
        for (const auto &pair : items) {
            Array::operator[](i) = pair.second;

            // Check that indexes match
            if (index_f(pair.first) != i) {
                std::terminate();
            }

            i++;
        }
    }

    inline constexpr Value get_fallback(Enum key, Enum fallback_enum) const {
        if (const auto i = index_f(key); i < this->size()) {
            return Array::operator[](i);
        } else {
            return Array::operator[](index_f(fallback_enum));
        }
    }

    inline constexpr Value get_or(Enum v, Value fallback_value) const {
        const auto i = index_f(v);
        return (i < this->size()) ? Array::operator[](i) : fallback_value;
    }
};
