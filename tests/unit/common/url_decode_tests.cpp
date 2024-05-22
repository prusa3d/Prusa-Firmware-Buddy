#include "catch2/catch.hpp"
#include <http/url_decode.h>

#include <string>
#include <cstring>

using http::url_decode;

TEST_CASE("url decode", "[url]") {
    std::string common_url = "/some/random/test/url";

    SECTION("decoding weird chars") {
        std::string url = "/a/v/f/%21%40%23%24%25%5E%26%2A%28%29%5B%5D%20%7B%7D";
        char decoded_url[url.size()];
        REQUIRE(url_decode(url, decoded_url, sizeof(decoded_url)));
        REQUIRE(strcmp(decoded_url, "/a/v/f/!@#$%^&*()[] {}") == 0);
    }

    SECTION("not a number after %") {
        std::string url = "/a/v/f/%^7e)";
        char decoded_url[url.size()];
        REQUIRE_FALSE(url_decode(url, decoded_url, sizeof(decoded_url)));
    }

    SECTION("incomplete escape sequence") {
        std::string url = "/a/v/f/%8";
        char decoded_url[url.size()];
        REQUIRE_FALSE(url_decode(url, decoded_url, sizeof(decoded_url)));
    }

    SECTION("short buffer") {
        char decoded_url[1];
        REQUIRE_FALSE(url_decode(common_url, decoded_url, sizeof(decoded_url)));
    }

    SECTION("zero length buffer") {
        char decoded_url[0];
        REQUIRE_FALSE(url_decode(common_url, decoded_url, sizeof(decoded_url)));
    }

    SECTION("empty url") {
        std::string url = "";
        char decoded_url[100];
        REQUIRE(url_decode(url, decoded_url, sizeof(decoded_url)));
        REQUIRE(strcmp(decoded_url, "") == 0);
    }
}
