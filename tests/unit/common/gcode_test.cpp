/// str_utils tests

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
using Catch::Matchers::Equals;

#define STM32F407xx

#include "gcode.cpp"

TEST_CASE("G code class", "[gcode]") {
    gCode gc;

    SECTION("write string") {
        const char text[] = "abcde";
        gc.write(text);
        REQUIRE_THAT(gc.readChars(), Equals(text));
    }
}
