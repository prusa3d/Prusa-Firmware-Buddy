#include <stdx/type_traits.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("conditional", "[type_traits]") {
    REQUIRE(std::is_same_v<stdx::conditional_t<true, int, float>, int>);
    REQUIRE(std::is_same_v<stdx::conditional_t<false, int, float>, float>);
}
