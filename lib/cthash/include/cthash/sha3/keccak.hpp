#ifndef CTHASH_SHA3_KECCAK_HPP
#define CTHASH_SHA3_KECCAK_HPP

#include <array>
#include <bit>
#include <span>
#include <type_traits>
#include <utility>
#include <concepts>
#include <cstdint>

namespace cthash::keccak {

// inspired by tiny-keccak (https://github.com/debris/tiny-keccak from Marek Kotewicz)

static constexpr auto rho = std::array<uint8_t, 24> { 1u, 3u, 6u, 10u, 15u, 21u, 28u, 36u, 45u, 55u, 2u, 14u, 27u, 41u, 56u, 8u, 25u, 43u, 62u, 18u, 39u, 61u, 20u, 44u };

static constexpr auto pi = std::array<uint8_t, 24> { 10u, 7u, 11u, 17u, 18u, 3u, 5u, 16u, 8u, 21u, 24u, 4u, 15u, 23u, 19u, 13u, 12u, 2u, 20u, 14u, 22u, 9u, 6u, 1u };

static constexpr auto rc = std::array<uint64_t, 24> { 0x1ULL, 0x8082ULL, 0x800000000000808aULL, 0x8000000080008000ULL, 0x808bULL, 0x80000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL, 0x8aULL, 0x88ULL, 0x80008009ULL, 0x8000000aULL, 0x8000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL, 0x800aULL, 0x800000008000000aULL, 0x8000000080008081ULL, 0x8000000000008080ULL, 0x80000001ULL, 0x8000000080008008ULL };

struct state_1600 : std::array<uint64_t, (5u * 5u)> {};

struct state_1600_ref : std::span<uint64_t, (5u * 5u)> {
    using super = std::span<uint64_t, (5u * 5u)>;
    using super::super;
};

[[gnu::always_inline, gnu::flatten]] constexpr void theta(state_1600_ref state) noexcept {
    // xor of columns
    const auto b = std::array<uint64_t, 5> {
        state[0] xor state[5] xor state[10] xor state[15] xor state[20],
        state[1] xor state[6] xor state[11] xor state[16] xor state[21],
        state[2] xor state[7] xor state[12] xor state[17] xor state[22],
        state[3] xor state[8] xor state[13] xor state[18] xor state[23],
        state[4] xor state[9] xor state[14] xor state[19] xor state[24],
    };

    const auto tmp = std::array<uint64_t, 5> {
        b[4] xor std::rotl(b[1], 1),
        b[0] xor std::rotl(b[2], 1),
        b[1] xor std::rotl(b[3], 1),
        b[2] xor std::rotl(b[4], 1),
        b[3] xor std::rotl(b[0], 1),
    };

    [&]<size_t... Idx>(std::index_sequence<Idx...>) {
        ((state[Idx] ^= tmp[Idx % 5u]), ...);
    }
    (std::make_index_sequence<25>());
}

[[gnu::always_inline, gnu::flatten]] constexpr void rho_pi(state_1600_ref state) noexcept {
    uint64_t tmp = state[1];

    [&]<size_t... Idx>(std::index_sequence<Idx...>) {
        ((state[pi[Idx]] = std::rotl(std::exchange(tmp, state[pi[Idx]]), rho[Idx])), ...);
    }
    (std::make_index_sequence<24>());
}

[[gnu::always_inline, gnu::flatten]] constexpr void chi(state_1600_ref state) noexcept {
    constexpr auto chi_helper = [](std::span<uint64_t, 5> row) {
        const auto b = std::array<uint64_t, 5> { row[0], row[1], row[2], row[3], row[4] };

        row[0] = b[0] xor ((~b[1]) bitand b[2]);
        row[1] = b[1] xor ((~b[2]) bitand b[3]);
        row[2] = b[2] xor ((~b[3]) bitand b[4]);
        row[3] = b[3] xor ((~b[4]) bitand b[0]);
        row[4] = b[4] xor ((~b[0]) bitand b[1]);
    };

    chi_helper(state.subspan<0>().first<5>());
    chi_helper(state.subspan<5>().first<5>());
    chi_helper(state.subspan<10>().first<5>());
    chi_helper(state.subspan<15>().first<5>());
    chi_helper(state.subspan<20>().first<5>());
}

[[gnu::flatten]] constexpr void keccak_f(state_1600 &state) noexcept {
    // rounds
    for (int i = 0; i != 24; ++i) {
        // theta (xor each column together)
        theta(state);
        rho_pi(state);
        chi(state);
        state[0] ^= rc[static_cast<size_t>(i)];
    };
}

} // namespace cthash::keccak

#endif
