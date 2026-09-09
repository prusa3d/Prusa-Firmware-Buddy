/// support_utils tests

#include "support_utils_lib.hpp"

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using Catch::Matchers::Equals;

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

TEST_CASE("to32 does not return '!' for valid 5-bit extractions", "[support_utils][to32]") {

    // various data patterns
    std::vector<uint8_t> all_zeros(10, 0x00);
    std::vector<uint8_t> all_ones(10, 0xFF);
    std::vector<uint8_t> alternating_bits(10);
    for (int i = 0; i < 10; ++i) {
        alternating_bits[i] = (i % 2 == 0) ? 0xAA : 0x55; // 10101010, 01010101, ...
    }

    for (uint8_t startBit = 0; startBit < 70; ++startBit) {

        // For all_zeros, val should always be 0
        REQUIRE(to32(all_zeros.data(), startBit) == '0');

        // For all_ones, val should always be 31 (0b11111) which maps to 'V'
        REQUIRE(to32(all_ones.data(), startBit) == 'V');

        // Test with alternating_bits
        REQUIRE(to32(alternating_bits.data(), startBit) != '!');
    }
}

TEST_CASE("to32 specific values (edge cases for bit alignment)", "[support_utils][to32][edge]") {
    uint8_t data[2] = { 0x00, 0x00 }; // testing byte crossing

    // Test extracting 0b11111 (31) starting at bit 0
    data[0] = 0b11111000;
    REQUIRE(to32(data, 0) == 'V'); // Expect 31 -> 'V'
    data[0] = 0x00; // Reset

    // Test extracting 0b11111 (31) starting at bit 3 (last possible within first byte)
    data[0] = 0b00011111;
    REQUIRE(to32(data, 3) == 'V');
    data[0] = 0x00; // Reset

    // Test extracting 0b11111 (31) starting at bit 4 (first to cross byte boundary)
    data[0] = 0b00001111;
    data[1] = 0b10000000;
    REQUIRE(to32(data, 4) == 'V');
    data[0] = 0x00;
    data[1] = 0x00;

    // Test extracting 0b11111 (31) starting at bit 7
    data[0] = 0b00000001;
    data[1] = 0b11110000;
    REQUIRE(to32(data, 7) == 'V');
    data[0] = 0x00;
    data[1] = 0x00;

    data[0] = 0x00;
    data[1] = 0x00;
    REQUIRE(to32(data, 0) == '0');
    REQUIRE(to32(data, 3) == '0');
    REQUIRE(to32(data, 4) == '0');
    REQUIRE(to32(data, 7) == '0');

    // Test cases that should yield other values (e.g., 10, 'A')
    data[0] = 0b00101000; // 0b01010 -> 10
    REQUIRE(to32(data, 1) == 'A');
    data[0] = 0b00000101; // 0b00101 -> 5
    REQUIRE(to32(data, 3) == '5');
    data[0] = 0b00000001;
    data[1] = 0b01000000; // 0b10100 -> 'K'
    REQUIRE(to32(data, 7) == 'K');
}
