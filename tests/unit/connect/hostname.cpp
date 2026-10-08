#include <hostname.hpp>

#include <catch2/catch_test_macros.hpp>
#include <cstring>

using namespace connect_client;

constexpr const char *uncompressed = "uncompressed.example.com";
constexpr const char *compressed = "whatever.connect.prusa3d.com";

TEST_CASE("connect::Host not compressed", "[connect]") {
    char buffer[30];

    REQUIRE(compress_host(uncompressed, buffer, sizeof buffer));
    REQUIRE(strcmp(uncompressed, buffer) == 0);

    decompress_host(buffer, sizeof buffer);
    REQUIRE(strcmp(uncompressed, buffer) == 0);
}

TEST_CASE("connect::Host doesn't fit, not compressed", "[connect]") {
    char buffer[10];

    REQUIRE_FALSE(compress_host(uncompressed, buffer, sizeof buffer));
}

TEST_CASE("connect::Host doesn't fit after compression", "[connect]") {
    // The compressed shall be 11 chars + \0, so this overflows by exactly 1
    char buffer[11];
    REQUIRE_FALSE(compress_host(compressed, buffer, sizeof buffer));
}

TEST_CASE("connect::Host compressed", "[connect]") {
    // This shall be an exact fit.
    char buffer[12];

    REQUIRE(compress_host(compressed, buffer, sizeof buffer));
    // The compressed representation is "valid" string, with \0 at the end.
    REQUIRE(strlen(buffer) < sizeof buffer);

    char decompress_buffer[30];
    memcpy(decompress_buffer, buffer, sizeof buffer);

    // An attempt to decompress without giving it space to expand shall fail (silently, shall do nothing).
    decompress_host(decompress_buffer, sizeof buffer);
    REQUIRE(memcmp(decompress_buffer, buffer, sizeof buffer) == 0);

    // When we give it enough space, it'll expand
    decompress_host(decompress_buffer, sizeof decompress_buffer);
    REQUIRE(strcmp(compressed, decompress_buffer) == 0);
}

TEST_CASE("connect::Looks like compressed", "[connect]") {
    // This things looks like a compressed thing, but isn't (index out of range).
    const char *impostor = "impostor.\x01X";

    char buffer[30];
    strcpy(buffer, impostor);
    decompress_host(buffer, sizeof buffer);
    // Nothing changed.
    REQUIRE(strcmp(buffer, impostor) == 0);
}
