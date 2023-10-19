#include <stdx/type_traits.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

namespace {
enum UnscopedEnum { Value3 = 3 };
enum UnscopedEnumWithUnderlying : short int { Value4 = 4 };
enum struct ScopedEnum : char { Value5 = 5 };
} // namespace

TEST_CASE("to_underlying types", "[type_traits]") {
    REQUIRE(std::is_same_v<decltype(stdx::to_underlying(Value3)),
                           std::underlying_type_t<UnscopedEnum>>);
    REQUIRE(std::is_same_v<decltype(stdx::to_underlying(Value4)),
                           std::underlying_type_t<UnscopedEnumWithUnderlying>>);
    REQUIRE(std::is_same_v<decltype(stdx::to_underlying(ScopedEnum::Value5)),
                           std::underlying_type_t<ScopedEnum>>);
}

TEST_CASE("to_underlying values", "[type_traits]") {
    REQUIRE(stdx::to_underlying(Value3) == 3);
    REQUIRE(stdx::to_underlying(Value4) == 4);
    REQUIRE(stdx::to_underlying(ScopedEnum::Value5) == 5);
}
