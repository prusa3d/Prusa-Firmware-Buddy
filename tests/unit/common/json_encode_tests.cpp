#include <json_encode.h>

#include <catch2/catch.hpp>
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

TEST_CASE("json escape bytes") {
    {
        const char *src = "abcdefhadg_";
        char out[256];
        REQUIRE(json_escape_bytes(src, out, 256));
        REQUIRE(!strcmp(src, out)); // Standard ASCII characters - no change
    }

    {
        const char *src = "abčíáýšíčř€$!_";
        char out[256];
        REQUIRE(json_escape_bytes(src, out, 256));

        std::string outstr = out;
        std::stringstream hex;

        for (const auto &ch : outstr) {
            hex << int(ch) << "|";
        }

        INFO("encoded: " + std::string(out) + " " + hex.str());
        REQUIRE(strcmp(src, out)); // Escaping should not look the same
    }

    {
        // pair(str, should there something be encoded?)
        std::vector<std::pair<const char *, bool>> data = {
            { "abcdef", false },
            { "étíáščusdk gsdgs", true },
            { "sdg sdfkjsd flwčutšwčítelrj", true },
            { "koůfgsd§qwrw§ůre$€sfsf\\__˘^^_", true }
        };
        for (const auto &rec : data) {
            INFO(rec.first);

            const bool src_has_encodable_characters = rec.second;
            char src[256];
            strcpy(src, rec.first);
            const auto src_len = strlen(src);

            char enc[512];
            memset(enc, 77, sizeof(enc)); // Fill control bytes in the buffer

            {
                // Escaping should never fit in the buffer of src_len - we don't have space for the terminating null
                REQUIRE(!json_escape_bytes(src, enc, src_len));

                // Check that the failed escape did not overwrite any bytes it shouldn't have
                REQUIRE(enc[src_len] == 77);
                REQUIRE(enc[src_len + 1] == 77);
            }

            {
                // If there are characters to be encoded, the string should also not fit in the orig src_len
                REQUIRE(json_escape_bytes(src, enc, src_len + 1) == !src_has_encodable_characters);

                if (!src_has_encodable_characters) {
                    REQUIRE(json_escape_bytes(src, enc, sizeof(enc)));
                }

                // Check that the failed escape did not overwrite any bytes it shouldn't have
                REQUIRE(enc[src_len + 1] == 77);
                REQUIRE(enc[src_len + 2] == 77);
            }

            // Now a proper escape should work
            REQUIRE(json_escape_bytes(src, enc, sizeof(enc)));

            char deenc[256];
            REQUIRE(json_unescape_bytes(enc, deenc));
            REQUIRE(!strcmp(src, deenc)); // De-encoding should yield the same results

            REQUIRE(!strcmp(src, rec.first)); // None of what we did should change the source
        }
    }
}
