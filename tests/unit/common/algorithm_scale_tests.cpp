#include "catch2/catch.hpp"
#include "algorithm_scale.hpp"

TEST_CASE("checking scales", "[scale]") {
    SECTION("equal to left") {
        CHECK(scale(0, 0, 1, 0, 1) == 0);
        CHECK(scale(10, 10, 100, 0, 1) == 0);
        CHECK(scale(2, 2, 5, 5, 666) == 5);
    }

    SECTION("too small") {
        CHECK(scale(-1, 0, 1, 0, 1) == 0);
        CHECK(scale(1, 10, 100, 0, 1) == 0);
        CHECK(scale(0, 2, 5, 5, 666) == 5);
    }

    SECTION("equal to right") {
        CHECK(scale(1, 0, 1, 0, 1) == 1);
        CHECK(scale(100, 10, 100, 0, 1) == 1);
        CHECK(scale(5, 2, 5, 5, 666) == 666);
    }

    SECTION("too big") {
        CHECK(scale(1000, 0, 1, 0, 1) == 1);
        CHECK(scale(1000, -10, 100, 0, 1) == 1);
        CHECK(scale(2, -5, -2, 5, 666) == 666);
    }

    SECTION("inside") {
        CHECK(scale(1, 0, 2, 0, 2) == 1);
        CHECK(scale(10, 5, 15, 0, 10) == 5);
        CHECK(scale(0, -1, 1, -1, 9) == 4);
    }
}
