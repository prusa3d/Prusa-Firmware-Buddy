#include "catch2/catch.hpp"
#include "timing.h"
#include "timer_defaults.h"
#include "timing_private.h"

TEST_CASE("calculating ticks difference", "[timing]") {
    SECTION("trivial cases") {
        CHECK(ticks_diff(2, 1) == 1);
        CHECK(ticks_diff(1, 2) == -1);
        CHECK(ticks_diff(1, 1) == 0);
    }
    SECTION("cases with an overflow") {
        CHECK(ticks_diff(0, UINT32_MAX) == 1);
        CHECK(ticks_diff(UINT32_MAX, 0) == -1);
    }
}

TEST_CASE("tick conversions", "[timing]") {
    SECTION("clock_to_ns") {
        CHECK(clock_to_ns(0) == 0);
        CHECK(clock_to_ns(TIM_BASE_CLK_MHZ) == 1000);                      //1ms
        CHECK(clock_to_ns(TIM_BASE_CLK_MHZ * 1'000'000) == 1'000'000'000); //period
    }
    SECTION("cases with an overflow") {
        CHECK(ms_to_clock(0) == 0);
        CHECK(ms_to_clock(1) == TIM_BASE_CLK_MHZ * 1000);
        CHECK(ms_to_clock(UINT32_MAX) == uint64_t(UINT32_MAX) * uint64_t(TIM_BASE_CLK_MHZ) * uint64_t(1000)); //overflow
    }
}
