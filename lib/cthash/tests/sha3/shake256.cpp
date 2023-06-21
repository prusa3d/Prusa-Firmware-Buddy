#include "../internal/support.hpp"
#include <cthash/sha3/shake256.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace cthash::literals;

TEST_CASE("shake256 literal basics") {
    const auto a = "1234"_shake256;
    REQUIRE(a.size() == 2u);

    const auto b = "12345678"_shake256;
    REQUIRE(b.size() == 4u);

    const auto c = "1234567890abcdef"_shake256;
    REQUIRE(c.size() == 8u);

    REQUIRE(a == b);
    REQUIRE(a == c);

    REQUIRE((a <=> b) == 0);
    REQUIRE((a <=> c) == 0);
}

TEST_CASE("shake256 calculation") {
    const auto expected = "2f671343d9b2e1604dc9dcf0753e5fe15c7c64a0d283cbbf722d411a0e36f6ca1d01d1369a23539cd80f7c054b6e5daf9c962cad5b8ed5bd11998b40d5734442bed798f6e5c915bd8bb07e0188d0a55c1290074f1c287af06352299184492cbdec9acba737ee292e5adaa445547355e72a03a3bac3aac770fe5d6b66600ff15d37d5b4789994ea2aeb097f550aa5e88e4d8ff0ba07b88c1c88573063f5d96df820abc2abd177ab037f351c375e553af917132cf2f563c79a619e1bb76e8e2266b0c5617d695f2c496a25f4073b6840c1833757ebb386f16757a8e16a21e9355e9b248f3b33be672da700266be99b8f8725e8ab06075f0219e655ebc188976364"_shake256;

    SECTION("16 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dog").final<16>();

        REQUIRE(r0 == expected);
    }

    SECTION("32 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dog").final<32>();

        REQUIRE(r0 == expected);
    }

    SECTION("64 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dog").final<64>();

        REQUIRE(r0 == expected);
    }

    SECTION("128 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dog").final<128>();

        REQUIRE(r0 == expected);
    }

    SECTION("256 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dog").final<256>();

        REQUIRE(r0 == expected);
    }

    SECTION("2048 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dog").final<2048>();

        REQUIRE(r0 == expected);
    }
}

TEST_CASE("shake256 calculation (b)") {
    const auto expected = "46b1ebb2e142c38b9ac9081bef72877fe4723959640fa57119b366ce6899d4013af024f4222921320bee7d3bfaba07a758cd0fde5d27bbd2f8d709f4307d2c34a0baacb6f2ca73e5bdfe951c4f6f80ccf14a216c17512129d7afdccbc31aee975c41c66c24a2367820d2d5914fab194fea6b5749372aabf0276c3424d3145ec41645053272fe036dd13daab3b80e959290e885838a5e57a509d2785e99f83ce3efd810959d2a97f0e244a1b8382405ca3d6673f382054794f74d73af2765452442e72e6d199fe83ecf663cc6a14b57b2f16ccf3a83a613e7475c4b0761711e216e68051ca97c3b6cc9a91e35d442506443599f42921bba9558c62a5fdcf1ea42"_shake256;

    SECTION("16 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dof").final<16>();

        REQUIRE(r0 == expected);
    }

    SECTION("32 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dof").final<32>();

        REQUIRE(r0 == expected);
    }

    SECTION("64 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dof").final<64>();

        REQUIRE(r0 == expected);
    }

    SECTION("128 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dof").final<128>();

        REQUIRE(r0 == expected);
    }

    SECTION("256 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dof").final<256>();

        REQUIRE(r0 == expected);
    }

    SECTION("2048 bits") {
        auto r0 = cthash::shake256().update("The quick brown fox jumps over the lazy dof").final<2048>();

        REQUIRE(r0 == expected);
    }
}
