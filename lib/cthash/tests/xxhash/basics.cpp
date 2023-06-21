#include "../internal/support.hpp"
#include <cthash/xxhash.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;

TEST_CASE("xxhash update", "[xxh-basic]") {
    const auto arr = array_of<32>(std::byte(0xEF));

    cthash::xxhash<32> h {};

    // IV
    REQUIRE(h.seed == 0u);
    REQUIRE(h.internal_state[0] == cthash::xxhash_types<32>::primes[0] + cthash::xxhash_types<32>::primes[1]);
    REQUIRE(h.internal_state[1] == cthash::xxhash_types<32>::primes[1]);
    REQUIRE(h.internal_state[2] == 0u);
    REQUIRE(h.internal_state[3] == 0u - cthash::xxhash_types<32>::primes[0]);

    REQUIRE(h.length == 0u);
    REQUIRE(h.buffer_usage() == 0u);

    // process
    h.update(std::span(arr).first(1));
    REQUIRE(h.length == 1u);
    REQUIRE(h.buffer_usage() == 1u);

    REQUIRE(h.buffer.size() == 16u);
    REQUIRE(unsigned(h.buffer[0]) == 0xEFu);
    for (int i = 1; i != h.buffer.size(); ++i) {
        REQUIRE(unsigned(h.buffer[static_cast<size_t>(i)]) == 0u);
    }

    h.update(std::span(arr).first(1));
    REQUIRE(h.length == 2u);
    REQUIRE(h.buffer_usage() == 2u);

    REQUIRE(unsigned(h.buffer[0]) == 0xEFu);
    REQUIRE(unsigned(h.buffer[1]) == 0xEFu);
    for (int i = 2; i != h.buffer.size(); ++i) {
        REQUIRE(unsigned(h.buffer[static_cast<size_t>(i)]) == 0u);
    }

    h.update(std::span(arr).first(14));
    REQUIRE(h.length == 16u);
    for (int i = 0; i != h.buffer.size(); ++i) {
        REQUIRE(unsigned(h.buffer[static_cast<size_t>(i)]) == 0xEFu);
    }
    REQUIRE(h.buffer_usage() == 0u);

    // buffer is untouched
    REQUIRE(cthash::get_le_number_from<uint32_t, 0>(std::span<const std::byte>(h.buffer)) == 0xEFEFEFEFu);
    REQUIRE(cthash::get_le_number_from<uint32_t, 1>(std::span<const std::byte>(h.buffer)) == 0xEFEFEFEFu);
    REQUIRE(cthash::get_le_number_from<uint32_t, 2>(std::span<const std::byte>(h.buffer)) == 0xEFEFEFEFu);
    REQUIRE(cthash::get_le_number_from<uint32_t, 3>(std::span<const std::byte>(h.buffer)) == 0xEFEFEFEFu);

    // first step should be called here, so internal state is different than it was
    REQUIRE(h.internal_state[0] == 0x659373acu);
    REQUIRE(h.internal_state[1] == 0x6015b815u);
    REQUIRE(h.internal_state[2] == 0xd21cf068u);
    REQUIRE(h.internal_state[3] == 0xcc9f34d1u);

    // additional block
    h.update(std::span(arr).first(16));
    REQUIRE(h.length == 32u);
    for (int i = 0; i != h.buffer.size(); ++i) {
        REQUIRE(unsigned(h.buffer[static_cast<size_t>(i)]) == 0xEFu);
    }
    REQUIRE(h.buffer_usage() == 0u);

    REQUIRE(h.internal_state[0] == 0xd721597au);
    REQUIRE(h.internal_state[1] == 0xceb0cfcau);
    REQUIRE(h.internal_state[2] == 0x59c4a3bbu);
    REQUIRE(h.internal_state[3] == 0x51541a0bu);

    SECTION("stop now") {
        const auto c = h.converge_conditionaly();
        REQUIRE(c == 0xb9139348u);
        const auto len = c + h.length;
        REQUIRE(len == 0xb9139368u);
        const auto cons = decltype(h)::config::consume_remaining(len, std::span<const std::byte>(h.buffer).first(h.buffer_usage()));
        REQUIRE(cons == 0xb9139368u);
        const auto aval = decltype(h)::config::avalanche(cons);
        REQUIRE(aval == 0xbea54e50u);
        REQUIRE(h.final() == "bea54e50"_xxh32);
    }

    SECTION("continue") {
        // additional block 2
        const auto arr2 = array_of<32>(std::byte(0xAB));
        REQUIRE(h.buffer_usage() == 0u);
        h.update(std::span(arr2).first(20));
        REQUIRE(h.length == 52u);
        REQUIRE(h.buffer_usage() == 4u);
        REQUIRE(unsigned(h.buffer[0]) == 0xABu);
        REQUIRE(unsigned(h.buffer[1]) == 0xABu);
        REQUIRE(unsigned(h.buffer[2]) == 0xABu);
        REQUIRE(unsigned(h.buffer[3]) == 0xABu);

        REQUIRE(h.internal_state[0] == 0x3f316f5bu);
        REQUIRE(h.internal_state[1] == 0xf45916adu);
        REQUIRE(h.internal_state[2] == 0x73c86d6fu);
        REQUIRE(h.internal_state[3] == 0x28f014c1u);

        SECTION("finish") {
            const auto c = h.converge_conditionaly();
            REQUIRE(c == 0x84c9d0acu);
            const auto len = c + h.length;
            REQUIRE(len == 0x84c9d0e0u);
            const auto cons = decltype(h)::config::consume_remaining(len, std::span<const std::byte>(h.buffer).first(h.buffer_usage()));
            REQUIRE(cons == 0xeab6d685u);
            const auto aval = decltype(h)::config::avalanche(cons);
            REQUIRE(aval == 0x78c3c388u);
            REQUIRE(h.final() == "78c3c388"_xxh32);
        }

        // const auto iv3 = h.internal_state;
    }
}

