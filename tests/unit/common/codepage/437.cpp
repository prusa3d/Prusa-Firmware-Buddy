#include <codepage/437.hpp>

#include <catch2/catch.hpp>
#include <cstring>

namespace {
// A weird string, a lot of things somewhere on the "border" of blocks, etc.
const uint8_t input[] = {
    1, // empty smilay
    31, // Downward triangle
    32, // Space
    49, // 1
    80, // P
    126, // ~
    127, // House
    128, // Ç
    243, // ≤
    254, // full square
    255, // No-break-space
    0, // null. Allows us to write the result as a string 😇
};

// Note: The space at the end may not look like it, but it's a NBSP, not an ordinary space.
const char expected[] = "☺▼ 1P~⌂Ç≤■ ";
} // namespace

TEST_CASE("437 -> utf8") {
    REQUIRE(codepage::cp437_to_utf8(nullptr, nullptr, 0) == 0);

    const size_t len = sizeof(expected);
    uint8_t output[sizeof(input) * 3 + 1];
    memset(output, 'X', sizeof(output));
    REQUIRE(sizeof(output) > len);
    REQUIRE(codepage::cp437_to_utf8(output, input, sizeof(input)) == len);
    REQUIRE(memcmp(expected, output, len) == 0);
}

TEST_CASE("utf8 -> 437 valid") {
    REQUIRE(codepage::utf8_to_cp437(nullptr, 0) == 0);

    const size_t len = sizeof(expected);
    uint8_t buffer[len];
    memcpy(buffer, expected, len);
    REQUIRE(codepage::utf8_to_cp437(buffer, sizeof(buffer)) == sizeof(input));
    REQUIRE(memcmp(input, buffer, sizeof(input)) == 0);
}
