/// str_utils tests

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
using Catch::Matchers::Equals;

#define STM32F407xx

#include "gcode.cpp"

TEST_CASE("G code class", "[gcode]") {
    gCode gc;

    SECTION("write string") {
        int buffer = gc.free();
        const char text[] = "abcde";
        gc.write(text);
        CHECK(buffer - gc.free());
        REQUIRE_THAT(text, Equals(gc.readChars()));
    }

    SECTION("write float") {
        gc.write(10.5f);
        REQUIRE_THAT("10.5", Equals(gc.readChars()));
    }

    SECTION("full buffer by string") {
        for (int i = 0; i < bufferSize + 10; ++i)
            gc.write("A");

        CHECK(gc.free() <= 0);
        CHECK(gc.isError());
        CHECK(gc.error() != 0);
        gc.resetError();
        CHECK(!gc.isError());
        CHECK(gc.error() == 0);
        REQUIRE(gc.isFull());
    }

    SECTION("clear") {
        gc.write("ABCD");
        gc.clear();
        const char text[] = "abcde";
        gc.write(text);
        REQUIRE_THAT(text, Equals(gc.readChars()));
    }

    SECTION("write parameter") {
        gc.param('S', 10.5);
        REQUIRE_THAT(" S10.5", Equals(gc.readChars()));
    }

    SECTION("write G G code") {
        gc.G(28);
        REQUIRE_THAT("G28", Equals(gc.readChars()));
    }

    SECTION("write 2 G G code") {
        gc.G(28);
        gc.G(28);
        REQUIRE_THAT("G28\nG28", Equals(gc.readChars()));
    }

    SECTION("write G codes chained") {
        gc.G(28).G(28);
        REQUIRE_THAT("G28\nG28", Equals(gc.readChars()));
    }

    SECTION("write M code") {
        gc.M(28);
        REQUIRE_THAT("M28", Equals(gc.readChars()));
    }

    SECTION("add space") {
        gc.space();
        REQUIRE_THAT(" ", Equals(gc.readChars()));
    }

    SECTION("add new line") {
        gc.newLine();
        REQUIRE_THAT("\n", Equals(gc.readChars()));
    }

    SECTION("add full G1 code") {
        gc.G1(1, 2, 3, 4, 5);
        REQUIRE_THAT("G1 X1 Y2 Z3 E4 F5", Equals(gc.readChars()));
    }

    SECTION("chain G1 codes") {
        gc.G1(1, 2, NAN, 4, 5).G1(NAN, NAN, 2, -5, 10);
        REQUIRE_THAT("G1 X1 Y2 E4 F5\nG1 Z2 E-5 F10", Equals(gc.readChars()));
    }

    SECTION("add G1 code with NANs") {
        gc.G1(NAN, NAN, NAN, NAN, NAN);
        REQUIRE_THAT("G1", Equals(gc.readChars()));
    }

    SECTION("add G1 short code") {
        gc.G1(1, 2, 3);
        REQUIRE_THAT("G1 X1 Y2 E3", Equals(gc.readChars()));
    }

    SECTION("extrude with predefined values") {
        gc.lastExtrusion(0, 0);
        gc.ex(3, 4);
        // sqrt(3^2 + 4^2) * 0.2 * 0.5 / (3.1415 * (1.75 / 2)^2)
        REQUIRE_THAT("G1 X3 Y4 E0.2079", Equals(gc.readChars()));
    }
}
