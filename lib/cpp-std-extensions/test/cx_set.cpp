#include <stdx/cx_set.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("empty and size", "[cx_set]") {
    auto s = stdx::cx_set<int, 64>{};
    CHECK(s.size() == 0);
    CHECK(s.empty());

    constexpr auto cs = stdx::cx_set<int, 64>{};
    static_assert(cs.size() == 0);
    static_assert(cs.empty());
}

TEST_CASE("CTAD", "[cx_set]") {
    stdx::cx_set set{1, 2, 3, 4, 5, 6};
    static_assert(std::is_same_v<decltype(set), stdx::cx_set<int, 6>>);
}

TEST_CASE("contains", "[cx_set]") {
    stdx::cx_set<int, 64> t;

    CHECK(not t.contains(10));

    t.insert(10);

    CHECK(t.size() == 1);
    CHECK(not t.empty());
    CHECK(t.contains(10));
    CHECK(not t.contains(11));
}

TEST_CASE("create", "[cx_set]") {
    stdx::cx_set<int, 64> t{1, 2, 3};

    CHECK(t.size() == 3);
    CHECK(not t.empty());
    CHECK(t.contains(1));
    CHECK(t.contains(2));
    CHECK(t.contains(3));
    CHECK(not t.contains(11));
}

TEST_CASE("insert multiple", "[cx_set]") {
    stdx::cx_set<int, 64> t;

    t.insert(10);
    t.insert(10);

    CHECK(t.size() == 1);
    CHECK(not t.empty());
    CHECK(t.contains(10));
}

TEST_CASE("erase all", "[cx_set]") {
    stdx::cx_set<int, 64> t;

    t.insert(10);
    t.erase(10);

    CHECK(t.size() == 0);
    CHECK(t.empty());
    CHECK(not t.contains(10));
}

TEST_CASE("erase some", "[cx_set]") {
    stdx::cx_set<int, 64> t;

    t.insert(10);
    t.insert(11);
    t.erase(10);

    CHECK(t.size() == 1);
    CHECK(not t.empty());
    CHECK(not t.contains(10));
    CHECK(t.contains(11));
}

TEST_CASE("constexpr populated set", "[cx_set]") {
    constexpr auto populatedSetTest = [] {
        stdx::cx_set<int, 64> t;
        t.insert(10);
        t.insert(10);
        return t;
    }();

    static_assert(not populatedSetTest.empty());
    static_assert(populatedSetTest.contains(10));
    static_assert(not populatedSetTest.contains(11));
}

TEST_CASE("constexpr erase", "[cx_set]") {
    constexpr auto testSetRemove = [] {
        stdx::cx_set<int, 64> t{10, 11, 12, 32, 40, 42, 56};
        t.erase(10);
        t.erase(11);
        t.erase(11);
        t.erase(32);
        t.erase(56);
        t.erase(56);
        return t;
    }();

    static_assert(not testSetRemove.empty());
    static_assert(testSetRemove.contains(12));
    static_assert(testSetRemove.contains(40));
    static_assert(testSetRemove.contains(42));

    static_assert(not testSetRemove.contains(10));
    static_assert(not testSetRemove.contains(11));
    static_assert(not testSetRemove.contains(32));
    static_assert(not testSetRemove.contains(56));
}
