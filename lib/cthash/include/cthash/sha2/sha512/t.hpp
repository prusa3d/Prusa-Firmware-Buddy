#ifndef CTHASH_SHA2_SHA512_T_HPP
#define CTHASH_SHA2_SHA512_T_HPP

#include "../sha512.hpp"

namespace cthash {

namespace sha256t_support {

    static consteval size_t width_of_decimal(unsigned t) {
        if (t < 10u) {
            return 1u;
        } else if (t < 100u) {
            return 2u;
        } else if (t < 1000u) {
            return 3u;
        } else {
            throw "we don't support more than three digits!";
        }
    }

    template <unsigned Width>
    static consteval auto generate_signature(unsigned t) {
        const char a = '0' + static_cast<char>((t / 100u) % 10u);
        const char b = '0' + static_cast<char>((t / 10u) % 10u);
        const char c = '0' + static_cast<char>((t / 1u) % 10u);

        if constexpr (Width == 1) {
            return std::array<char, Width + 8u> { 'S', 'H', 'A', '-', '5', '1', '2', '/', c };
        } else if constexpr (Width == 2) {
            return std::array<char, Width + 8u> { 'S', 'H', 'A', '-', '5', '1', '2', '/', b, c };
        } else if constexpr (Width == 3) {
            return std::array<char, Width + 8u> { 'S', 'H', 'A', '-', '5', '1', '2', '/', a, b, c };
        } else {
            throw "we don't support greater width than 3";
        }
    }

} // namespace sha256t_support

static consteval auto calculate_sha512t_iv(std::span<const char> in) {
    auto sha512hasher = internal_hasher<sha512_config> {};

    // modify IV
    for (auto &val : sha512hasher.hash) {
        val = val xor 0xa5a5a5a5a5a5a5a5ull;
    }

    sha512hasher.update_to_buffer_and_process(in);
    sha512hasher.finalize();
    return sha512hasher.hash;
}

template <size_t T>
constexpr auto signature_for_sha512t = sha256t_support::generate_signature<sha256t_support::width_of_decimal(T)>(T);
template <size_t T>
constexpr auto iv_for_sha512t = calculate_sha512t_iv(signature_for_sha512t<T>);

template <unsigned T>
struct sha512t_config : sha512_config {
    static_assert(T % 8u == 0u, "only hashes aligned to bytes are supported");
    static_assert(T != 384u, "sha-512/384 is not allowed, use sha-384 instead");
    static_assert(T <= 512u, "T can't be larger than 512");
    static_assert(T != 0u, "T can't be zero");

    static constexpr size_t digest_length = T / 8u;

    static constexpr std::array<uint64_t, 8> initial_values = iv_for_sha512t<T>;
};

static_assert(cthash::internal::digest_length_provided<sha512t_config<224>>);
static_assert(cthash::internal::digest_length_provided<sha512t_config<256>>);
static_assert(cthash::internal::digest_bytes_length_of<sha512t_config<224>> == 28u);
static_assert(cthash::internal::digest_bytes_length_of<sha512t_config<256>> == 32u);

template <unsigned T>
using sha512t = hasher<sha512t_config<T>>;
template <unsigned T>
using sha512t_value = tagged_hash_value<sha512t_config<T>>;

namespace literals {

    template <internal::fixed_string Value>
    consteval auto operator""_sha512_224() {
        return sha512t_value<224>(Value);
    }

    template <internal::fixed_string Value>
    consteval auto operator""_sha512_256() {
        return sha512t_value<256>(Value);
    }

} // namespace literals

} // namespace cthash

#endif
