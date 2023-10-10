#include <stdx/ct_string.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
template <typename T, T...> struct string_constant {};
} // namespace

TEST_CASE("construction", "[ct_string]") {
    [[maybe_unused]] constexpr auto s = stdx::ct_string{"ABC"};
}

TEST_CASE("UDL", "[ct_string]") {
    using namespace stdx::ct_string_literals;
    [[maybe_unused]] constexpr auto s = "ABC"_cts;
}

TEST_CASE("empty", "[ct_string]") {
    constexpr auto s1 = stdx::ct_string{""};
    static_assert(s1.empty());
    constexpr auto s2 = stdx::ct_string{"A"};
    static_assert(not s2.empty());
}

TEST_CASE("size", "[ct_string]") {
    constexpr auto s = stdx::ct_string{"ABC"};
    static_assert(s.size() == 3u);
    static_assert(not s.empty());
}

TEST_CASE("equality", "[ct_string]") {
    constexpr auto s1 = stdx::ct_string{"ABC"};
    constexpr auto s2 = stdx::ct_string{"ABC"};
    constexpr auto s3 = stdx::ct_string{"ABD"};
    static_assert(s1 == s2);
    static_assert(s1 != s3);
}

TEST_CASE("explicit length construction", "[ct_string]") {
    constexpr auto s = stdx::ct_string<3u>{"ABC", 2};
    static_assert(s == stdx::ct_string{"AB"});
}

TEST_CASE("from type", "[ct_string]") {
    using T = string_constant<char, 'A', 'B', 'C'>;
    constexpr auto s = stdx::ct_string_from_type(T{});
    static_assert(s == stdx::ct_string{"ABC"});
}

TEST_CASE("to type", "[ct_string]") {
    constexpr auto s = stdx::ct_string{"ABC"};
    constexpr auto sc = stdx::ct_string_to_type<s, string_constant>();
    static_assert(std::is_same_v<decltype(sc),
                                 string_constant<char, 'A', 'B', 'C'> const>);
}

TEST_CASE("to string_view", "[ct_string]") {
    constexpr auto s = stdx::ct_string{"ABC"};
    auto const sv = static_cast<std::string_view>(s);
    CHECK(sv == "ABC");
}

TEST_CASE("string split (character present)", "[ct_string]") {
    constexpr auto s = stdx::ct_string{"A.B"};
    constexpr auto p = stdx::split<s, '.'>();
    static_assert(p.first == stdx::ct_string{"A"});
    static_assert(p.second == stdx::ct_string{"B"});
}

TEST_CASE("string split (character not present)", "[ct_string]") {
    constexpr auto s = stdx::ct_string{"A"};
    constexpr auto p = stdx::split<s, '.'>();
    static_assert(p.first == stdx::ct_string{"A"});
    static_assert(p.second.empty());
}
