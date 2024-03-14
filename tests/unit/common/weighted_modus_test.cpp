#include "catch2/catch.hpp"

#include "homing_modus.hpp"

using Catch::Matchers::Equals;

TEST_CASE("Circular distance", "") {

    SECTION("Zero distance") {
        CHECK(0 == to_calibrated(0, 0));
        CHECK(0 == to_calibrated(50, 50));
        CHECK(0 == to_calibrated(1023, 1023));
    }

    SECTION("Simple distance") {
        CHECK(50 == to_calibrated(50, 0));
        CHECK(40 == to_calibrated(290, 250));
        CHECK(23 == to_calibrated(1023, 1000));
    }

    SECTION("Negative distance") {
        CHECK(-50 == to_calibrated(0, 50));
        CHECK(-40 == to_calibrated(250, 290));
        CHECK(-23 == to_calibrated(1000, 1023));
    }

    SECTION("Distance over the edge") {
        CHECK(124 == to_calibrated(0, 900));
        CHECK(-124 == to_calibrated(900, 0));
        CHECK(-200 == to_calibrated(824, 0));
    }
}

TEST_CASE("Weighted modus", "") {

    SECTION("Zeros") {
        const int size = 5;
        uint16_t positions[size] = { 0, 0, 0, 0, 0 };
        CHECK(0 == home_modus(positions, size, 128));
    }

    SECTION("Same number") {
        const int size = 5;
        uint16_t positions[size] = { 50, 50, 50, 50, 50 };
        CHECK(50 == home_modus(positions, size, 128));
    }

    SECTION("Single modus") {
        const int size = 5;
        uint16_t positions[size] = { 0, 0, 1, 1, 1 };
        CHECK(1 == home_modus(positions, size, 128));
    }

    SECTION("Two modi") {
        const int size = 7;
        uint16_t positions[size] = { 1, 1, 1, 5, 5, 5, 6 };
        CHECK(5 == home_modus(positions, size, 128));
    }

    SECTION("Three modi") {
        const int size = 10;
        uint16_t positions[size] = { 1, 1, 1, 5, 5, 5, 6, 6, 6, 7 };
        CHECK(5 == home_modus(positions, size, 128));
    }

    SECTION("Distant modi") {
        const int size = 10;
        uint16_t positions[size] = { 1, 1, 1, 205, 205, 205, 206, 206, 206, 207 };
        CHECK(206 == home_modus(positions, size, 128));
    }

    SECTION("Round modi") {
        const int size = 11;
        uint16_t positions[size] = { 1, 1, 1, 1005, 1005, 1005, 2, 206, 206, 206, 207 };
        CHECK(1 == home_modus(positions, size, 128));
    }
}
