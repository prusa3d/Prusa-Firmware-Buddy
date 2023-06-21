#ifndef CTHASH_SHA3_SHA3_384_HPP
#define CTHASH_SHA3_SHA3_384_HPP

#include "common.hpp"

namespace cthash {

struct sha3_384_config {
    static constexpr size_t digest_length_bit = 384u;
    static constexpr size_t capacity_bit = digest_length_bit * 2u;
    static constexpr size_t rate_bit = 1600u - capacity_bit;

    static constexpr auto suffix = keccak_suffix(2, 0b0000'0010u); // in reverse
};

static_assert((sha3_384_config::capacity_bit + sha3_384_config::rate_bit) == 1600u);

using sha3_384 = cthash::keccak_hasher<sha3_384_config>;
using sha3_384_value = tagged_hash_value<sha3_384_config>;

namespace literals {

    template <internal::fixed_string Value>
    consteval auto operator""_sha3_384() {
        return sha3_384_value(Value);
    }

} // namespace literals

} // namespace cthash

#endif
