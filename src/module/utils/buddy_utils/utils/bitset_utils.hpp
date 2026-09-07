/// @file
#pragma once

#include <bit>
#include <bitset>
#include <type_traits>

/// Iterator that iterates over set bits in the provided bitset
struct SetBitsIterator {

public:
    // Necessary to satisfy std::input_iterator, even though it doesn't make sense
    using difference_type = std::ptrdiff_t;
    using value_type = size_t;

    using BitIndex = size_t;
    using Data = unsigned long;

public:
    template <size_t size>
    constexpr SetBitsIterator(const std::bitset<size> &bitset)
        : remaining_bits_(bitset.to_ulong()) {
        static_assert(std::is_same_v<Data, unsigned long>);
        static_assert(size <= sizeof(Data) * 8);
    }
    constexpr SetBitsIterator(const SetBitsIterator &o) = default;

    constexpr SetBitsIterator begin() const {
        return *this;
    }

    constexpr SetBitsIterator end() const {
        // Advancing the iterator clears the least significant set bit, so zero is the end sentinel.
        return SetBitsIterator { 0 };
    }

    constexpr BitIndex operator*() const {
        return std::countr_zero(remaining_bits_);
    }

    constexpr SetBitsIterator &operator++() {
        // Clear lowest set bit
        // https://www.geeksforgeeks.org/dsa/bit-tricks-competitive-programming/
        remaining_bits_ &= remaining_bits_ - 1;
        return *this;
    }

    constexpr SetBitsIterator operator++(int) {
        auto prev = *this;
        ++(*this);
        return prev;
    }

    constexpr bool operator==(const SetBitsIterator &) const = default;
    constexpr SetBitsIterator &operator=(const SetBitsIterator &) = default;

private:
    constexpr SetBitsIterator(Data data)
        : remaining_bits_(data) {}

private:
    Data remaining_bits_;
};

/// Treats the @p input as a set of bits,
/// transitively adds @p f(bit) for every bit already in the set.
///
/// @param f  Maps a bit index (size_t) to a set of bits to be added to the result.
///           Gets called exactly once for every set bit (in the result).
///
/// @returns  reflexive transitive closure of set @p input under relation @p f.
template <typename Bitset, typename F>
Bitset bitset_flood_fill(Bitset input, F &&f) {
    Bitset previous = {};
    Bitset result = input;

    while (result != previous) {
        const auto diff = (result ^ previous);
        previous = result;

        for (size_t i : SetBitsIterator { diff }) {
            result |= f(i);
        }
    }

    return result;
}
