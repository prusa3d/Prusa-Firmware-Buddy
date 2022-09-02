#include "catch2/catch.hpp"
#include "hash_table.hpp"
#include <set>

TEST_CASE("Simple test") {
    HashMap<10> set;
    set.Set(1, 100);
    REQUIRE(set.Get(1).has_value());
    REQUIRE(set.Get(1).value() == 100);
}
TEST_CASE("Test collisions and deletions") {
    HashMap<10> set;
    set.Set(1, 100);
    set.Set(11, 1000);
    set.Set(21, 10000);
    REQUIRE(set.Get(1).has_value());
    REQUIRE(set.Get(1).value() == 100);
    REQUIRE(set.Get(11).has_value());
    REQUIRE(set.Get(11).value() == 1000);
    REQUIRE(set.Get(21).has_value());
    REQUIRE(set.Get(21).value() == 10000);
    REQUIRE(!set.Get(31).has_value());
    set.remove(11);
    REQUIRE(!set.Get(11).has_value());
    REQUIRE(set.Get(21).has_value());
    REQUIRE(set.Get(21).value() == 10000);
}
TEST_CASE("Test iterator") {
    std::set<std::pair<uint32_t, int32_t>> data { std::pair<uint32_t, int32_t> { 10, 100 }, std::pair<uint32_t, int32_t> { 11, 101 }, std::pair<uint32_t, int32_t> { 12, 102 } };
    HashMap<10> set;
    // empty set
    REQUIRE(set.begin() == set.end());
    for (const auto &[key, value] : data) {
        REQUIRE(!set.Set(key, value));
    }

    for (const auto &[key, value] : data) {
        REQUIRE(set.Get(key).has_value());
        REQUIRE(set.Get(key).value() == value);
    }
    size_t count = 0;
    for (auto &elem : set) {
        REQUIRE(data.count(elem) == 1);
        count++;
    }
    REQUIRE(count == data.size());
}
TEST_CASE("Full hash map") {
    HashMap<1> set;
    set.Set(10, 10);
    REQUIRE(set.Get(10).has_value());
    REQUIRE(set.Get(10).value() == 10);

    REQUIRE_THROWS_AS(set.Set(11, 11), std::runtime_error);
}
