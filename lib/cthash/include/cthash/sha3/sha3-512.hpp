#ifndef CTHASH_SHA3_SHA3_512_HPP
#define CTHASH_SHA3_SHA3_512_HPP

#include "common.hpp"

namespace cthash {

struct sha3_512_config {
    static constexpr size_t digest_length_bit = 512u;
    static constexpr size_t capacity_bit = digest_length_bit * 2u;
    static constexpr size_t rate_bit = 1600u - capacity_bit;

    static constexpr auto suffix = keccak_suffix(2, 0b0000'0010u); // in reverse
};

static_assert((sha3_512_config::capacity_bit + sha3_512_config::rate_bit) == 1600u);

using sha3_512 = cthash::keccak_hasher<sha3_512_config>;
using sha3_512_value = tagged_hash_value<sha3_512_config>;

namespace literals {

    template <internal::fixed_string Value>
    consteval auto operator""_sha3_512() {
        return sha3_512_value(Value);
    }

} // namespace literals

} // namespace cthash

#endif