TEST_CASE("xxhash_fnc basics", "[xxh]") {
    SECTION("empty string") {
        const std::string_view empty = "";

        // REQUIRE(cthash::xxhash<32>{}.update(empty).final() == "02cc5d05"_xxh32);
        // REQUIRE(cthash::xxhash<64>{}.update(empty).final() == "ef46db3751d8e999"_xxh64);

        REQUIRE(cthash::simple<cthash::xxhash<32>>(empty, 0u) == "02cc5d05"_xxh32);
        REQUIRE(cthash::simple<cthash::xxhash<64>>(empty, 0u) == "ef46db3751d8e999"_xxh64);

        REQUIRE(cthash::simple<cthash::xxhash<32>>(empty, 42u) == "d5be6eb8"_xxh32);
        REQUIRE(cthash::simple<cthash::xxhash<64>>(empty, 42u) == "98b1582b0977e704"_xxh64);
    }

    SECTION("empty string") {
        const std::string_view in = "hello there";

        REQUIRE(cthash::simple<cthash::xxhash<32>>(in, 0u) == "371c3e72"_xxh32);
        REQUIRE(cthash::simple<cthash::xxhash<64>>(in, 0u) == "08f296af889a203c"_xxh64);

        REQUIRE(cthash::simple<cthash::xxhash<32>>(in, 42u) == "4a90b3c2"_xxh32);
        REQUIRE(cthash::simple<cthash::xxhash<64>>(in, 42u) == "1a910e9618a06c28"_xxh64);
    }

    SECTION("longer string") {
        const std::string_view in = "hello there, from somehow long string! really this should be enought :)";

        REQUIRE(cthash::simple<cthash::xxhash<32>>(in, 0u) == "2daeaacd"_xxh32);
        REQUIRE(cthash::simple<cthash::xxhash<64>>(in, 0u) == "4f6f14232e5ab579"_xxh64);

        REQUIRE(cthash::simple<cthash::xxhash<32>>(in, 42u) == "b66e8e53"_xxh32);
        REQUIRE(cthash::simple<cthash::xxhash<64>>(in, 42u) == "62eee52b8dbf7af9"_xxh64);
    }

    SECTION("longer string via multiple update") {
        cthash::xxhash<32> h1 {};
        cthash::xxhash<64> h2 {};

        h1.update("hello there, ").update("from somehow long string!").update(" really this should be enought :)");
        h2.update("hello there, ").update("from somehow long string!").update(" really this should be enought :)");

        SECTION("32 bits") {
            REQUIRE(h1.internal_state[0] == 0x2c7844f7u);
            REQUIRE(h1.internal_state[1] == 0xdf8adc4fu);
            REQUIRE(h1.internal_state[2] == 0x502598e7u);
            REQUIRE(h1.internal_state[3] == 0x032d3506u);

            REQUIRE(h1.final() == "2daeaacd"_xxh32);
        }

        SECTION("64 bits") {
            REQUIRE(h2.final() == "4f6f14232e5ab579"_xxh64);
        }
    }

    SECTION("longer string") {
        const std::string_view lit = "hello there, from somehow long string! really this should be enought :)";

        cthash::xxhash<32> h1 {};
        cthash::xxhash<64> h2 {};

        for (int i = 0; i != 1000; ++i) {
            h1.update(lit);
            h2.update(lit);
        }

        SECTION("32 bits") {
            REQUIRE(h1.final() == "a7d7a81d"_xxh32);
        }

        SECTION("64 bits") {
            REQUIRE(h2.final() == "bd6f22acc408272d"_xxh64);
        }
    }
}

TEST_CASE("xxhash_fnc benchmarks", "[xxh]") {
    auto val = std::string(10u * 1024u * 1024u, '*');

    BENCHMARK("really long string (32bit) (10MB)") {
        return cthash::simple<cthash::xxhash<32>>(std::string_view(val));
    };

    BENCHMARK("really long string (64bit) (10MB)") {
        return cthash::simple<cthash::xxhash<64>>(std::string_view(val));
    };

    auto val2 = std::string(1024u * 1024u * 1024u, '*');

    BENCHMARK("really long string (32bit) (1GB)") {
        return cthash::simple<cthash::xxhash<32>>(std::string_view(val2));
    };

    BENCHMARK("really long string (64bit) (1GB)") {
        return cthash::simple<cthash::xxhash<64>>(std::string_view(val2));
    };
}
