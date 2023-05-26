#ifndef CTHASH_SHA3_SHAKE128_HPP
#define CTHASH_SHA3_SHAKE128_HPP

#include "common.hpp"

namespace cthash {

template <size_t N>
struct shake128_value;

struct shake128_config {
    template <size_t N>
    using variable_digest = shake128_value<N>;

    static constexpr size_t digest_length_bit = 0;

    static constexpr size_t capacity_bit = 256;
    static constexpr size_t rate_bit = 1344;

    static constexpr auto suffix = keccak_suffix(4, 0b0000'1111u); // in reverse
};

static_assert((shake128_config::capacity_bit + shake128_config::rate_bit) == 1600u);

using shake128 = cthash::keccak_hasher<shake128_config>;

template <size_t N>
struct shake128_value : tagged_hash_value<variable_bit_length_tag<N, shake128_config>> {
    static_assert(N > 0);
    using super = tagged_hash_value<variable_bit_length_tag<N, shake128_config>>;
    using super::super;

    template <typename CharT>
    explicit constexpr shake128_value(const internal::fixed_string<CharT, N / 8u> &in) noexcept
        : super { in } {}

    template <size_t K>
    constexpr friend bool operator==(const shake128_value &lhs, const shake128_value<K> &rhs) noexcept {
        static_assert(K > 0);
        constexpr auto smallest_n = std::min(N, K);
        const auto lhs_view = std::span<const std::byte, smallest_n / 8u>(lhs.data(), smallest_n / 8u);
        const auto rhs_view = std::span<const std::byte, smallest_n / 8u>(rhs.data(), smallest_n / 8u);
        return std::equal(lhs_view.begin(), lhs_view.end(), rhs_view.begin());
    }

    template <size_t K>
    constexpr friend auto operator<=>(const shake128_value &lhs, const shake128_value<K> &rhs) noexcept {
        static_assert(K > 0);
        constexpr auto smallest_n = std::min(N, K);
        return internal::threeway_compare_of_same_size(lhs.data(), rhs.data(), smallest_n / 8u);
    }
};

template <typename CharT, size_t N>
requires(N % 2 == 0)
    shake128_value(const internal::fixed_string<CharT, N> &)
        ->shake128_value<N * 4u>;

namespace literals {

    template <internal::fixed_string Value>
    consteval auto operator""_shake128() {
        return shake128_value(Value);
    }

} // namespace literals

} // namespace cthash

#endif
