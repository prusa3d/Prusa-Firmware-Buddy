/// support_utils tests

#include "support_utils_lib.hpp"

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

using Catch::Matchers::Equals;

TEST_CASE("Block To Hex", "[support_utils]") {
    constexpr uint8_t str_size = 20;
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
        REQUIRE(str[2 * length] == (char)0);
        REQUIRE_THAT(str, Equals("964422005435F0"));
    }
}

TEST_CASE("Set bit", "[support_utils]") {
    SECTION("Already set bit") {
        uint8_t i = 0b01010101;
        setBit(&i, 0);
        CHECK(i == 0b01010101);
    }

    SECTION("Set bit 0") {
        uint8_t i = 0b01010100;
        setBit(&i, 0);
        CHECK(i == 0b01010101);
    }

    SECTION("Bit 7") {
        uint8_t i = 0b01010101;
        setBit(&i, 7);
        CHECK(i == 0b11010101);
    }
}

TEST_CASE("Clear bit", "[support_utils]") {
    SECTION("Already cleared bit") {
        uint8_t i = 0b01010100;
        clearBit(&i, 0);
        CHECK(i == 0b01010100);
    }

    SECTION("Unset bit 0") {
        uint8_t i = 0b01010101;
        clearBit(&i, 0);
        CHECK((uint16_t)i == 0b01010100);
    }

    SECTION("Bit 7") {
        uint8_t i = 0b11010101;
        clearBit(&i, 7);
        CHECK(i == 0b01010101);
    }
}

TEST_CASE("Right shift by 2 bits", "[support_utils]") {
    SECTION("Just test") {
        uint32_t n1 = 0b10111111'10011011'10110001'00010001;
        uint32_t n2 = 0b11000000'01111001'10111111'00010000;
        rShift2Bits(n1, n2);
        CHECK(n1 == 0b00101111'11100110'11101100'01000100);
        CHECK(n2 == 0b01000000'01111001'10111111'00010000);
    }
}

TEST_CASE("Convert to 32 symbol encoded string", "[support_utils]") {
    uint8_t number[4] = { 0b11011010, 0b01000100, 0b10111111, 0b10111110 };

    SECTION("First byte 0") {
        CHECK(to32(number, 0) == 'R'); // 27
    }

    SECTION("First byte 1") {
        CHECK(to32(number, 1) == 'M'); // 22
    }

    SECTION("First byte 2") {
        CHECK(to32(number, 2) == 'D'); // 13
    }

    SECTION("Crossing bytes") {
        CHECK(to32(number, 5) == '9'); // 9
    }

    SECTION("Next byte 1") {
        CHECK(to32(number, 8) == '8'); // 8
    }

    SECTION("Next byte 2") {
        CHECK(to32(number, 13) == 'I'); // 18
    }
}
