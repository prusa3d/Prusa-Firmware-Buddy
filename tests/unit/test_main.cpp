#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

TEST_CASE("type tests", "[system]") {
    REQUIRE(sizeof(uint64_t) == 8);
}
