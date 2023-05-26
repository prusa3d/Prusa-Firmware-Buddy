#ifndef CTHASH_SHA2_SHA256_HPP
#define CTHASH_SHA2_SHA256_HPP

#include "common.hpp"

namespace cthash {

struct sha256_config {
    using length_type = uint64_t;
    static constexpr size_t length_size_bits = 64;

    static constexpr size_t block_bits = 512u;

    static constexpr auto initial_values = std::array<uint32_t, 8> { 0x6a09e667ul, 0xbb67ae85ul, 0x3c6ef372ul, 0xa54ff53aul, 0x510e527ful, 0x9b05688cul, 0x1f83d9abul, 0x5be0cd19ul };

    // staging sigmas
    [[gnu::always_inline]] static constexpr auto sigma_0(uint32_t w_15) noexcept -> uint32_t {
        return std::rotr(w_15, 7u) xor std::rotr(w_15, 18u) xor (w_15 >> 3u);
    }

    [[gnu::always_inline]] static constexpr auto sigma_1(uint32_t w_2) noexcept -> uint32_t {
        return std::rotr(w_2, 17u) xor std::rotr(w_2, 19u) xor (w_2 >> 10u);
    }

    // rounds constants...
    static constexpr auto constants = std::array<uint32_t, 64> {
        0x428a2f98ul, 0x71374491ul, 0xb5c0fbcful, 0xe9b5dba5ul, 0x3956c25bul, 0x59f111f1ul, 0x923f82a4ul, 0xab1c5ed5ul,
        0xd807aa98ul, 0x12835b01ul, 0x243185beul, 0x550c7dc3ul, 0x72be5d74ul, 0x80deb1feul, 0x9bdc06a7ul, 0xc19bf174ul,
        0xe49b69c1ul, 0xefbe4786ul, 0x0fc19dc6ul, 0x240ca1ccul, 0x2de92c6ful, 0x4a7484aaul, 0x5cb0a9dcul, 0x76f988daul,
        0x983e5152ul, 0xa831c66dul, 0xb00327c8ul, 0xbf597fc7ul, 0xc6e00bf3ul, 0xd5a79147ul, 0x06ca6351ul, 0x14292967ul,
        0x27b70a85ul, 0x2e1b2138ul, 0x4d2c6dfcul, 0x53380d13ul, 0x650a7354ul, 0x766a0abbul, 0x81c2c92eul, 0x92722c85ul,
        0xa2bfe8a1ul, 0xa81a664bul, 0xc24b8b70ul, 0xc76c51a3ul, 0xd192e819ul, 0xd6990624ul, 0xf40e3585ul, 0x106aa070ul,
        0x19a4c116ul, 0x1e376c08ul, 0x2748774cul, 0x34b0bcb5ul, 0x391c0cb3ul, 0x4ed8aa4aul, 0x5b9cca4ful, 0x682e6ff3ul,
        0x748f82eeul, 0x78a5636ful, 0x84c87814ul, 0x8cc70208ul, 0x90befffaul, 0xa4506cebul, 0xbef9a3f7ul, 0xc67178f2ul
    };

    // rounds sums
    [[gnu::always_inline]] static constexpr auto sum_a(uint32_t a) noexcept -> uint32_t {
        return std::rotr(a, 2u) xor std::rotr(a, 13u) xor std::rotr(a, 22u);
    }

    [[gnu::always_inline]] static constexpr auto sum_e(uint32_t e) noexcept -> uint32_t {
        return std::rotr(e, 6u) xor std::rotr(e, 11u) xor std::rotr(e, 25u);
    }

    // rounds
    [[gnu::always_inline]] static constexpr void rounds(std::span<const uint32_t, 64> w, std::array<uint32_t, 8> &state) noexcept {
        return sha2::rounds<sha256_config>(w, state);
    }
};

static_assert(not cthash::internal::digest_length_provided<sha256_config>);
static_assert(cthash::internal::digest_bytes_length_of<sha256_config> == 32u);

using sha256 = hasher<sha256_config>;
using sha256_value = tagged_hash_value<sha256_config>;

namespace literals {

    template <internal::fixed_string Value>
    consteval auto operator""_sha256() {
        return sha256_value(Value);
    }

} // namespace literals

} // namespace cthash

#endif
