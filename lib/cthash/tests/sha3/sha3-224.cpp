#include "../internal/support.hpp"
#include <cthash/sha3/sha3-224.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace cthash::literals;

TEST_CASE("sha3-224 basics") {
    const auto a = "6b4e03423667dbb73b6e15454f0eb1abd4597f9a1b078e3f5b5a6bc7"_sha3_224;
    REQUIRE(a.size() == 224u / 8u);
}

TEST_CASE("sha3-224 test strings") {
    SECTION("empty") {
        const auto r0 = cthash::sha3_224().update("").final();
        REQUIRE(r0 == "6b4e03423667dbb73b6e15454f0eb1abd4597f9a1b078e3f5b5a6bc7"_sha3_224);
    }

    SECTION("empty with bytes") {
        const auto r0 = cthash::sha3_224().update(std::span<const std::byte>()).final();
        REQUIRE(r0 == "6b4e03423667dbb73b6e15454f0eb1abd4597f9a1b078e3f5b5a6bc7"_sha3_224);
    }

    SECTION("test") {
        const auto r0 = cthash::sha3_224().update("test").final();
        REQUIRE(r0 == "3797bf0afbbfca4a7bbba7602a2b552746876517a7f9b7ce2db0ae7b"_sha3_224);
    }

    SECTION("hanicka") {
        const auto r0 = cthash::sha3_224().update("hanicka").final();
        REQUIRE(r0 == "bb8c7c352111f9e6e024c9aae448a25da287590ce6ff9be8063b6206"_sha3_224);
    }

    SECTION("*136 characters (exactly block size)") {
        auto in = std::string(size_t(136), '*'); // size of block
        const auto r0 = cthash::sha3_224().update(in).final();
        REQUIRE(r0 == "e6b935498d1db267b4eb1b71fcc6b0598f46ffb689ce2df273d70ee4"_sha3_224);
    }

    SECTION("*137 characters (exactly block + 1 size)") {
        auto in = std::string(size_t(137), '*'); // size of block + 1
        const auto r0 = cthash::sha3_224().update(in).final();
        REQUIRE(r0 == "3527595762e032bc0cc0974f6df0c8f2454ab09b6139a117f2b78467"_sha3_224);
    }

    SECTION("*2500 characters") {
        auto in = std::string(size_t(2500), '*'); // size of block + 1
        const auto r0 = cthash::sha3_224().update(in).final();
        REQUIRE(r0 == "7d4b80d6adeb7ba20723a0635b6582bd38c011389688305d381fa2cc"_sha3_224);
    }

    SECTION("*2500 by one") {
        auto h = cthash::sha3_224();
        for (int i = 0; i != 2500; ++i) {
            h.update("*");
        }
        const auto r0 = h.final();
        REQUIRE(r0 == "7d4b80d6adeb7ba20723a0635b6582bd38c011389688305d381fa2cc"_sha3_224);
    }
}

TEST_CASE("sha3-224 stability") {
    auto h = cthash::sha3_224();

    constexpr int end = int(h.rate) * 2;

    for (int i = 0; i != end; ++i) {
        const auto piece = std::string(size_t(i), '#');
        h.update(piece);
    }

    const auto r0 = h.final();
    REQUIRE(r0 == "2409c68f45c73652fef9ef6ca701560557fb01747b52f92ffffd0477"_sha3_224);
}
