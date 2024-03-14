// #define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

using Catch::Matchers::Equals;
#include <filters/median_filter.hpp>

TEST_CASE("Median 3 test", "[med3]") {
    static MedianFilter filter;
    SECTION("Positive") {
        int32_t nums[3] = { 1, 2, 3 };
        CHECK(filter.median_3_i32(nums) == 1);
    }

    SECTION("Negative") {
        int32_t nums[3] = { -1, -2, -3 };
        CHECK(filter.median_3_i32(nums) == 1);
    }

    SECTION("Mixed") {
        int32_t nums[3] = { 0, -2, 3 };
        CHECK(filter.median_3_i32(nums) == 0);
    }

    SECTION("Shuffle 0") {
        int32_t nums[3] = { 50, -2, 300 };
        CHECK(filter.median_3_i32(nums) == 0);
    }

    SECTION("Shuffle 1") {
        int32_t nums[3] = { 5, 20, 300 };
        CHECK(filter.median_3_i32(nums) == 1);
    }

    SECTION("Shuffle 2") {
        int32_t nums[3] = { -50, -2, -30 };
        CHECK(filter.median_3_i32(nums) == 2);
    }
}
