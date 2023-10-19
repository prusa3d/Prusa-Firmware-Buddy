#include <stdx/cx_vector.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <type_traits>

TEST_CASE("empty vector", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 3> const v{};
    CHECK(0u == v.size());
    CHECK(v.empty());
    static_assert(3u == v.capacity());
}

TEST_CASE("CTAD", "[cx_vector]") {
    stdx::cx_vector v{1, 2, 3, 4, 5, 6};
    static_assert(std::is_same_v<decltype(v), stdx::cx_vector<int, 6>>);
}

TEST_CASE("mutable vector", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 6> v{0x13, 0x8, 0x43, 0x1024, 0xdeadbeef, 0x600};

    REQUIRE(6u == v.size());
    CHECK(0x13 == v[0]);
    CHECK(0x8 == v[1]);
    CHECK(0x43 == v[2]);
    CHECK(0x1024 == v[3]);
    CHECK(0xdeadbeef == v[4]);
    CHECK(0x600 == v[5]);

    v[2] = 0x55;
    CHECK(0x55 == v[2]);
    CHECK(0x55 == stdx::get<2>(v));
}

TEST_CASE("const vector", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 7> const v{0xFF0F, 0x1420, 0x5530};

    REQUIRE(3u == v.size());
    CHECK(0xFF0F == v[0]);
    CHECK(0x1420 == v[1]);
    CHECK(0x5530 == v[2]);
    CHECK(0x5530 == stdx::get<2>(v));
}

TEST_CASE("vector iterator", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 12> v{0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    for (auto &item : v) {
        ++item;
    }

    auto iter = v.begin();
    CHECK(0x12 == *iter);
    ++iter;
    REQUIRE(iter != v.end());
    CHECK(0x23 == *iter);
    ++iter;
    REQUIRE(iter != v.end());
    CHECK(0x34 == *iter);
    ++iter;
    REQUIRE(iter != v.end());
    CHECK(0x45 == *iter);
    ++iter;
    REQUIRE(iter != v.end());
    CHECK(0x56 == *iter);
    ++iter;
    REQUIRE(iter != v.end());
    CHECK(0x67 == *iter);
    ++iter;
    CHECK(iter == v.end());
}

TEST_CASE("const_iterator", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 7> const v{0x54, 0x70, 0x31};

    auto iter = v.begin();
    CHECK(0x54 == *iter);
    ++iter;
    REQUIRE(iter != v.end());
    CHECK(0x70 == *iter);
    ++iter;
    REQUIRE(iter != v.end());
    CHECK(0x31 == *iter);
    ++iter;
    CHECK(iter == v.end());
}

TEST_CASE("push_back", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 6> v{0x1, 0x12, 0x123, 0x1234};

    CHECK(4u == v.size());
    REQUIRE_FALSE(v.full());
    v.push_back(0x12345);
    REQUIRE(5u == v.size());
    CHECK(0x12345 == v[4]);
    REQUIRE_FALSE(v.full());
    v.push_back(0x123456);
    REQUIRE(6u == v.size());
    CHECK(0x123456 == v[5]);
    REQUIRE(v.full());
}

TEST_CASE("pop_back", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 14> v{0xA, 0xB, 0xC, 0xD, 0xE, 0xF};

    CHECK(0xF == v.pop_back());
    CHECK(5u == v.size());
    REQUIRE_FALSE(v.empty());
    CHECK(0xE == v.pop_back());
    CHECK(0xD == v.pop_back());
    CHECK(0xC == v.pop_back());
    CHECK(0xB == v.pop_back());
    REQUIRE_FALSE(v.empty());
    CHECK(0xA == v.pop_back());
    REQUIRE(v.empty());
}

TEST_CASE("front", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 4> v{};
    v.push_back(1);
    v.push_back(2);
    CHECK(v.front() == 1);
}

TEST_CASE("back", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 4> v{};
    v.push_back(1);
    v.push_back(2);
    CHECK(v.back() == 2);
}

TEST_CASE("equality", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 10> a{4, 9, 2, 3, 1};
    stdx::cx_vector<uint32_t, 10> b{4, 9, 2, 3, 1};

    stdx::cx_vector<uint32_t, 10> c{4, 9, 2, 3, 1, 0};
    stdx::cx_vector<uint32_t, 10> d{4, 9, 2, 3};
    stdx::cx_vector<uint32_t, 10> e{1, 9, 2, 3, 1};

    CHECK(a == b);
    CHECK_FALSE(a == c);
    CHECK_FALSE(a == d);
    CHECK_FALSE(a == e);
}

TEST_CASE("inequality", "[cx_vector]") {
    stdx::cx_vector<uint32_t, 10> a{4, 9, 2, 3, 1};
    stdx::cx_vector<uint32_t, 10> b{4, 9, 2, 3, 1};

    stdx::cx_vector<uint32_t, 10> c{4, 9, 2, 3, 1, 0};
    stdx::cx_vector<uint32_t, 10> d{4, 9, 2, 3};
    stdx::cx_vector<uint32_t, 10> e{1, 9, 2, 3, 1};

    CHECK_FALSE(a != b);
    CHECK(a != c);
    CHECK(a != d);
    CHECK(a != e);
}

TEST_CASE("zero capacity", "[cx_vector]") {
    stdx::cx_vector<int, 0> const v{};
    CHECK(0u == v.size());
    static_assert(0u == v.capacity());
    CHECK(v.empty());
    CHECK(v.full());
    CHECK(v.begin() == v.end());
    CHECK(v == stdx::cx_vector<int, 0>{});
}

TEST_CASE("resize_and_overwrite", "[cx_vector]") {
    stdx::cx_vector<int, 5> v{1, 2, 3, 4, 5};
    resize_and_overwrite(v, [](int *dest, std::size_t max_size) {
        CHECK(max_size == 5);
        *dest = 42;
        return 1u;
    });
    REQUIRE(v.size() == 1u);
    CHECK(v[0] == 42);
}
