#include <stdx/type_traits.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("always_false", "[type_traits]") {
    REQUIRE(not stdx::always_false_v<void>);
    REQUIRE(not stdx::always_false_v<void, int, bool>);
}
