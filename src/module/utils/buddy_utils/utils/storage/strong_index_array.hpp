/// @file
#pragma once

#include <array>
#include <cstddef>

namespace strong_index_array {

enum class AllowWeakIndexing {
    /// Allow both Index type and size_t to be used for indexing
    yes,
    /// Allow only Index type to be used for indexing
    no
};

}

/// Extends std::array to support custom index types
/// @param Item type of item in array
/// @param Index type used for indexing
/// @param capacity size of array
/// @param index_f conversion function to get raw index from Index type
/// @param weak_indexing specifies if size_t can be used for indexing
template <typename Item, size_t capacity, typename Index, auto index_f, strong_index_array::AllowWeakIndexing weak_indexing = strong_index_array::AllowWeakIndexing::no>
class StrongIndexArray : public std::array<Item, capacity> {

public:
    using BaseArray = std::array<Item, capacity>;

    using BaseArray::operator=;

    inline constexpr const Item &operator[](Index index) const {
        return BaseArray::operator[](index_f(index));
    }

    inline constexpr Item &operator[](Index index) {
        return BaseArray::operator[](index_f(index));
    }

    inline constexpr const Item &at(Index index) const {
        return BaseArray::at(index_f(index));
    }

    inline constexpr Item &at(Index index) {
        return BaseArray::at(index_f(index));
    }

    // Do NOT remove these methods or the [[deprecated]] attributes;
    // they are an integral part of the class design.
    //
    // These overloads allow size_t indexing during refactoring but mark the usage
    // as deprecated (appearing as strikethrough in editor) to guide the transition
    // toward the strong Index type.

    [[deprecated("Try to use strong Index type for indexing")]]
    inline constexpr Item &
    operator[](size_t index)
        requires(weak_indexing == strong_index_array::AllowWeakIndexing::yes)
    {
        return BaseArray::operator[](index);
    }

    [[deprecated("Try to use strong Index type for indexing")]]
    inline constexpr const Item &
    operator[](size_t index) const
        requires(weak_indexing == strong_index_array::AllowWeakIndexing::yes)
    {
        return BaseArray::operator[](index);
    }

    [[deprecated("Try to use strong Index type for indexing")]]
    inline constexpr Item &at(size_t index)
        requires(weak_indexing == strong_index_array::AllowWeakIndexing::yes)
    {
        return BaseArray::at(index);
    }

    [[deprecated("Try to use strong Index type for indexing")]]
    inline constexpr const Item &at(size_t index) const
        requires(weak_indexing == strong_index_array::AllowWeakIndexing::yes)
    {
        return BaseArray::at(index);
    }
};

namespace std {

// std::tuple_size is only specialized for std::array itself, not for classes derived from it.
template <typename Item, size_t capacity, typename Index, auto index_f, ::strong_index_array::AllowWeakIndexing weak_indexing>
struct tuple_size<::StrongIndexArray<Item, capacity, Index, index_f, weak_indexing>> : integral_constant<size_t, capacity> {};

} // namespace std
