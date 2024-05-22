#include "catch2/catch.hpp"
#include "algorithm_range.hpp"
#include <fstream>
#include <string>
#include <chrono>
#include <regex>
#include <string_view>
#include <optional>
#include "filters/median_filter.hpp"

TEST_CASE("checking ranges", "[range]") {
    SECTION("equal to left") {
        CHECK_FALSE(IsInOpenRange(0, 0, 1));
        CHECK(IsInClosedRange(0, 0, 1));
        CHECK_FALSE(IsInLeftOpenRange(0, 0, 1));
        CHECK(IsInRightOpenRange(0, 0, 1));

        CHECK_FALSE(IsInOpenRange(0, 0, -1));
        CHECK(IsInClosedRange(0, 0, -1));
        CHECK_FALSE(IsInLeftOpenRange(0, 0, -1));
        CHECK(IsInRightOpenRange(0, 0, -1));

        CHECK_FALSE(IsInOpenRange(0.0, 0.0, 1.0));
        CHECK(IsInClosedRange(0.0, 0.0, 1.0));
        CHECK_FALSE(IsInLeftOpenRange(0.0, 0.0, 1.0));
        CHECK(IsInRightOpenRange(0.0, 0.0, 1.0));

        CHECK_FALSE(IsInOpenRange(0.0, 0.0, -1.0));
        CHECK(IsInClosedRange(0.0, 0.0, -1.0));
        CHECK_FALSE(IsInLeftOpenRange(0.0, 0.0, -1.0));
        CHECK(IsInRightOpenRange(0.0, 0.0, -1.0));
    }

    SECTION("too small") {
        CHECK_FALSE(IsInOpenRange(-1, 0, 1));
        CHECK_FALSE(IsInClosedRange(0, 3, 5));
        CHECK_FALSE(IsInLeftOpenRange(1, 10, 100));
        CHECK_FALSE(IsInRightOpenRange(2, 3, 4));

        CHECK_FALSE(IsInOpenRange(-1, 1, 0));
        CHECK_FALSE(IsInClosedRange(0, 4, 3));
        CHECK_FALSE(IsInLeftOpenRange(1, 100, 10));
        CHECK_FALSE(IsInRightOpenRange(2, 4, 3));

        CHECK_FALSE(IsInOpenRange(-1.0, 0.0, 1.0));
        CHECK_FALSE(IsInClosedRange(0.0, 3.0, 5.0));
        CHECK_FALSE(IsInLeftOpenRange(1.0, 10.0, 100.0));
        CHECK_FALSE(IsInRightOpenRange(2.0, 3.0, 4.0));

        CHECK_FALSE(IsInOpenRange(-1.0, 1.0, 0.0));
        CHECK_FALSE(IsInClosedRange(0.0, 4.0, 3.0));
        CHECK_FALSE(IsInLeftOpenRange(1.0, 100.0, 10.0));
        CHECK_FALSE(IsInRightOpenRange(2.0, 4.0, 3.0));
    }

    SECTION("equal to right") {
        CHECK_FALSE(IsInOpenRange(1, 0, 1));
        CHECK(IsInClosedRange(1, 0, 1));
        CHECK(IsInLeftOpenRange(1, 0, 1));
        CHECK_FALSE(IsInRightOpenRange(1, 0, 1));

        CHECK_FALSE(IsInOpenRange(-1, 0, -1));
        CHECK(IsInClosedRange(-1, 0, -1));
        CHECK(IsInLeftOpenRange(-1, 0, -1));
        CHECK_FALSE(IsInRightOpenRange(-1, 0, -1));

        CHECK_FALSE(IsInOpenRange(1.0, 0.0, 1.0));
        CHECK(IsInClosedRange(1.0, 0.0, 1.0));
        CHECK(IsInLeftOpenRange(1.0, 0.0, 1.0));
        CHECK_FALSE(IsInRightOpenRange(1.0, 0.0, 1.0));

        CHECK_FALSE(IsInOpenRange(-1.0, 0.0, -1.0));
        CHECK(IsInClosedRange(-1.0, 0.0, -1.0));
        CHECK(IsInLeftOpenRange(-1.0, 0.0, -1.0));
        CHECK_FALSE(IsInRightOpenRange(-1.0, 0.0, -1.0));
    }

    SECTION("too big") {
        CHECK_FALSE(IsInOpenRange(5, 0, 1));
        CHECK_FALSE(IsInClosedRange(6, 3, 5));
        CHECK_FALSE(IsInLeftOpenRange(1111, 10, 100));
        CHECK_FALSE(IsInRightOpenRange(-2, -4, -3));

        CHECK_FALSE(IsInOpenRange(5, 1, 0));
        CHECK_FALSE(IsInClosedRange(6, 5, 3));
        CHECK_FALSE(IsInLeftOpenRange(1111, 100, 10));
        CHECK_FALSE(IsInRightOpenRange(-2, -3, -4));

        CHECK_FALSE(IsInOpenRange(5.0, 0.0, 1.0));
        CHECK_FALSE(IsInClosedRange(6.0, 3.0, 5.0));
        CHECK_FALSE(IsInLeftOpenRange(1111.0, 10.0, 100.0));
        CHECK_FALSE(IsInRightOpenRange(-2.0, -4.0, -3.0));

        CHECK_FALSE(IsInOpenRange(5.0, 1.0, 0.0));
        CHECK_FALSE(IsInClosedRange(6.0, 5.0, 3.0));
        CHECK_FALSE(IsInLeftOpenRange(1111.0, 100.0, 10.0));
        CHECK_FALSE(IsInRightOpenRange(-2.0, -3.0, -4.0));
    }

    SECTION("inside") {
        CHECK(IsInOpenRange(1, 0, 2));
        CHECK(IsInClosedRange(4, 3, 5));
        CHECK(IsInLeftOpenRange(11, 10, 100));
        CHECK(IsInRightOpenRange(-2, -4, -1));

        CHECK(IsInOpenRange(1, 2, 0));
        CHECK(IsInClosedRange(4, 5, 3));
        CHECK(IsInLeftOpenRange(11, 100, 10));
        CHECK(IsInRightOpenRange(-2, -1, -4));

        CHECK(IsInOpenRange(1.0, 0.0, 2.0));
        CHECK(IsInClosedRange(4.0, 3.0, 5.0));
        CHECK(IsInLeftOpenRange(11.0, 10.0, 100.0));
        CHECK(IsInRightOpenRange(-2.0, -4.0, -1.0));

        CHECK(IsInOpenRange(1.0, 2.0, 0.0));
        CHECK(IsInClosedRange(4.0, 5.0, 3.0));
        CHECK(IsInLeftOpenRange(11.0, 100.0, 10.0));
        CHECK(IsInRightOpenRange(-2.0, -1.0, -4.0));
    }

    SECTION("special") {
        CHECK_FALSE(IsInOpenRange<double>(NAN, 0.0, 1.0));
        CHECK_FALSE(IsInClosedRange<double>(NAN, 0.0, 1.0));
        CHECK_FALSE(IsInLeftOpenRange<double>(NAN, 0.0, 1.0));
        CHECK_FALSE(IsInRightOpenRange<double>(NAN, 0.0, 1.0));

        CHECK_FALSE(IsInOpenRange<double>(INFINITY, 0.0, 1.0));
        CHECK_FALSE(IsInClosedRange<double>(INFINITY, 0.0, 1.0));
        CHECK_FALSE(IsInLeftOpenRange<double>(INFINITY, 0.0, 1.0));
        CHECK_FALSE(IsInRightOpenRange<double>(INFINITY, 0.0, 1.0));
    }
}
