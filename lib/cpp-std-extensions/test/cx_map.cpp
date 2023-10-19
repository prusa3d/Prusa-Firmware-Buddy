#include <stdx/cx_map.hpp>

#include <catch2/catch_test_macros.hpp>

#include <iterator>

TEST_CASE("empty and size", "[cx_map]") {
    auto m = stdx::cx_map<int, int, 64>{};
    CHECK(m.size() == 0);
    CHECK(m.empty());

    CHECK(m.insert_or_assign(10, 50));
    CHECK(m.size() == 1);
    CHECK(not m.empty());
}

TEST_CASE("contains and get", "[cx_map]") {
    auto m = stdx::cx_map<int, int, 64>{};
    CHECK(m.put(10, 50));
    CHECK(m.put(11, 100));

    REQUIRE(m.contains(10));
    CHECK(m.get(10) == 50);
    REQUIRE(m.contains(11));
    CHECK(m.get(11) == 100);
    CHECK(not m.contains(12));
}

TEST_CASE("const get", "[cx_map]") {
    auto const t = [] {
        auto m = stdx::cx_map<int, int, 64>{};
        m.put(10, 50);
        m.put(11, 100);
        return m;
    }();

    REQUIRE(t.contains(10));
    CHECK(t.get(10) == 50);
    REQUIRE(t.contains(11));
    CHECK(t.get(11) == 100);
    CHECK(not t.contains(12));
}

TEST_CASE("update existing key", "[cx_map]") {
    auto t = stdx::cx_map<int, int, 64>{};
    t.put(13, 500);
    t.put(13, 700);

    REQUIRE(t.contains(13));
    CHECK(t.get(13) == 700);
}

TEST_CASE("pop_back", "[cx_map]") {
    stdx::cx_map<int, int, 64> t;
    t.put(13, 500);

    auto const entry = t.pop_back();
    CHECK(t.size() == 0);
    CHECK(t.empty());
    CHECK(entry.key == 13);
    CHECK(entry.value == 500);
}

TEST_CASE("erase", "[cx_map]") {
    stdx::cx_map<int, int, 64> t;
    t.put(13, 500);
    t.put(18, 600);
    t.put(19, 700);

    CHECK(t.erase(18) == 1u);
    CHECK(t.size() == 2);
    CHECK(not t.contains(18));
    CHECK(t.get(13) == 500);
    CHECK(t.get(19) == 700);
}

TEST_CASE("erase non existant key", "[cx_map]") {
    stdx::cx_map<int, int, 64> t;
    t.put(13, 500);
    t.put(18, 600);

    CHECK(t.erase(50) == 0u);
    CHECK(t.size() == 2);
    CHECK(not t.contains(50));
    CHECK(t.get(13) == 500);
    CHECK(t.get(18) == 600);
}

TEST_CASE("empty iterators", "[cx_map]") {
    stdx::cx_map<int, int, 64> t;
    CHECK(t.begin() == t.end());
}

TEST_CASE("non-empty iterators", "[cx_map]") {
    stdx::cx_map<int, int, 64> t;
    t.put(18, 600);
    CHECK(std::next(t.begin()) == t.end());
}

TEST_CASE("constexpr empty and size", "[cx_map]") {
    constexpr auto m = stdx::cx_map<int, int, 64>{};
    static_assert(m.size() == 0);
    static_assert(m.empty());
}

TEST_CASE("constexpr populated map", "[cx_map]") {
    constexpr auto m = [] {
        stdx::cx_map<int, int, 64> t;
        t.put(10, 50);
        t.put(11, 100);
        return t;
    }();

    static_assert(not m.empty());
    static_assert(m.contains(10));
    static_assert(m.get(10) == 50);
    static_assert(m.contains(11));
    static_assert(m.get(11) == 100);
    static_assert(not m.contains(12));
}

TEST_CASE("constexpr erase", "[cx_map]") {
    constexpr auto m = [] {
        stdx::cx_map<int, int, 64> t;
        t.put(10, 50);
        t.put(11, 100);
        t.erase(11);
        t.erase(12);
        return t;
    }();

    static_assert(not m.empty());
    static_assert(m.contains(10));
    static_assert(m.get(10) == 50);
    static_assert(not m.contains(11));
}

TEST_CASE("constexpr update value", "[cx_map]") {
    constexpr auto m = [] {
        stdx::cx_map<int, int, 64> t;
        t.put(10, 50);
        t.put(10, 100);
        return t;
    }();

    static_assert(not m.empty());
    static_assert(m.contains(10));
    static_assert(m.get(10) == 100);
}

TEST_CASE("constexpr erase first", "[cx_map]") {
    constexpr auto m = [] {
        stdx::cx_map<int, int, 64> t;
        t.put(10, 50);
        t.put(11, 100);
        t.erase(10);
        return t;
    }();

    static_assert(not m.empty());
    static_assert(m.contains(11));
    static_assert(m.get(11) == 100);
    static_assert(not m.contains(10));
}
