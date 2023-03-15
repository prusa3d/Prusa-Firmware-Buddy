#include <json_encode.h>

#include <catch2/catch.hpp>
#include <cstring>
#include <string_view>

using std::string_view;

TEST_CASE("bools") {
    REQUIRE(jsonify_bool(true) == string_view("true"));
    REQUIRE(jsonify_bool(false) == string_view("false"));
    // Yes, comparing pointers. These always return the same constant.
    REQUIRE(jsonify_bool(true) == jsonify_bool(true));
    REQUIRE(jsonify_bool(false) == jsonify_bool(false));
    REQUIRE(jsonify_bool(true) != jsonify_bool(false));
}

// Strings without anything special
TEST_CASE("String nothing weird") {
    const char *hello = "Hello world";
    REQUIRE(jsonify_str_buffer(hello) == 0);
    REQUIRE(jsonify_str_buffer_len(hello, strlen(hello)) == 0);

    JSONIFY_STR(hello);
    // Pointer comparison, shall point to the same thing.
    REQUIRE(hello == hello_escaped);
}

TEST_CASE("String escapes") {
    const char *hello = "Hello\t\nWorld\"':";
    REQUIRE(jsonify_str_buffer(hello) > strlen(hello));
    REQUIRE(jsonify_str_buffer_len(hello, strlen(hello)) > strlen(hello));

    JSONIFY_STR(hello);
    REQUIRE(hello_escaped == string_view("Hello\\t\\nWorld\\\"':"));
    REQUIRE(hello != hello_escaped);
}

TEST_CASE("String with zeroes") {
    const char *weird = "Hello\0World";
    const size_t len = 10; // Excluding the last d and terminating null.
    const size_t needed = jsonify_str_buffer_len(weird, len);
    REQUIRE(needed > len);
    char buffer[needed];
    jsonify_str_len(weird, len, buffer);
    REQUIRE(&buffer[0] == string_view("Hello\\u0000Worl"));
}

TEST_CASE("Escape len") {
    REQUIRE(jsonify_str_buffer("\n") == strlen("\\n") + 1);
    REQUIRE(jsonify_str_buffer_len("\0", 1) == strlen("\\u0000") + 1);
}
