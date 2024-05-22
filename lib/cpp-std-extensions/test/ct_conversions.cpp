#include <stdx/ct_conversions.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

struct complete {};
struct incomplete;

TEST_CASE("type as string", "[ct_conversion]") {
    using namespace std::string_view_literals;
    static_assert(stdx::type_as_string<int>() == "int"sv);
    static_assert(stdx::type_as_string<complete>() == "complete"sv);
    static_assert(stdx::type_as_string<incomplete>() == "incomplete"sv);
}

namespace {
enum A { X };
enum struct B { Y };
} // namespace

TEST_CASE("enum as string", "[ct_conversion]") {
    using namespace std::string_view_literals;
    static_assert(stdx::enum_as_string<X>() == "X"sv);
    static_assert(stdx::enum_as_string<B::Y>() == "Y"sv);
}
