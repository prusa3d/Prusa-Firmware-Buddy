#include <stdx/utility.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("compile-time named bools", "[utility]") {
    using namespace stdx::literals;
    static_assert("variable"_b);
    static_assert(not(not "variable"_b));

    static_assert("variable"_true);
    static_assert(not "variable"_false);
}

TEST_CASE("decimal units", "[units]") {
    using namespace stdx::literals;
    static_assert(1_k == 1'000ull);
    static_assert(1_M == 1'000'000ull);
    static_assert(1_G == 1'000'000'000ull);
}

TEST_CASE("binary units", "[units]") {
    using namespace stdx::literals;
    static_assert(1_ki == 1'024ull);
    static_assert(1_Mi == 1'024ull * 1'024ull);
    static_assert(1_Gi == 1'024ull * 1'024ull * 1'024ull);
}
