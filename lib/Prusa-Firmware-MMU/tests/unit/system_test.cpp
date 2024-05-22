#include "catch2/catch_test_macros.hpp"

TEST_CASE("type tests", "[system]") {
    REQUIRE(sizeof(uint64_t) == 8);
}
