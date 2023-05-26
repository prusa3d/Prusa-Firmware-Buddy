#ifndef CTHASH_SHA3_COMMON_HPP
#define CTHASH_SHA3_COMMON_HPP

#include "keccak.hpp"
#include "../hasher.hpp"
#include "../internal/bit.hpp"
#include "../internal/convert.hpp"
#include "../simple.hpp"
#include "../value.hpp"

namespace cthash {

template <typename T, typename Y>
concept castable_to = requires(T val) {
    { static_cast<Y>(val) }
    ->std::same_as<Y>;
};

template <size_t N>
struct keccak_suffix {
    unsigned bits;
    std::array<std::byte, N> values;

    constexpr keccak_suffix(unsigned b, castable_to<std::byte> auto... v) noexcept
        : bits { b }
        , values { static_cast<std::byte>(v)... } {}
};

template <castable_to<std::byte>... Ts>
keccak_suffix(unsigned, Ts...)->keccak_suffix<sizeof...(Ts)>;

template <typename T>
struct identify;

template <typename T, byte_like Byte>
constexpr auto convert_prefix_into_aligned(std::span<const Byte> input, unsigned pos) noexcept -> std::array<std::byte, sizeof(T)> {
    CTHASH_ASSERT(input.size() <= sizeof(T));
    CTHASH_ASSERT(pos <= sizeof(T));
    CTHASH_ASSERT((input.size() + pos) <= sizeof(T));

    std::array<std::byte, sizeof(T)> buffer {};

    std::fill(buffer.begin(), buffer.end(), std::byte { 0 });
    std::transform(input.data(), input.data() + input.size(), buffer.data() + pos, [](auto v) { return static_cast<std::byte>(v); });

    return buffer;
}

template <typename T, byte_like Byte>
constexpr auto convert_prefix_into_value(std::span<const Byte> input, unsigned pos) noexcept -> uint64_t {
    const auto tmp = convert_prefix_into_aligned<T, Byte>(input, pos);
    return cast_from_le_bytes<T>(std::span<const std::byte, 8>(tmp));
}

template <typename Config>
struct basic_keccak_hasher {
    static_assert(Config::digest_length_bit % 8u == 0u);
    static_assert(Config::rate_bit % 8u == 0u);
    static_assert(Config::capacity_bit % 8u == 0u);

    static_assert((Config::rate_bit + Config::capacity_bit) == 1600u, "Only Keccak 1600 is implemented");

    static constexpr size_t digest_length = Config::digest_length_bit / 8u;
    static constexpr size_t rate = Config::rate_bit / 8u;
    static constexpr size_t capacity = Config::capacity_bit / 8u;

    using result_t = cthash::tagged_hash_value<Config>;
    using digest_span_t = std::span<std::byte, digest_length>;

    keccak::state_1600 internal_state {};
    uint8_t position { 0u };

    constexpr basic_keccak_hasher() noexcept {
        std::fill(internal_state.begin(), internal_state.end(), uint64_t { 0 });
    }

    template <byte_like T>
    constexpr size_t xor_overwrite_block(std::span<const T> input) noexcept {
        using value_t = keccak::state_1600::value_type;

        if ((std::is_constant_evaluated() | (std::endian::native != std::endian::little))) {
            CTHASH_ASSERT((size_t(position) + input.size()) <= rate);

            // unaligned prefix (by copying from left to right it should be little endian)
            if (position % sizeof(value_t) != 0u) {
                // xor unaligned value and move to aligned if possible
                const size_t prefix_size = std::min(input.size(), sizeof(value_t) - (position % sizeof(value_t)));
                internal_state[position / sizeof(uint64_t)] ^= convert_prefix_into_value<value_t>(input.first(prefix_size), static_cast<unsigned>(position % sizeof(value_t)));
                position += static_cast<uint8_t>(prefix_size);
                input = input.subspan(prefix_size);
            }

            // aligned blocks
            while (input.size() >= sizeof(value_t)) {
                // xor aligned value and move to next
                internal_state[position / sizeof(value_t)] ^= cast_from_le_bytes<value_t>(input.template first<sizeof(value_t)>());
                position += static_cast<uint8_t>(sizeof(value_t));
                input = input.subspan(sizeof(value_t));
            }

            // unaligned suffix
            if (not input.empty()) {
                // xor and finish
                internal_state[position / sizeof(value_t)] ^= convert_prefix_into_value<value_t>(input, 0u);
                position += static_cast<uint8_t>(input.size());
            }

            return position;
        } else {
            const auto buffer = std::as_writable_bytes(std::span<uint64_t>(internal_state));
            const auto remaining = buffer.subspan(position);
            const auto place = remaining.first(std::min(input.size(), remaining.size()));

            std::transform(place.data(), place.data() + place.size(), input.data(), place.data(), [](std::byte lhs, auto rhs) { return lhs ^ static_cast<std::byte>(rhs); });

            position += static_cast<uint8_t>(place.size());
            return position;
        }
    }

    template <byte_like T>
    constexpr auto update(std::span<const T> input) noexcept {
        CTHASH_ASSERT(position < rate);
        const size_t remaining_in_buffer = rate - position;

        if (remaining_in_buffer > input.size()) {
            // xor overwrite as much as we can, and that's all
            xor_overwrite_block(input);
            CTHASH_ASSERT(position < rate);
            return;
        }

        // finish block and call keccak :)
        const auto first_part = input.first(remaining_in_buffer);
        input = input.subspan(remaining_in_buffer);
        xor_overwrite_block(first_part);
        CTHASH_ASSERT(position == rate);
        keccak_f(internal_state);
        position = 0u;

        // for each full block we can absorb directly
        while (input.size() >= rate) {
            const auto block = input.template first<rate>();
            input = input.subspan(rate);
            CTHASH_ASSERT(position == 0u);
            xor_overwrite_block<T>(block);
            keccak_f(internal_state);
            position = 0u;
        }

        // xor overwrite internal state with current remainder, and set position to end of it
        if (not input.empty()) {
            CTHASH_ASSERT(position == 0u);
            xor_overwrite_block(input);
            CTHASH_ASSERT(position < rate);
        }
    }

