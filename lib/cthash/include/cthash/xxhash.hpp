#ifndef CTHASH_XXHASH_HPP
#define CTHASH_XXHASH_HPP

#include "simple.hpp"
#include "value.hpp"
#include "internal/assert.hpp"
#include "internal/bit.hpp"
#include "internal/concepts.hpp"
#include "internal/convert.hpp"
#include "internal/deduce.hpp"
#include <array>
#include <span>
#include <string_view>

namespace cthash {

template <size_t Bits>
struct xxhash_types;

template <std::unsigned_integral T, byte_like Byte>
constexpr auto read_le_number_from(std::span<const Byte> &input) noexcept {
    CTHASH_ASSERT(input.size() >= sizeof(T));

    const auto r = cast_from_le_bytes<T>(input.template first<sizeof(T)>());
    input = input.subspan(sizeof(T));

    return r;
}

template <std::unsigned_integral T, size_t Off, byte_like Byte, size_t N>
constexpr auto get_le_number_from(std::span<const Byte, N> input) noexcept {
    return cast_from_le_bytes<T>(input.template subspan<Off * sizeof(T), sizeof(T)>());
}

template <>
struct xxhash_types<32> {
    using value_type = uint32_t;
    using acc_array = std::array<value_type, 4>;
    static constexpr auto primes = std::array<value_type, 5> { 2654435761U, 2246822519U, 3266489917U, 668265263U, 374761393U };

    static constexpr auto round(value_type accN, value_type laneN) noexcept -> value_type {
        return std::rotl(accN + (laneN * primes[1]), 13u) * primes[0];
    }

    static constexpr auto convergence(const acc_array &accs) noexcept -> value_type {
        return std::rotl(accs[0], 1u) + std::rotl(accs[1], 7u) + std::rotl(accs[2], 12u) + std::rotl(accs[3], 18u);
    };

    template <byte_like Byte>
    static constexpr auto consume_remaining(value_type acc, std::span<const Byte> input) noexcept -> value_type {
        CTHASH_ASSERT(input.size() < sizeof(acc_array));

        while (input.size() >= sizeof(uint32_t)) {
            const auto lane = read_le_number_from<uint32_t>(input);
            acc = std::rotl(acc + (lane * primes[2]), 17u) * primes[3];
        }

        while (input.size() >= 1u) {
            const auto lane = read_le_number_from<uint8_t>(input);
            acc = std::rotl(acc + lane * primes[4], 11u) * primes[0];
        }

        return acc;
    }

    static constexpr auto avalanche(value_type acc) noexcept -> value_type {
        acc = (acc xor (acc >> 15u)) * primes[1];
        acc = (acc xor (acc >> 13u)) * primes[2];
        return (acc xor (acc >> 16u));
    }
};

template <>
struct xxhash_types<64> {
    using value_type = uint64_t;
    using acc_array = std::array<value_type, 4>;
    static constexpr auto primes = std::array<value_type, 5> { 11400714785074694791ULL, 14029467366897019727ULL, 1609587929392839161ULL, 9650029242287828579ULL, 2870177450012600261ULL };

    static constexpr auto round(value_type accN, value_type laneN) noexcept -> value_type {
        return std::rotl(accN + (laneN * primes[1]), 31u) * primes[0];
    }

    static constexpr auto convergence(const acc_array &accs) noexcept -> value_type {
        constexpr auto merge = [](value_type acc, value_type accN) {
            return ((acc xor round(0, accN)) * primes[0]) + primes[3];
        };

        value_type acc = std::rotl(accs[0], 1u) + std::rotl(accs[1], 7u) + std::rotl(accs[2], 12u) + std::rotl(accs[3], 18u);
        acc = merge(acc, accs[0]);
        acc = merge(acc, accs[1]);
        acc = merge(acc, accs[2]);
        return merge(acc, accs[3]);
    };

    template <byte_like Byte>
    static constexpr auto consume_remaining(value_type acc, std::span<const Byte> input) noexcept -> value_type {
        CTHASH_ASSERT(input.size() < sizeof(acc_array));

        while (input.size() >= sizeof(uint64_t)) {
            const auto lane = read_le_number_from<uint64_t>(input);
            acc = (std::rotl(acc xor round(0, lane), 27u) * primes[0]) + primes[3];
        }

        if (input.size() >= sizeof(uint32_t)) {
            const auto lane = read_le_number_from<uint32_t>(input);
            acc = (std::rotl(acc xor (lane * primes[0]), 23u) * primes[1]) + primes[2];
        }

        while (input.size() >= 1u) {
            const auto lane = read_le_number_from<uint8_t>(input);
            acc = (std::rotl(acc xor (lane * primes[4]), 11u) * primes[0]);
        }

        return acc;
    }

    static constexpr auto avalanche(value_type acc) noexcept -> value_type {
        acc = (acc xor (acc >> 33u)) * primes[1];
        acc = (acc xor (acc >> 29u)) * primes[2];
        return (acc xor (acc >> 32u));
    }
};

template <size_t Bits>
struct xxhash {
    static_assert(Bits == 32u || Bits == 64u);

    struct tag {
        static constexpr size_t digest_length = Bits / 8u;
    };

    using config = xxhash_types<Bits>;
    using acc_array = typename config::acc_array;
    using value_type = typename config::value_type;

    using digest_span_t = std::span<std::byte, Bits / 8u>;

