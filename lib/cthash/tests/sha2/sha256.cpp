#include "../internal/support.hpp"
#include <cthash/sha2/sha256.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;

TEST_CASE("sha256 size") {
    auto h = cthash::sha256 {};
    h.update("aloha");
    REQUIRE(h.size() == 5u);
    h.update("hana");
    REQUIRE(h.size() == 9u);
}

TEST_CASE("sha256 zero staging should be empty") {
    const auto block = array_of_zeros<64>();
    const auto staging = cthash::internal_hasher<cthash::sha256_config>::build_staging(block);

    for (auto val : staging) {
        REQUIRE(val == static_cast<decltype(val)>(0));
    }
}

TEST_CASE("sha256 empty input") {
    const auto block = [] {
        auto r = array_of_zeros<64>();
        r[0] = std::byte { 0b1000'0000 };
        return r;
    }();
    const auto staging = cthash::internal_hasher<cthash::sha256_config>::build_staging(block);

    auto it = staging.begin();

    // from the block
    REQUIRE(*it++ == 0x80000000ul);
    for (int i = 1; i != 16; ++i) {
        REQUIRE(*it++ == 0ull);
    }

    // calculated by staging function
    REQUIRE(staging[16] == 0b10000000000000000000000000000000ul);
    REQUIRE(staging[17] == 0b00000000000000000000000000000000ul);
    REQUIRE(staging[18] == 0b00000000001000000101000000000000ul);
    REQUIRE(staging[19] == 0b00000000000000000000000000000000ul);
    REQUIRE(staging[20] == 0b00100010000000000000100000000000ul);
    REQUIRE(staging[21] == 0b00000000000000000000000000000000ul);
    REQUIRE(staging[22] == 0b00000101000010001001010101000010ul);
    REQUIRE(staging[23] == 0b10000000000000000000000000000000ul);
    REQUIRE(staging[24] == 0b01011000000010000000000000000000ul);
    REQUIRE(staging[25] == 0b00000000010000001010000000000000ul);
    REQUIRE(staging[26] == 0b00000000000101100010010100000101ul);
    REQUIRE(staging[27] == 0b01100110000000000001100000000000ul);
    REQUIRE(staging[28] == 0b11010110001000100010010110000000ul);
    REQUIRE(staging[29] == 0b00010100001000100101010100001000ul);
    REQUIRE(staging[30] == 0b11010110010001011111100101011100ul);
    REQUIRE(staging[31] == 0b11001001001010000010000000000000ul);
    REQUIRE(staging[32] == 0b11000011111100010000000010010100ul);
    REQUIRE(staging[33] == 0b00101000010011001010011101100110ul);
    REQUIRE(staging[34] == 0b00000110100010000110110111000110ul);
    REQUIRE(staging[35] == 0b10100011011110111111000100010110ul);
    REQUIRE(staging[36] == 0b01110001011111001011111010010110ul);
    REQUIRE(staging[37] == 0b11111110110000101101011101001010ul);
    REQUIRE(staging[38] == 0b10100111101101100111111100000000ul);
    REQUIRE(staging[39] == 0b10000001000101011001011010100010ul);
    REQUIRE(staging[40] == 0b10011000101001101110011101101000ul);
    REQUIRE(staging[41] == 0b00000011101100100000110010000010ul);
    REQUIRE(staging[42] == 0b01011101000111011010011111001001ul);
    REQUIRE(staging[43] == 0b10110001010101101011100100110101ul);
    REQUIRE(staging[44] == 0b11000011110111011100101000010001ul);
    REQUIRE(staging[45] == 0b00100100100111000001000001111111ul);
    REQUIRE(staging[46] == 0b11000100100011010010010011101111ul);
    REQUIRE(staging[47] == 0b01011101111001010100110000110000ul);
    REQUIRE(staging[48] == 0b11011110111111101100111001100101ul);
    REQUIRE(staging[49] == 0b00101100101000010100100000001101ul);
    REQUIRE(staging[50] == 0b00111100000101010011001100101100ul);
    REQUIRE(staging[51] == 0b00000001110011101100100110101101ul);
    REQUIRE(staging[52] == 0b00010110000011001100110011010000ul);
    REQUIRE(staging[53] == 0b00001011101011001101101010011000ul);
    REQUIRE(staging[54] == 0b00110110000110111000111111100000ul);
    REQUIRE(staging[55] == 0b11010010001100100000101110100110ul);
    REQUIRE(staging[56] == 0b00000010100110110111000000000111ul);
    REQUIRE(staging[57] == 0b01110101010001100101100001111100ul);
    REQUIRE(staging[58] == 0b00000111111101010100111100111001ul);
    REQUIRE(staging[59] == 0b11111000000010001101110111000011ul);
    REQUIRE(staging[60] == 0b11011100110010100111011000001000ul);
    REQUIRE(staging[61] == 0b01011110010000100111000110001000ul);
    REQUIRE(staging[62] == 0b01000100101111001110110001011101ul);
    REQUIRE(staging[63] == 0b00111011010111101100010010011011ul);

    REQUIRE(64 == staging.size());
}

TEST_CASE("sha256 basics (constexpr and runtime)") {
    constexpr auto v1 = cthash::sha256 {}.update("").final();
    auto v1r = cthash::sha256 {}.update(runtime_pass("")).final();
    REQUIRE(v1 == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"_sha256);
    REQUIRE(v1 == v1r);

    constexpr auto v2 = cthash::sha256 {}.update("hana").final();
    auto v2r = cthash::sha256 {}.update(runtime_pass("hana")).final();
    REQUIRE(v2 == "599ba25a0d7c7d671bee93172ca7e272fc87f0c0e02e44df9e9436819067ea28"_sha256);
    REQUIRE(v2 == v2r);

    constexpr auto v3 = cthash::sha256 {}.update(array_of_zeros<96>()).final();
    auto v3r = cthash::sha256 {}.update(runtime_pass(array_of_zeros<96>())).final();
    REQUIRE(v3 == "2ea9ab9198d1638007400cd2c3bef1cc745b864b76011a0e1bc52180ac6452d4"_sha256);
    REQUIRE(v3 == v3r);

    constexpr auto v4 = cthash::sha256 {}.update(array_of_zeros<120>()).final();
    auto v4r = cthash::sha256 {}.update(runtime_pass(array_of_zeros<120>())).final();
    auto v4b = cthash::sha256 {}.update(runtime_pass(array_of_zeros<120, char>())).final();
    REQUIRE(v4 == "6edd9f6f9cc92cded36e6c4a580933f9c9f1b90562b46903b806f21902a1a54f"_sha256);
    REQUIRE(v4 == v4r);
    REQUIRE(v4 == v4b);

    constexpr auto v5 = cthash::sha256 {}.update(array_of_zeros<128>()).final();
    auto v5r = cthash::sha256 {}.update(runtime_pass(array_of_zeros<128>())).final();
    REQUIRE(v5 == "38723a2e5e8a17aa7950dc008209944e898f69a7bd10a23c839d341e935fd5ca"_sha256);
    REQUIRE(v5 == v5r);
}

TEST_CASE("sha256 long hash over 512MB", "[.long]") {
    cthash::sha256 h {};
    for (int i = 0; i != 512 * 1024; ++i) {
        h.update(array_of_zeros<1024>());
    }
    REQUIRE(h.size() == 512u * 1024u * 1024u);
    const auto r = h.final();

    REQUIRE(r == "9acca8e8c22201155389f65abbf6bc9723edc7384ead80503839f49dcc56d767"_sha256);
}