    // pad the message
    constexpr void xor_padding_block() noexcept {
        CTHASH_ASSERT(position < rate);

        constexpr const auto &suffix = Config::suffix;
        constexpr std::byte suffix_and_start_of_padding = (suffix.values[0] | (std::byte { 0b0000'0001u } << suffix.bits));

        internal_state[position / sizeof(uint64_t)] ^= uint64_t(suffix_and_start_of_padding) << ((position % sizeof(uint64_t)) * 8u);
        internal_state[(rate - 1u) / sizeof(uint64_t)] ^= 0x8000000000000000ull; // last bit
    }

    constexpr void final_absorb() noexcept {
        xor_padding_block();
        keccak_f(internal_state);
    }

    // get resulting hash
    constexpr void squeeze(std::span<std::byte> output) noexcept {
        using value_t = keccak::state_1600::value_type;

        static_assert((rate % sizeof(value_t)) == 0u);
        auto r = std::span<const value_t>(internal_state).first(rate / sizeof(value_t));

        // aligned results will be processed here...
        while ((output.size() >= sizeof(value_t))) {
            // if we ran out of `rate` part, we need to squeeze another block
            if (r.empty()) {
                keccak_f(internal_state);
                r = std::span<const value_t>(internal_state).first(rate / sizeof(value_t));
            }

            // look at current to process
            const value_t current = r.front();
            const auto part = output.first<sizeof(value_t)>();

            // convert
            unwrap_littleendian_number<value_t> { part } = current;

            // move to next
            r = r.subspan(1u);
            output = output.subspan(sizeof(value_t));
        }

        // unaligned result is here
        if (!output.empty()) {
            // if we ran out of `rate` part, we need to squeeze another block
            if (r.empty()) {
                keccak_f(internal_state);
                r = std::span<const value_t>(internal_state).first(rate / sizeof(value_t));
            }

            const value_t current = r.front();

            // convert
            std::array<std::byte, sizeof(value_t)> tmp;
            unwrap_littleendian_number<value_t> { tmp } = current;
            CTHASH_ASSERT(tmp.size() > output.size());
            std::copy_n(tmp.data(), output.size(), output.data());
        }
    }

    constexpr void squeeze(digest_span_t output_fixed) noexcept requires((digest_length < rate) && digest_length != 0u) {
        auto output = std::span<std::byte>(output_fixed);

        // we don't need to squeeze anything
        using value_t = keccak::state_1600::value_type;

        static_assert((rate % sizeof(value_t)) == 0u);
        auto r = std::span<const value_t>(internal_state).first(rate / sizeof(value_t));

        // aligned results will be processed here...
        while ((output.size() >= sizeof(value_t))) {
            CTHASH_ASSERT(!r.empty());
            // look at current to process
            const value_t current = r.front();
            const auto part = output.template first<sizeof(value_t)>();

            // convert
            unwrap_littleendian_number<value_t> { part } = current;

            // move to next
            r = r.subspan(1u);
            output = output.subspan(sizeof(value_t));
        }

        if constexpr ((output_fixed.size() % sizeof(value_t)) != 0u) {
            // unaligned result is here
            CTHASH_ASSERT(!output.empty());
            CTHASH_ASSERT(!r.empty());

            const value_t current = r.front();

            // convert
            std::array<std::byte, sizeof(value_t)> tmp;
            unwrap_littleendian_number<value_t> { tmp } = current;
            CTHASH_ASSERT(tmp.size() > output.size());
            std::copy_n(tmp.data(), output.size(), output.data());
        }
    }

    constexpr void final(digest_span_t digest) noexcept requires(digest_length != 0u) {
        final_absorb();
        squeeze(digest);
    }

    constexpr result_t final() noexcept requires(digest_length != 0u) {
        result_t output;
        final(output);
        return output;
    }

    template <size_t N>
    constexpr auto final() noexcept requires(digest_length == 0u) {
        static_assert(N % 8u == 0u, "Only whole bytes are supported!");
        using result_type = typename Config::template variable_digest<N>;
        result_type output;
        final_absorb();
        squeeze(output);
        return output;
    }
};

template <typename Config>
struct keccak_hasher : basic_keccak_hasher<Config> {
    using super = basic_keccak_hasher<Config>;
    using result_t = typename super::result_t;
    using digest_span_t = typename super::digest_span_t;

    constexpr keccak_hasher() noexcept
        : super() {}
    constexpr keccak_hasher(const keccak_hasher &) noexcept = default;
    constexpr keccak_hasher(keccak_hasher &&) noexcept = default;
    constexpr ~keccak_hasher() noexcept = default;

    constexpr keccak_hasher &update(std::span<const std::byte> input) noexcept {
        super::update(input);
        return *this;
    }

    template <convertible_to_byte_span T>
    constexpr keccak_hasher &update(const T &something) noexcept {
        using value_type = typename decltype(std::span(something))::value_type;
        super::update(std::span<const value_type>(something));
        return *this;
    }

    template <one_byte_char CharT>
    constexpr keccak_hasher &update(std::basic_string_view<CharT> in) noexcept {
        super::update(std::span(in.data(), in.size()));
        return *this;
    }

    template <string_literal T>
    constexpr keccak_hasher &update(const T &lit) noexcept {
        super::update(std::span(lit, std::size(lit) - 1u));
        return *this;
    }

    using super::final;
};

} // namespace cthash

#endif
