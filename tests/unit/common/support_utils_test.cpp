/// support_utils tests

#include "support_utils.h"

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

using Catch::Matchers::Equals;

TEST_CASE("End of string", "[support_utils]") {
    SECTION("Empty string") {
        char str[1] = "";
        char *eos = eofstr(str);
        CHECK(eos == '\0');
        CHECK(eos == str);
    }

    SECTION("Some string") {
        char str[20] = "sdjknrbtdfnvakd";
        char *eos = eofstr(str);
        CHECK(eos == '\0');
        CHECK(eos == str + 15);
    }
}

TEST_CASE("Block To Hex", "[support_utils]") {
    constexpr str_size = 20;
    char str[str_size];

    SECTION("Empty data") {
        uint8_t data[10];
        char c = (str[0] = 'A');
        block2hex(str, str_size, data, 0);
        CHECK(str[0] == c);
    }

    SECTION("Some data") {
        constexpr size_t length = 7;
        uint8_t data[length] = { 150, 68, 34, 0, 84, 53, 240 };
        block2hex(str, str_size, data, length);
        REQUIRE(str[2 * length] == '\0');
        REQUIRE_THAT(str, Equals("964422005435F0"));
    }
}

TEST_CASE("Set bit", "[support_utils]") {
    SECTION("Already set bit") {
        uint8_t i = 0b01010101;
        setbit(i, 0);
        CHECK(i == 0b01010101);
    }

    SECTION("Set bit 0") {
        uint8_t i = 0b01010100;
        setbit(i, 0);
        CHECK(i == 0b01010101);
    }

    SECTION("Bit 7") {
        uint8_t i = 0b01010101;
        setbit(i, 7);
        CHECK(i == 0b11010101);
    }
}

TEST_CASE("Clear bit", "[support_utils]") {
    SECTION("Already cleared bit") {
        uint8_t i = 0b01010100;
        setbit(i, 0);
        CHECK(i == 0b01010100);
    }

    SECTION("Unset bit 0") {
        uint8_t i = 0b01010101;
        setbit(i, 0);
        CHECK(i == 0b11010100);
    }

    SECTION("Bit 7") {
        uint8_t i = 0b11010101;
        setbit(i, 7);
        CHECK(i == 0b01010101);
    }
}
