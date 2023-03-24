#include "catch2/catch.hpp"

TEST_CASE("type tests", "[system]") {
    REQUIRE(sizeof(uint64_t) == 8);
}
