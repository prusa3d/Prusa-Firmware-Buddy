/// @file
/// Unit tests for the WaitDeadline helper used by the blocking wait G-codes (M269)

#include <catch2/catch_test_macros.hpp>

#include <wait_deadline.hpp>

TEST_CASE("WaitDeadline") {
    SECTION("without a timeout it never expires") {
        const WaitDeadline deadline(1000, std::nullopt);
        REQUIRE_FALSE(deadline.expired(1000));
        REQUIRE_FALSE(deadline.expired(0xFFFFFFFF));
        REQUIRE_FALSE(deadline.expired(999)); // even a wrapped-around tick
    }

    SECTION("expires exactly after the timeout") {
        const WaitDeadline deadline(1000, 500);
        REQUIRE_FALSE(deadline.expired(1000));
        REQUIRE_FALSE(deadline.expired(1499));
        REQUIRE(deadline.expired(1500));
        REQUIRE(deadline.expired(2000));
    }

    SECTION("survives the tick counter wrap-around") {
        const WaitDeadline deadline(0xFFFFFF00, 0x200);
        REQUIRE_FALSE(deadline.expired(0xFFFFFFFF)); // 0xFF elapsed
        REQUIRE_FALSE(deadline.expired(0x000000FF)); // 0x1FF elapsed
        REQUIRE(deadline.expired(0x00000100)); // 0x200 elapsed
    }
}
