#include "../internal/support.hpp"
#include <cthash/sha2/sha512/t.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;
using namespace std::string_literals;

TEST_CASE("sha512/t (init strings)") {
    REQUIRE(to_str(cthash::signature_for_sha512t<8>) == "SHA-512/8"s);
    REQUIRE(to_str(cthash::signature_for_sha512t<16>) == "SHA-512/16"s);
    REQUIRE(to_str(cthash::signature_for_sha512t<128>) == "SHA-512/128"s);
    REQUIRE(to_str(cthash::signature_for_sha512t<224>) == "SHA-512/224"s);
    REQUIRE(to_str(cthash::signature_for_sha512t<256>) == "SHA-512/256"s);
}

TEST_CASE("sha512/224 (iv)") {
    constexpr auto &iv = cthash::sha512t_config<224>::initial_values;
    REQUIRE(iv[0] == 0x8C3D37C819544DA2ull);
    REQUIRE(iv[1] == 0x73E1996689DCD4D6ull);
    REQUIRE(iv[2] == 0x1DFAB7AE32FF9C82ull);
    REQUIRE(iv[3] == 0x679DD514582F9FCFull);
    REQUIRE(iv[4] == 0x0F6D2B697BD44DA8ull);
    REQUIRE(iv[5] == 0x77E36F7304C48942ull);
    REQUIRE(iv[6] == 0x3F9D85A86A1D36C8ull);
    REQUIRE(iv[7] == 0x1112E6AD91D692A1ull);
}

TEST_CASE("sha512/256 (iv)") {
    constexpr auto &iv = cthash::sha512t_config<256>::initial_values;
    REQUIRE(iv[0] == 0x22312194FC2BF72Cull);
    REQUIRE(iv[1] == 0x9F555FA3C84C64C2ull);
    REQUIRE(iv[2] == 0x2393B86B6F53B151ull);
    REQUIRE(iv[3] == 0x963877195940EABDull);
    REQUIRE(iv[4] == 0x96283EE2A88EFFE3ull);
    REQUIRE(iv[5] == 0xBE5E1E2553863992ull);
    REQUIRE(iv[6] == 0x2B0199FC2C85B8AAull);
    REQUIRE(iv[7] == 0x0EB72DDC81C52CA2ull);
}

TEST_CASE("sha512/224 (literals)") {
    [[maybe_unused]] const auto v1 = "750d81a39c18d3ce27ff3e5ece30b0088f12d8fd0450fe435326294b"_sha512_224;
}

TEST_CASE("sha512/224 (basics)") {
    constexpr auto v1 = cthash::sha512t<224> {}.update("").final();
    auto v1r = cthash::sha512t<224> {}.update(runtime_pass("")).final();
    REQUIRE(v1 == "6ed0dd02806fa89e25de060c19d3ac86cabb87d6a0ddd05c333b84f4"_sha512_224);
    REQUIRE(v1 == v1r);

    constexpr auto v2 = cthash::sha512t<224> {}.update("hana").final();
    auto v2r = cthash::sha512t<224> {}.update(runtime_pass("hana")).final();
    REQUIRE(v2 == "53a276b702c0133dcec23f6ec5dc1ad56b224f386fdd57710dc53f9f"_sha512_224);
    REQUIRE(v2 == v2r);

    constexpr auto v3 = cthash::sha512t<224> {}.update(array_of_zeros<32>()).final();
    auto v3r = cthash::sha512t<224> {}.update(runtime_pass(array_of_zeros<32, char>())).final();
    REQUIRE(v3 == "9e7d6080def4e1ccf4aeaac6f7fad008d060a6cf87062038d6166774"_sha512_224);
    REQUIRE(v3 == v3r);

    constexpr auto v4 = cthash::sha512t<224> {}.update(array_of_zeros<64>()).final();
    auto v4r = cthash::sha512t<224> {}.update(runtime_pass(array_of_zeros<64>())).final();
    REQUIRE(v4 == "1319d9b322452068e6f43c0ed3da115fbeccc169711dbbaee2846f90"_sha512_224);
    REQUIRE(v4 == v4r);

    constexpr auto v5 = cthash::sha512t<224> {}.update(array_of_zeros<120>()).final();
    auto v5r = cthash::sha512t<224> {}.update(runtime_pass(array_of_zeros<120, char>())).final();
    REQUIRE(v5 == "d4dfc5c3449b4e3b180d9fda54e1bd86e2c40e2b790db950b4b3d297"_sha512_224);
    REQUIRE(v5 == v5r);

    constexpr auto v6 = cthash::sha512t<224> {}.update(array_of_zeros<128>()).final();
    auto v6r = cthash::sha512t<224> {}.update(runtime_pass(array_of_zeros<128>())).final();
    REQUIRE(v6 == "9ae639d7038fa1946a6f032dc72cb38afb0de1765a82a31621196f44"_sha512_224);
    REQUIRE(v6 == v6r);

    constexpr auto v7 = cthash::sha512t<224> {}.update(array_of_zeros<512>()).final();
    auto v7r = cthash::sha512t<224> {}.update(runtime_pass(array_of_zeros<512>())).final();
    REQUIRE(v7 == "6992572b245cb279973a119cb7f2859e75dff8c5fb9ace89566ae06d"_sha512_224);
    REQUIRE(v7 == v7r);
}

