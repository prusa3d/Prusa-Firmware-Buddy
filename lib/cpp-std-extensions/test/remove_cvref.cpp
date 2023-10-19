#include <stdx/type_traits.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("remove_cvref", "[type_traits]") {
    REQUIRE(std::is_same_v<stdx::remove_cvref_t<int>, int>);
    REQUIRE(std::is_same_v<stdx::remove_cvref_t<int const>, int>);
    REQUIRE(std::is_same_v<stdx::remove_cvref_t<int &>, int>);
    REQUIRE(std::is_same_v<stdx::remove_cvref_t<int const &>, int>);
    REQUIRE(std::is_same_v<stdx::remove_cvref_t<int &&>, int>);
    REQUIRE(std::is_same_v<stdx::remove_cvref_t<int const &&>, int>);
}
