#include <catch2/catch_test_macros.hpp>
#include <http/percent_encode.h>

#include <string_view>

using http::percent_encode;
using std::string_view;

namespace {

// attr-char of RFC 5987
constexpr string_view attr_char = "!#$&+-.^_`|~";

string_view encode(std::string_view input, char *buffer, size_t buffer_len) {
    return string_view(buffer, percent_encode(input, attr_char, buffer, buffer_len));
}

} // namespace

TEST_CASE("percent encode") {
    char buffer[64];

    SECTION("nothing to escape") {
        REQUIRE(encode("Model~1.gcode", buffer, sizeof buffer) == "Model~1.gcode");
    }

    SECTION("escaping") {
        // "ú" is 0xC3 0xBA in UTF-8.
        REQUIRE(encode("My M\xC3\xBA"
                       "del/%.gcode",
                    buffer, sizeof buffer)
            == "My%20M%C3%BAdel%2F%25.gcode");
    }

    SECTION("extra_unreserved is honored") {
        REQUIRE(string_view(buffer, percent_encode("a~b", "", buffer, sizeof buffer)) == "a%7Eb");
    }

    SECTION("empty input") {
        REQUIRE(encode("", buffer, sizeof buffer).empty());
    }

    SECTION("truncation keeps escapes whole") {
        REQUIRE(encode("ab cd", buffer, 4) == "ab");
        REQUIRE(encode("ab cd", buffer, 5) == "ab%20");
        REQUIRE(encode("", buffer, 0).empty());
    }
}
