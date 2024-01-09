#include "dummy_connection.hpp"

#include <http/buffered_conn.hpp>

#include <catch2/catch.hpp>

using namespace http;

TEST_CASE("Buffered connection") {
    DummyConnection inner_conn;
    BufferedConnection buffered(&inner_conn, reinterpret_cast<const uint8_t *>("hello "), 6);

    REQUIRE(buffered.poll_readable(0));
    REQUIRE_FALSE(inner_conn.poll_readable(0));

    REQUIRE(get<size_t>(buffered.tx(reinterpret_cast<const uint8_t *>("xxx"), 3)) == 3);
    REQUIRE(inner_conn.sent == "xxx");

    uint8_t buffer[50];
    REQUIRE(get<size_t>(buffered.rx(buffer, 3, true)) == 3);
    REQUIRE(memcmp(buffer, "hel", 3) == 0);

    REQUIRE(buffered.poll_readable(0));
    inner_conn.received = "world";

    REQUIRE(get<size_t>(buffered.rx(buffer, 50, true)) == 3);
    REQUIRE(memcmp(buffer, "lo ", 3) == 0);

    REQUIRE(buffered.poll_readable(0));

    REQUIRE(get<size_t>(buffered.rx(buffer, 50, true)) == 5);
    REQUIRE(memcmp(buffer, "world", 5) == 0);

    REQUIRE_FALSE(buffered.poll_readable(0));
}