    // members
    value_type seed { 0u };
    value_type length { 0u };
    acc_array internal_state {};
    std::array<std::byte, sizeof(value_type) * 4u> buffer {};

    // step 1 in constructor
    explicit constexpr xxhash(value_type s = 0u) noexcept
        : seed { s }
        , internal_state { seed + config::primes[0] + config::primes[1], seed + config::primes[1], seed, seed - config::primes[0] } {}

    template <byte_like Byte>
    constexpr void process_lanes(std::span<const Byte, sizeof(acc_array)> lanes) noexcept {
        // step 2: process lanes
        internal_state[0] = config::round(internal_state[0], get_le_number_from<value_type, 0>(lanes));
        internal_state[1] = config::round(internal_state[1], get_le_number_from<value_type, 1>(lanes));
        internal_state[2] = config::round(internal_state[2], get_le_number_from<value_type, 2>(lanes));
        internal_state[3] = config::round(internal_state[3], get_le_number_from<value_type, 3>(lanes));
    }

    constexpr size_t buffer_usage() const noexcept {
        return length % buffer.size();
    }

    template <byte_like Byte>
    constexpr void process_blocks(std::span<const Byte> &input) noexcept {
        while (input.size() >= buffer.size()) {
            const auto current_lanes = input.template first<sizeof(acc_array)>();
            input = input.subspan(buffer.size());

            process_lanes(current_lanes);
        }
    }

    template <byte_like Byte>
    [[gnu::flatten]] constexpr xxhash &update(std::span<const Byte> input) noexcept {
        const auto buffer_remaining = std::span(buffer).subspan(buffer_usage());

        // everything we insert here is counting as part of input (even if we process it later)
        length += static_cast<uint8_t>(input.size());

        // if there is remaining data from previous...
        if (buffer_remaining.size() != buffer.size()) {
            const auto to_copy = input.first(std::min(input.size(), buffer_remaining.size()));
            byte_copy(to_copy.begin(), to_copy.end(), buffer_remaining.begin());
            input = input.subspan(to_copy.size());

            // if we didn't fill current block, we will do it later
            if (buffer_remaining.size() != to_copy.size()) {
                CTHASH_ASSERT(input.size() == 0u);
                return *this;
            }

            // but if we did, we need to process it
            const auto full_buffer_view = std::span<const std::byte, sizeof(acc_array)>(buffer);
            process_lanes(full_buffer_view);
        }

        // process blocks
        process_blocks(input);

        // copy remainder of input to the buffer, so it's processed later
        byte_copy(input.begin(), input.end(), buffer.begin());
        return *this;
    }

    template <one_byte_char CharT>
    [[gnu::flatten]] constexpr xxhash &update(std::basic_string_view<CharT> input) noexcept {
        return update(std::span<const CharT>(input.data(), input.size()));
    }

    template <string_literal T>
    [[gnu::flatten]] constexpr xxhash &update(const T &input) noexcept {
        return update(std::span(std::data(input), std::size(input) - 1u));
    }

    template <byte_like Byte>
    [[gnu::flatten]] constexpr auto update_and_final(std::span<const Byte> input) noexcept {
        length = static_cast<value_type>(input.size());
        process_blocks(input);
        tagged_hash_value<tag> output;
        final_from(input, output);
        return output;
    }

    template <one_byte_char CharT>
    [[gnu::flatten]] constexpr auto update_and_final(std::basic_string_view<CharT> input) noexcept {
        return update_and_final(std::span<const CharT>(input.data(), input.size()));
    }

    template <string_literal T>
    [[gnu::flatten]] constexpr auto update_and_final(const T &input) noexcept {
        return update_and_final(std::span(std::data(input), std::size(input) - 1u));
    }

    constexpr auto converge_conditionaly() const noexcept -> value_type {
        // step 1 shortcut for short input
        if (length < buffer.size()) {
            return seed + config::primes[4];
        }

        // otherwise we need to merge&converge internal state
        return config::convergence(internal_state);
    }

    template <byte_like Byte>
    constexpr void final_from(std::span<const Byte> source, digest_span_t out) const noexcept {
        CTHASH_ASSERT(source.size() < buffer.size());

        value_type acc = converge_conditionaly();

        // step 4: add input length
        acc += static_cast<value_type>(length);

        // step 5: consume remainder (not finished block from buffer)
        acc = config::consume_remaining(acc, source);

        // step 6: final mix/avalanche
        acc = config::avalanche(acc);

        // convert to big endian representation
        unwrap_bigendian_number<value_type> { out } = acc;
    }

    [[gnu::flatten]] constexpr void final(digest_span_t out) const noexcept {
        const auto buffer_used = std::span<const std::byte>(buffer).first(buffer_usage());
        final_from(buffer_used, out);
    }

    [[gnu::flatten]] constexpr auto final() const noexcept -> tagged_hash_value<tag> {
        tagged_hash_value<tag> output;
        this->final(output);
        return output;
    }
};

using xxhash32 = cthash::xxhash<32>;
using xxhash32_value = tagged_hash_value<xxhash32::tag>;

using xxhash64 = cthash::xxhash<64>;
using xxhash64_value = tagged_hash_value<xxhash64::tag>;

namespace literals {

    template <internal::fixed_string Value>
    consteval auto operator""_xxh32() {
        return xxhash32_value(Value);
    }

    template <internal::fixed_string Value>
    consteval auto operator""_xxh64() {
        return xxhash64_value(Value);
    }

} // namespace literals

} // namespace cthash

#endif
