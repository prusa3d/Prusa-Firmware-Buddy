#include "internal/support.hpp"
#include <cthash/sha3/keccak.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("keccakF") {
    auto s = cthash::keccak::state_1600 {};
    cthash::keccak::keccak_f(s);
    const auto expected = cthash::keccak::state_1600 {
        0xf1258f7940e1dde7ull,
        0x84d5ccf933c0478aull,
        0xd598261ea65aa9eeull,
        0xbd1547306f80494dull,
        0x8b284e056253d057ull,
        0xff97a42d7f8e6fd4ull,
        0x90fee5a0a44647c4ull,
        0x8c5bda0cd6192e76ull,
        0xad30a6f71b19059cull,
        0x30935ab7d08ffc64ull,
        0xeb5aa93f2317d635ull,
        0xa9a6e6260d712103ull,
        0x81a57c16dbcf555full,
        0x43b831cd0347c826ull,
        0x1f22f1a11a5569full,
        0x05e5635a21d9ae61ull,
        0x64befef28cc970f2ull,
        0x613670957bc46611ull,
        0xb87c5a554fd00ecbull,
        0x8c3ee88a1ccf32c8ull,
        0x940c7922ae3a2614ull,
        0x1841f924a2c509e4ull,
        0x16f53526e70465c2ull,
        0x75f644e97f30a13bull,
        0xeaf1ff7b5ceca249ull,
    };
    REQUIRE(s == expected);
}