TEST_CASE("sha512/256 (basics)") {
    constexpr auto v1 = cthash::sha512t<256> {}.update("").final();
    auto v1r = cthash::sha512t<256> {}.update(runtime_pass("")).final();
    REQUIRE(v1 == "c672b8d1ef56ed28ab87c3622c5114069bdd3ad7b8f9737498d0c01ecef0967a"_sha512_256);
    REQUIRE(v1 == v1r);

    constexpr auto v2 = cthash::sha512t<256> {}.update("hana").final();
    auto v2r = cthash::sha512t<256> {}.update(runtime_pass("hana")).final();
    REQUIRE(v2 == "2a0e3f7643580859507710f4569a60a86c83c025955298e7a93d766f71e8e399"_sha512_256);
    REQUIRE(v2 == v2r);

    constexpr auto v3 = cthash::sha512t<256> {}.update(array_of_zeros<32>()).final();
    auto v3r = cthash::sha512t<256> {}.update(runtime_pass(array_of_zeros<32, char>())).final();
    REQUIRE(v3 == "af13c048991224a5e4c664446b688aaf48fb5456db3629601b00ec160c74e554"_sha512_256);
    REQUIRE(v3 == v3r);

    constexpr auto v4 = cthash::sha512t<256> {}.update(array_of_zeros<64>()).final();
    auto v4r = cthash::sha512t<256> {}.update(runtime_pass(array_of_zeros<64>())).final();
    REQUIRE(v4 == "8aeecfa0b9f2ac7818863b1362241e4f32d06b100ae9d1c0fbcc4ed61b91b17a"_sha512_256);
    REQUIRE(v4 == v4r);

    constexpr auto v5 = cthash::sha512t<256> {}.update(array_of_zeros<120>()).final();
    auto v5r = cthash::sha512t<256> {}.update(runtime_pass(array_of_zeros<120, char>())).final();
    REQUIRE(v5 == "067880a5256c0584cff10526ed4c9761e584bf0ecdb1b12c2ae7f1dcedaf3dbf"_sha512_256);
    REQUIRE(v5 == v5r);

    constexpr auto v6 = cthash::sha512t<256> {}.update(array_of_zeros<128>()).final();
    auto v6r = cthash::sha512t<256> {}.update(runtime_pass(array_of_zeros<128>())).final();
    REQUIRE(v6 == "fe3d375e149b888e08e2521007764b422d2cd6f7b0606881b7fe1b1370d5fa88"_sha512_256);
    REQUIRE(v6 == v6r);

    constexpr auto v7 = cthash::sha512t<256> {}.update(array_of_zeros<512>()).final();
    auto v7r = cthash::sha512t<256> {}.update(runtime_pass(array_of_zeros<512>())).final();
    REQUIRE(v7 == "552b405c9716945bfc0caee69baec21b2a05560bfbf58db8bd1a4c2cc42b42a6"_sha512_256);
    REQUIRE(v7 == v7r);
}
