#ifndef CTHASH_SHA3_SHAKE256_HPP
#define CTHASH_SHA3_SHAKE256_HPP

#include "common.hpp"

namespace cthash {

template <size_t N>
struct shake256_value;

struct shake256_config {
    template <size_t N>
    using variable_digest = shake256_value<N>;

    static constexpr size_t digest_length_bit = 0;

    static constexpr size_t capacity_bit = 512;
    static constexpr size_t rate_bit = 1088;

    static constexpr auto suffix = keccak_suffix(4, 0b0000'1111u); // in reverse
};

static_assert((shake256_config::capacity_bit + shake256_config::rate_bit) == 1600u);

using shake256 = cthash::keccak_hasher<shake256_config>;

template <size_t N>
struct shake256_value : tagged_hash_value<variable_bit_length_tag<N, shake256_config>> {
    static_assert(N > 0);
    using super = tagged_hash_value<variable_bit_length_tag<N, shake256_config>>;
    using super::super;

    template <typename CharT>
    explicit constexpr shake256_value(const internal::fixed_string<CharT, N / 8u> &in) noexcept
        : super { in } {}

    template <size_t K>
    constexpr friend bool operator==(const shake256_value &lhs, const shake256_value<K> &rhs) noexcept {
        constexpr auto smallest_n = std::min(N, K);
        const auto lhs_view = std::span<const std::byte, smallest_n / 8u>(lhs.data(), smallest_n / 8u);
        const auto rhs_view = std::span<const std::byte, smallest_n / 8u>(rhs.data(), smallest_n / 8u);
        return std::equal(lhs_view.begin(), lhs_view.end(), rhs_view.begin());
    }

    template <size_t K>
    constexpr friend auto operator<=>(const shake256_value &lhs, const shake256_value<K> &rhs) noexcept {
        constexpr auto smallest_n = std::min(N, K);
        return internal::threeway_compare_of_same_size(lhs.data(), rhs.data(), smallest_n / 8u);
    }
};

template <typename CharT, size_t N>
requires(N % 2 == 0)
    shake256_value(const internal::fixed_string<CharT, N> &)
        ->shake256_value<N * 4u>;

namespace literals {

    template <internal::fixed_string Value>
    consteval auto operator""_shake256() {
        return shake256_value(Value);
    }

} // namespace literals

} // namespace cthash

#endif
