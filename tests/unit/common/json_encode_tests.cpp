#include <json_encode.h>

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <sstream>

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

    JSONIFY_STR(hello);
    // Pointer comparison, shall point to the same thing.
    REQUIRE(hello == hello_escaped);
}

TEST_CASE("String escapes") {
    const char *hello = "Hello\t\nWorld\"':";
    REQUIRE(jsonify_str_buffer(hello) > strlen(hello));

    JSONIFY_STR(hello);
    REQUIRE(hello_escaped == string_view("Hello\\t\\nWorld\\\"':"));
    REQUIRE(hello != hello_escaped);
}

TEST_CASE("String with zeroes") {
    const char *weird = "Hello\0World";
    const size_t len = 10; // Excluding the last d and terminating null.
    const size_t needed = jsonify_str_buffer(std::string_view { weird, len });
    REQUIRE(needed > len);
    char buffer[needed];
    jsonify_str(std::string_view { weird, len }, buffer);
    REQUIRE(&buffer[0] == string_view("Hello\\u0000Worl"));
}

TEST_CASE("Escape len") {
    REQUIRE(jsonify_str_buffer("\n") == strlen("\\n") + 1);
    REQUIRE(jsonify_str_buffer(std::string_view { "\0", 1 }) == strlen("\\u0000") + 1);
}

TEST_CASE("Unescape json") {
    char json[] = R"(1\"\\a\"34\f\b5\r\n6\n\\78\t0)";
    size_t new_size = unescape_json_i(json, strlen(json));

    INFO("json: " + std::string(json));
    const char *expected = "1\"\\a\"34\f\b5\r\n6\n\\78\t0";
    REQUIRE(new_size == strlen(expected));
    REQUIRE(strncmp(json, expected, strlen(expected)) == 0);
}

TEST_CASE("Unescape empty json") {
    char json[] = "";
    size_t new_size = unescape_json_i(json, strlen(json));

    INFO("json: " + std::string(json));
    REQUIRE(new_size == 0);
    REQUIRE(strcmp(json, "") == 0);
}

TEST_CASE("Nothing to unescape json") {
    char json[] = "1234567890abcdefgh";
    size_t new_size = unescape_json_i(json, strlen(json));

    INFO("json: " + std::string(json));
    const char *expected = "1234567890abcdefgh";
    REQUIRE(new_size == strlen(expected));
    REQUIRE(strncmp(json, expected, strlen(expected)) == 0);
}

TEST_CASE("Unescape only part of string") {
    char json[] = R"(\"abc\12345)";
    size_t new_size = unescape_json_i(json, 4);

    INFO("json: " + std::string(json));
    const char *expected = R"("ab)";
    REQUIRE(new_size == strlen(expected));
    REQUIRE(strncmp(json, expected, strlen(expected)) == 0);
}

TEST_CASE("null unescape") {
    char json[] = R"(abc\u00001234)";
    size_t new_size = unescape_json_i(json, strlen(json));

    INFO("json: " + std::string(json));
    REQUIRE(new_size == 8);
    REQUIRE(json == string_view("abc"));
    REQUIRE(json[3] == '\0');
    REQUIRE(strncmp(json + 4, "1234", 4) == 0);
}

TEST_CASE("not escaping the whole null sequence") {
    char json[] = R"(abc\u0000bla)";
    size_t new_size = unescape_json_i(json, 8);

    INFO("json: " + std::string(json));
    const char *expected = R"(abc\u000)";
    REQUIRE(new_size == strlen(expected));
    REQUIRE(strncmp(json, expected, strlen(expected)) == 0);
}

TEST_CASE("unescape slash char at the end") {
    char json[] = R"(abc\"123\"a)";
    size_t new_size = unescape_json_i(json, 9);

    INFO("json: " + std::string(json));
    const char *expected = R"(abc"123\)";
    REQUIRE(new_size == strlen(expected));
    REQUIRE(strncmp(json, expected, strlen(expected)) == 0);
}

TEST_CASE("Unescape a unicode escape") {
    // The regression: System.Text.Json escapes '+' as \u002B. Leaving the
    // backslash in place turned a filename into a two-segment path, because
    // FatFs treats a backslash as a separator.
    char json[] = R"(L\u002B_0.4n)";
    size_t new_size = unescape_json_i(json, strlen(json));

    INFO("json: " + std::string(json));
    REQUIRE(string_view(json, new_size) == "L+_0.4n");
}

TEST_CASE("Unescape a unicode escape, lowercase hex") {
    char json[] = R"(a\u002bb)";
    size_t new_size = unescape_json_i(json, strlen(json));

    REQUIRE(string_view(json, new_size) == "a+b");
}

TEST_CASE("Unescape a unicode escape to multi-byte UTF-8") {
    SECTION("two bytes") {
        char json[] = R"(caf\u00e9)";
        size_t new_size = unescape_json_i(json, strlen(json));
        REQUIRE(string_view(json, new_size) == "caf\xc3\xa9");
    }

    SECTION("three bytes") {
        char json[] = R"(\u20AC)";
        size_t new_size = unescape_json_i(json, strlen(json));
        REQUIRE(string_view(json, new_size) == "\xe2\x82\xac");
    }

    SECTION("surrogate pair") {
        char json[] = R"(\uD83D\uDE00)";
        size_t new_size = unescape_json_i(json, strlen(json));
        REQUIRE(string_view(json, new_size) == "\xf0\x9f\x98\x80");
    }
}

TEST_CASE("Unpaired surrogates become U+FFFD") {
    SECTION("high surrogate with no low one") {
        char json[] = R"(\uD83Dx)";
        size_t new_size = unescape_json_i(json, strlen(json));
        REQUIRE(string_view(json, new_size) == "\xef\xbf\xbdx");
    }

    SECTION("low surrogate with no high one") {
        char json[] = R"(\uDE00)";
        size_t new_size = unescape_json_i(json, strlen(json));
        REQUIRE(string_view(json, new_size) == "\xef\xbf\xbd");
    }
}

TEST_CASE("Unescape solidus") {
    // jsmn accepts \/ but special_chars has no entry for it, so it used to keep
    // its backslash - the same defect as \u002B, one escape over.
    char json[] = R"(a\/b)";
    size_t new_size = unescape_json_i(json, strlen(json));

    REQUIRE(string_view(json, new_size) == "a/b");
}

TEST_CASE("Malformed unicode escape keeps its backslash") {
    char json[] = R"(\uZZZZ)";
    size_t new_size = unescape_json_i(json, strlen(json));

    REQUIRE(string_view(json, new_size) == R"(\uZZZZ)");
}

TEST_CASE("Unescaping never grows and never invents a backslash") {
    // The escape input space is small enough to cover exhaustively, which is
    // worth more than any single case: the original defect was not a wrong
    // answer to a question anyone had asked, but an escape nobody had thought
    // of. These two properties hold for every one of them.
    size_t grew = 0;
    size_t invented = 0;

    for (uint32_t v = 0; v <= 0xFFFF; v++) {
        char json[16];
        snprintf(json, sizeof json, "a\\u%04Xb", v);
        const size_t in_size = strlen(json);
        const size_t out_size = unescape_json_i(json, in_size);

        // In-place rewriting is sound only while output cannot outgrow input.
        if (out_size > in_size) {
            grew++;
        }
        // A backslash downstream is a path separator, so the only escape
        // allowed to produce one is the one that actually means it.
        if (v != 0x005C && memchr(json, '\\', out_size) != nullptr) {
            invented++;
        }
    }

    REQUIRE(grew == 0);
    REQUIRE(invented == 0);
}
