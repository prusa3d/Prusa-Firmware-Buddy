#include "../internal/support.hpp"
#include <cthash/sha3/shake128.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace cthash::literals;

TEST_CASE("shake128 literal basics") {
    const auto a = "1234"_shake128;
    REQUIRE(a.size() == 2u);

    const auto b = "12345678"_shake128;
    REQUIRE(b.size() == 4u);

    const auto c = "1234567890abcdef"_shake128;
    REQUIRE(c.size() == 8u);

    const auto d = "1234567800000000"_shake128;
    REQUIRE(d.size() == 8u);

    REQUIRE(a == b);
    REQUIRE(a == c);

    REQUIRE(a == d);
    REQUIRE(b == d);

    REQUIRE(c != d);

    REQUIRE((a <=> b) == 0);
    REQUIRE((a <=> c) == 0);
}

TEST_CASE("shake128 calculation") {
    const auto expected = "f4202e3c5852f9182a0430fd8144f0a74b95e7417ecae17db0f8cfeed0e3e66eb5585ec6f86021cacf272c798bcf97d368b886b18fec3a571f096086a523717a3732d50db2b0b7998b4117ae66a761ccf1847a1616f4c07d5178d0d965f9feba351420f8bfb6f5ab9a0cb102568eabf3dfa4e22279f8082dce8143eb78235a1a54914ab71abb07f2f3648468370b9fbb071e074f1c030a4030225f40c39480339f3dc71d0f04f71326de1381674cc89e259e219927fae8ea2799a03da862a55afafe670957a2af3318d919d0a3358f3b891236d6a8e8d19999d1076b529968faefbd880d77bb300829dca87e9c8e4c28e0800ff37490a5bd8c36c0b0bdb2701a"_shake128;

    SECTION("16 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dog").final<16>();

        REQUIRE(r0 == expected);
    }

    SECTION("32 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dog").final<32>();

        REQUIRE(r0 == expected);
    }

    SECTION("64 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dog").final<64>();

        REQUIRE(r0 == expected);
    }

    SECTION("128 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dog").final<128>();

        REQUIRE(r0 == expected);
    }

    SECTION("256 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dog").final<256>();

        REQUIRE(r0 == expected);
    }

    SECTION("2048 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dog").final<2048>();

        REQUIRE(r0 == expected);
    }
}

TEST_CASE("shake128 calculation (b)") {
    const auto expected = "853f4538be0db9621a6cea659a06c1107b1f83f02b13d18297bd39d7411cf10c79229b22a2d29332019fca1dde63ad893e9b2fb2cf1e710e591a7dfaab201f528373514e015d872edb70b59eb04480dc669851e9f72272e33e382701575e9cd0a3247ffd784d968801c3e40bf9c0ec3b118e838329d89f16a081a1201e8350f5c839c846ec0262a6dec85450e0b350786b2e47b93de4f9a566b5220300d19cb9783af4a6242c19e36881cbcf328e5419009af9906634c37a37fecb35b8a96476a7e3fae1be94b90a9a635fac947bf633f5280d580e571ec3b019086e8166fcceb2478a2834ce35651b662be28382ceb1b5f6f6f7263c57037e58336fd1c354bb"_shake128;

    SECTION("16 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dof").final<16>();

        REQUIRE(r0 == expected);
    }

    SECTION("32 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dof").final<32>();

        REQUIRE(r0 == expected);
    }

    SECTION("64 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dof").final<64>();

        REQUIRE(r0 == expected);
    }

    SECTION("128 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dof").final<128>();

        REQUIRE(r0 == expected);
    }

    SECTION("256 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dof").final<256>();

        REQUIRE(r0 == expected);
    }

    SECTION("2048 bits") {
        auto r0 = cthash::shake128().update("The quick brown fox jumps over the lazy dof").final<2048>();

        REQUIRE(r0 == expected);
    }
}
