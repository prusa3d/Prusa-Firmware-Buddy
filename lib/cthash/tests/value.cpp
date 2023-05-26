#include <cthash/value.hpp>
#include <sstream>
#include <catch2/catch_test_macros.hpp>

using namespace cthash::literals;

TEST_CASE("hash_value (constexpr basics)") {
    constexpr auto v1 = cthash::hash_value { "0011223300112233" };
    constexpr auto v2 = cthash::hash_value { "00112233aabbccdd" };

    STATIC_REQUIRE((v1 <=> v1) == 0); // appleclang doesn't have std::is_eq
    STATIC_REQUIRE(v1 == v1);
    STATIC_REQUIRE(v1 < v2);
    STATIC_REQUIRE(v2 > v1);
    STATIC_REQUIRE(v1 != v2);

    constexpr auto v3 = cthash::hash_value { u8"00112233aabbccdd" };

    STATIC_REQUIRE(v1 != v2);
    STATIC_REQUIRE(v2 == v3);

    [[maybe_unused]] constexpr auto v4 = "599ba25a0d7c7d671bee93172ca7e272fc87f0c0e02e44df9e9436819067ea28"_hash;
    constexpr auto v5 = "00112233aabbccdd"_hash;

    STATIC_REQUIRE(v5 == v3);

    // constexpr bool comparable = requires(cthash::hash_value<8> l, cthash::hash_value<4> r) { v1 == v2; };
}

TEST_CASE("hash_value (runtime basics)") {
    auto v1 = cthash::hash_value { "0011223300112233" };
    auto v2 = cthash::hash_value { "00112233aabbccdd" };

    REQUIRE((v1 <=> v1) == 0); // appleclang doesn't have std::is_eq
    REQUIRE(v1 == v1);
    REQUIRE(v1 < v2);
    REQUIRE(v2 > v1);
    REQUIRE(v1 != v2);

    auto v3 = cthash::hash_value { u8"00112233aabbccdd" };

    REQUIRE(v1 != v2);
    REQUIRE(v2 == v3);

    [[maybe_unused]] auto v4 = "599ba25a0d7c7d671bee93172ca7e272fc87f0c0e02e44df9e9436819067ea28"_hash;
    auto v5 = "00112233aabbccdd"_hash;

    REQUIRE(v5 == v3);

    // constexpr bool comparable = requires(cthash::hash_value<8> l, cthash::hash_value<4> r) { v1 == v2; };
}

auto convert_to_string(auto &&val) {
    std::ostringstream os;
    os << val;
    return std::move(os).str();
}

TEST_CASE("hash stringification") {
    auto v1 = cthash::hash_value { "00112233aabbccdd" };
    REQUIRE(convert_to_string(v1) == "00112233aabbccdd");
}
