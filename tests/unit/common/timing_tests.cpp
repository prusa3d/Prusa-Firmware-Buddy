#include "catch2/catch.hpp"
#include "timing.h"
#include "timer_defaults.h"

TEST_CASE("calculating ticks difference", "[timing]") {
    SECTION("trivial cases") {
        CHECK(ticks_diff(2, 1) == 1);
        CHECK(ticks_diff(1, 2) == -1);
        CHECK(ticks_diff(1, 1) == 0);
        CHECK(ticks_diff(2147483647, 0) == 2147483647);
        CHECK(ticks_diff(0, 2147483648) == -2147483648);
    }
    SECTION("cases with an overflow") {
        CHECK(ticks_diff(0, UINT32_MAX) == 1);
        CHECK(ticks_diff(UINT32_MAX, 0) == -1);
        CHECK(ticks_diff(5, UINT32_MAX - 5) == 11);
        CHECK(ticks_diff(UINT32_MAX - 5, 5) == -11);
    }
}
