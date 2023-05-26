#include "../internal/support.hpp"
#include <cthash/sha3/sha3-512.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace cthash::literals;

TEST_CASE("sha3-512 basics") {
    const auto a = "5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be035891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03"_sha3_512;
    REQUIRE(a.size() == 512u / 8u);
}

TEST_CASE("sha3-512 test strings") {
    SECTION("empty") {
        const auto r0 = cthash::sha3_512().update("").final();
        REQUIRE(r0 == "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26"_sha3_512);
    }

    SECTION("empty with bytes") {
        const auto r0 = cthash::sha3_512().update(std::span<const std::byte>()).final();
        REQUIRE(r0 == "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26"_sha3_512);
    }

    SECTION("test") {
        const auto r0 = cthash::sha3_512().update("test").final();
        REQUIRE(r0 == "9ece086e9bac491fac5c1d1046ca11d737b92a2b2ebd93f005d7b710110c0a678288166e7fbe796883a4f2e9b3ca9f484f521d0ce464345cc1aec96779149c14"_sha3_512);
    }

    SECTION("hanicka") {
        const auto r0 = cthash::sha3_512().update("hanicka").final();
        REQUIRE(r0 == "6d8826eec43e037dca9a7c92ffede543c067aa72d3aa55033f76f1789e0054a19f19f6636fc909744b85f26e334c77be8bea4cc70a520ca1ef6db88da98a9ecf"_sha3_512);
    }

    SECTION("*136 characters (exactly block size)") {
        auto in = std::string(size_t(136), '*'); // size of block
        const auto r0 = cthash::sha3_512().update(in).final();
        REQUIRE(r0 == "0b5070207de47faf3fca6aa3c44cef2f79f970151f283d2a36857437002e0ae76543f3c5d9818d6341ea1562a4d2329e3d1a9f4dc821c792bc69e3b51aef76cc"_sha3_512);
    }

    SECTION("*137 characters (exactly block + 1 size)") {
        auto in = std::string(size_t(137), '*'); // size of block + 1
        const auto r0 = cthash::sha3_512().update(in).final();
        REQUIRE(r0 == "844698f1032311721397d3189cfcfcb7199bc1309bc87bc20223cf6b7d27100dd37d21fde6557994aa330c0f493fc09eb600cbe0a8a6c63477d5f7a36077eb8b"_sha3_512);
    }

    SECTION("*2500 characters") {
        auto in = std::string(size_t(2500), '*'); // size of block + 1
        const auto r0 = cthash::sha3_512().update(in).final();
        REQUIRE(r0 == "e4163add44fed59d52141fe016088b98a9716e0fde36c9f0fce75937414bdcb8b4211d1909a5ccd7f32df8af6d991a7fe1f65238e6da7e591d946b289b6b0a49"_sha3_512);
    }

    SECTION("*2500 by one") {
        const auto star = std::string_view { "*" };
        REQUIRE(star.size() == 1u);
        auto h = cthash::sha3_512();
        for (int i = 0; i != 2500; ++i) {
            h.update(star);
        }
        const auto r0 = h.final();
        REQUIRE(r0 == "e4163add44fed59d52141fe016088b98a9716e0fde36c9f0fce75937414bdcb8b4211d1909a5ccd7f32df8af6d991a7fe1f65238e6da7e591d946b289b6b0a49"_sha3_512);
    }
}

TEST_CASE("sha3-512 stability") {
    auto h = cthash::sha3_512();

    constexpr int end = int(h.rate) * 2;

    for (int i = 0; i != end; ++i) {
        const auto piece = std::string(size_t(i), '#');
        h.update(piece);
    }

    const auto r0 = h.final();
    REQUIRE(r0 == "62a8487cf7d35dd2688962c80581436171c93c7250da289944ac95abd597baa12cca5c022a19addfaaae48f144da2e6dd9f24c02c63c8ca25bb3fc288277179c"_sha3_512);
}
