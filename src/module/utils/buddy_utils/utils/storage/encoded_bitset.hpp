/// @file
#pragma once

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <type_traits>
#include <bsod/bsod.h>

/// A bitset backed by a plain uint32_t with guaranteed binary representation.
///
/// This happens to be the same binary representation as std::bitset and we are
/// going to use the fact for backwards compatibility in the EEPROM storage.
///
/// Mostly just mimicks the std::bitset API, just "conserving" the internal
/// layout for future compatibility.
template <size_t N>
    requires(N > 0 && N <= 32)
struct EncodedBitset {
    static constexpr uint32_t mask = (N == 32) ? ~uint32_t(0) : ((uint32_t(1) << N) - 1);

    // Public only because C++ requires all members to be public for types
    // used as non-type template parameters (structural types). Use the
    // accessor methods instead of accessing this directly.
    uint32_t data_ = 0;

    constexpr EncodedBitset() = default;

    /// Returns a bitset with all N bits set.
    static constexpr EncodedBitset all() {
        return ~EncodedBitset();
    }

    /// Construct from a raw integer value.
    /// Useful for patterns like store_item.set(1 << 0).
    /// Out-of-range bits are masked off.
    constexpr EncodedBitset(uint32_t val)
        : data_(val & mask) {}

    [[nodiscard]] constexpr uint32_t data() const {
        return data_;
    }

    [[nodiscard]] constexpr bool test(size_t pos) const {
        debug_assert(pos < N);
        return (data_ >> pos) & 1;
    }

    [[nodiscard]] constexpr bool operator[](size_t pos) const {
        return test(pos);
    }

    constexpr EncodedBitset &set(size_t pos, bool val = true) {
        debug_assert(pos < N);
        if (val) {
            data_ |= (uint32_t(1) << pos);
        } else {
            data_ &= ~(uint32_t(1) << pos);
        }
        return *this;
    }

    constexpr EncodedBitset &reset(size_t pos) {
        return set(pos, false);
    }

    [[nodiscard]] constexpr EncodedBitset operator~() const {
        return EncodedBitset(~data_ & mask);
    }

    constexpr bool operator==(const EncodedBitset &) const = default;
};

// Few sanity checks that we are binary compatible with the std::bitset at the
// time of writing this.
static_assert(std::is_trivially_copyable_v<EncodedBitset<16>>);
static_assert(std::default_initializable<EncodedBitset<16>>);
static_assert(std::equality_comparable<EncodedBitset<16>>);
static_assert(std::is_standard_layout_v<EncodedBitset<16>>);

static_assert(sizeof(EncodedBitset<16>) == 4);
static_assert(sizeof(EncodedBitset<32>) == 4);
static_assert(offsetof(EncodedBitset<16>, data_) == 0);
