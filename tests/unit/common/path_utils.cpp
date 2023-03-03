#include <path_utils.h>

#include <catch2/catch.hpp>
#include <string_view>

using std::string_view;

TEST_CASE("Dedup slashes") {
    char with_slashes[] = "//hello/world////extra/sl/ashes/";
    dedup_slashes(with_slashes);
    REQUIRE(string_view("/hello/world/extra/sl/ashes/") == with_slashes);
}

TEST_CASE("Dedup empty slashes") {
    char empty[] = "";
    dedup_slashes(empty);
    REQUIRE(string_view("") == empty);
}

TEST_CASE("Dedup without dup slashes") {
    char without_slashes[] = "well/whatever/ok";
    dedup_slashes(without_slashes);
    REQUIRE(string_view("well/whatever/ok") == without_slashes);
}
