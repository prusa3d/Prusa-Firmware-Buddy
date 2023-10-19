#include "detail/tuple_types.hpp"

#include <stdx/for_each_n_args.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("default to unary function", "[for_each_n_args]") {
    auto sum = 0;
    auto f = [&](int x) { sum += x; };
    stdx::for_each_n_args(f, 1, 2, 3);
    CHECK(sum == 6);
}

TEST_CASE("binary function", "[for_each_n_args]") {
    auto sum = 0;
    auto f = [&](int x, int y) { sum += x * y; };
    stdx::for_each_n_args<2>(f, 2, 3, 3, 4);
    CHECK(sum == 18);
}

TEST_CASE("generic function", "[for_each_n_args]") {
    auto sum = 0;
    auto f = [&](auto x, auto y) { sum += x * y; };
    stdx::for_each_n_args<2>(f, 2, 3, 3, 4);
    CHECK(sum == 18);
}

TEST_CASE("move-only arguments", "[for_each_n_args]") {
    auto sum = 0;
    auto f = [&](move_only x) { sum += x.value; };
    stdx::for_each_n_args(f, move_only{1}, move_only{2}, move_only{3});
    CHECK(sum == 6);
}

TEST_CASE("default arguments", "[for_each_n_args]") {
    auto sum = 0;
    auto f = [&](int x, int y, int z = 42) { sum += x * y + z; };
    stdx::for_each_n_args<2>(f, 2, 3, 3, 4);
    CHECK(sum == 42 + 42 + 18);
}
