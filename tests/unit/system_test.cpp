#include "catch2/catch.hpp"
#include <functional>

TEST_CASE("type tests", "[system]") {
    REQUIRE(sizeof(uint64_t) == 8);
    REQUIRE(sizeof(char *) == 4);
}

std::uint32_t foo() { return 10; };
std::uint32_t bar(std::uint32_t i) { return i; };

using oldschool = std::uint32_t (*)();
using oldschool_param = std::uint32_t (*)(std::uint32_t);

using better = std::function<std::uint32_t()>;
using better_param = std::function<std::uint32_t(std::uint32_t)>;

TEST_CASE("callbacks", "[system]") {
    SECTION("classic usage") {
        oldschool pfoo = foo;
        oldschool_param pbar = bar;
        better pf = foo;
        better_param pb = bar;
        REQUIRE(pfoo() == 10);
        REQUIRE(pbar(1) == 1);
        REQUIRE(pf() == 10);
        REQUIRE(pb(100) == 100);
    }

    SECTION("lambda usage") {
        // oldschool pfoo = [](){return 7;};            //compile error: invalid user-defined conversion
        // oldschool_param pbar= [](std::uint32_t d) {return d;}; //compile error: invalid user-defined conversion
        better pf = []() { return 7; };
        better_param pb = [](std::uint32_t d) { return d; };
        REQUIRE(pf() == 7);
        REQUIRE(pb(100) == 100);
    }
}
