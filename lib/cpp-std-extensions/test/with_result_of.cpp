#include <stdx/functional.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("explicit conversion", "[with_result_of]") {
    constexpr auto f = stdx::with_result_of{[]() { return 42; }};
    static_assert(std::is_empty_v<std::decay_t<decltype(f)>>);
    constexpr auto result = static_cast<int>(f);
    static_assert(result == 42);
}

TEST_CASE("implicit conversion (lvalue)", "[with_result_of]") {
    constexpr auto f = stdx::with_result_of{[]() { return 42; }};
    static_assert(std::is_empty_v<std::decay_t<decltype(f)>>);
    constexpr auto result = [](int n) { return n; }(f);
    static_assert(result == 42);
}

TEST_CASE("implicit conversion (rvalue)", "[with_result_of]") {
    constexpr auto result = [](int n) {
        return n;
    }(stdx::with_result_of{[]() { return 42; }});
    static_assert(result == 42);
}

TEST_CASE("capturing lambda", "[with_result_of]") {
    auto value = 42;
    auto const result = [](int n) {
        return n;
    }(stdx::with_result_of{[&]() { return value; }});
    CHECK(result == value);
}
