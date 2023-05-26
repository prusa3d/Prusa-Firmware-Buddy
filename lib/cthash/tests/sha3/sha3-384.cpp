#include "../internal/support.hpp"
#include <cthash/sha3/sha3-384.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace cthash::literals;

TEST_CASE("sha3-384 basics") {
    const auto a = "9c87cb0b9a8924030b93b4b3fd397c70030a57c6f98a051e0cc87d6d671e4da02e5fe4c6e8fe9d1ca46f085434092a99"_sha3_384;
    REQUIRE(a.size() == 384u / 8u);
}

TEST_CASE("sha3-384 test strings") {
    SECTION("empty") {
        const auto r0 = cthash::sha3_384().update("").final();
        REQUIRE(r0 == "0c63a75b845e4f7d01107d852e4c2485c51a50aaaa94fc61995e71bbee983a2ac3713831264adb47fb6bd1e058d5f004"_sha3_384);
    }

    SECTION("empty with bytes") {
        const auto r0 = cthash::sha3_384().update(std::span<const std::byte>()).final();
        REQUIRE(r0 == "0c63a75b845e4f7d01107d852e4c2485c51a50aaaa94fc61995e71bbee983a2ac3713831264adb47fb6bd1e058d5f004"_sha3_384);
    }

    SECTION("test") {
        const auto r0 = cthash::sha3_384().update("test").final();
        REQUIRE(r0 == "e516dabb23b6e30026863543282780a3ae0dccf05551cf0295178d7ff0f1b41eecb9db3ff219007c4e097260d58621bd"_sha3_384);
    }

    SECTION("hanicka") {
        const auto r0 = cthash::sha3_384().update("hanicka").final();
        REQUIRE(r0 == "9c87cb0b9a8924030b93b4b3fd397c70030a57c6f98a051e0cc87d6d671e4da02e5fe4c6e8fe9d1ca46f085434092a99"_sha3_384);
    }

    SECTION("*136 characters (exactly block size)") {
        auto in = std::string(size_t(136), '*'); // size of block
        const auto r0 = cthash::sha3_384().update(in).final();
        REQUIRE(r0 == "d86290cbc1abedee2f677b4938836ce984d8ddd955beefa2b421a55747ddcd3a2c5e5d0aae4e3173e92e2a2c80891980"_sha3_384);
    }

    SECTION("*137 characters (exactly block + 1 size)") {
        auto in = std::string(size_t(137), '*'); // size of block + 1
        const auto r0 = cthash::sha3_384().update(in).final();
        REQUIRE(r0 == "23082738027c7f6aa5aaf64312869e586352ee0077aa246bb4ac7a9459210af9397af84160ee335f6585f86148a9d396"_sha3_384);
    }

    SECTION("*2500 characters") {
        auto in = std::string(size_t(2500), '*'); // size of block + 1
        const auto r0 = cthash::sha3_384().update(in).final();
        REQUIRE(r0 == "2f5adba183a526a1c34e38575846001c5bc749c02af04beb6b0bd4bfc94f6f34ef1af3471fc438b28c78e652fc166129"_sha3_384);
    }

    SECTION("*2500 by one") {
        auto h = cthash::sha3_384();
        for (int i = 0; i != 2500; ++i) {
            h.update("*");
        }
        const auto r0 = h.final();
        REQUIRE(r0 == "2f5adba183a526a1c34e38575846001c5bc749c02af04beb6b0bd4bfc94f6f34ef1af3471fc438b28c78e652fc166129"_sha3_384);
    }
}

TEST_CASE("sha3-384 stability") {
    auto h = cthash::sha3_384();

    constexpr int end = int(h.rate) * 2;

    for (int i = 0; i != end; ++i) {
        const auto piece = std::string(size_t(i), '#');
        h.update(piece);
    }

    const auto r0 = h.final();
    REQUIRE(r0 == "83c6f40bf077d3f198ff2acc88eb2b82bab09bd9733e82fae081f7a50597160d02254357c352064a37104e8108d2d2bc"_sha3_384);
}
