#include <stdx/type_traits.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
constexpr auto f() -> int {
    if (stdx::is_constant_evaluated()) {
        return 42;
    } else {
        return 0;
    }
}
} // namespace

TEST_CASE("constexpr context", "[is_constant_evaluated]") {
    constexpr auto n = f();
    CHECK(n == 42);
}

TEST_CASE("runtime context", "[is_constant_evaluated]") {
    auto n = f();
    CHECK(n == 0);
}
