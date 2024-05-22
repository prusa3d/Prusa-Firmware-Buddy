#include <stdx/utility.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("overload set without captures", "[utility]") {
    auto const f =
        stdx::overload{[](int) { return 1; }, [](char) { return 'X'; }};
    REQUIRE(f(1) == 1);
    REQUIRE(f('a') == 'X');
}

TEST_CASE("overload set with captures", "[utility]") {
    auto const i = 1;
    auto const c = 'X';
    auto const f =
        stdx::overload{[&](int) { return i; }, [&](char) { return c; }};
    REQUIRE(f(1) == 1);
    REQUIRE(f('a') == 'X');
}
