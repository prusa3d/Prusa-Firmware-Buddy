#include "../internal/support.hpp"
#include <cthash/sha2/sha224.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;

TEST_CASE("sha224 basics") {
    constexpr auto v1 = cthash::sha224 {}.update("").final();
    auto v1r = cthash::sha224 {}.update(runtime_pass("")).final();
    REQUIRE(v1 == "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"_sha224);
    REQUIRE(v1 == v1r);

    constexpr auto v2 = cthash::sha224 {}.update("hana").final();
    auto v2r = cthash::sha224 {}.update(runtime_pass("hana")).final();
    REQUIRE(v2 == "a814c3122b1a3f2402bbcd0faffe28a9a7c24d389af78b596c752684"_sha224);
    REQUIRE(v2 == v2r);

    constexpr auto v3 = cthash::sha224 {}.update(array_of_zeros<32>()).final();
    auto v3r = cthash::sha224 {}.update(runtime_pass(array_of_zeros<32>())).final();
    REQUIRE(v3 == "b338c76bcffa1a0b3ead8de58dfbff47b63ab1150e10d8f17f2bafdf"_sha224);
    REQUIRE(v3 == v3r);

    constexpr auto v4 = cthash::sha224 {}.update(array_of_zeros<64>()).final();
    auto v4r = cthash::sha224 {}.update(runtime_pass(array_of_zeros<64>())).final();
    REQUIRE(v4 == "750d81a39c18d3ce27ff3e5ece30b0088f12d8fd0450fe435326294b"_sha224);
    REQUIRE(v4 == v4r);

    constexpr auto v5 = cthash::sha224 {}.update(array_of_zeros<120>()).final();
    auto v5r = cthash::sha224 {}.update(runtime_pass(array_of_zeros<120>())).final();
    REQUIRE(v5 == "83438028e7817c90b386a11c9a4e051f821b37c818bb4b5c08279584"_sha224);
    REQUIRE(v5 == v5r);

    constexpr auto v6 = cthash::sha224 {}.update(array_of_zeros<128>()).final();
    auto v6r = cthash::sha224 {}.update(runtime_pass(array_of_zeros<128>())).final();
    REQUIRE(v6 == "2fbd823ebcd9909d265827e4bce793a4fc572e3f39c7c3dd67749f3e"_sha224);
    REQUIRE(v6 == v6r);

    constexpr auto v7 = cthash::sha224 {}.update(array_of_zeros<512>()).final();
    auto v7r = cthash::sha224 {}.update(runtime_pass(array_of_zeros<512, char>())).final();
    REQUIRE(v7 == "4026dd4dbeb4d8a951dfd9a592897f46203ebe2d99c4a8837aa3afc9"_sha224);
    REQUIRE(v7 == v7r);
}
