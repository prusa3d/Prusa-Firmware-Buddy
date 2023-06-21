#ifndef CTHASH_SHA2_SHA224_HPP
#define CTHASH_SHA2_SHA224_HPP

#include "sha256.hpp"

namespace cthash {

struct sha224_config : sha256_config {
    // these are only changes against sha256 specification...

    static constexpr size_t digest_length = 28u;

    static constexpr auto initial_values = std::array<uint32_t, 8> { 0xc1059ed8ul, 0x367cd507ul, 0x3070dd17ul, 0xf70e5939ul, 0xffc00b31ul, 0x68581511ul, 0x64f98fa7ul, 0xbefa4fa4ul };
};

static_assert(cthash::internal::digest_length_provided<sha224_config>);
static_assert(cthash::internal::digest_bytes_length_of<sha224_config> == 28u);

using sha224 = hasher<sha224_config>;
using sha224_value = tagged_hash_value<sha224_config>;

namespace literals {

    template <internal::fixed_string Value>
    consteval auto operator""_sha224() {
        return sha224_value(Value);
    }

} // namespace literals

} // namespace cthash

#endif
