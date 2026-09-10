#include <path_utils.h>

#include <catch2/catch_test_macros.hpp>
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

TEST_CASE("Path allowed accepts usb paths") {
    REQUIRE(path_allowed("/usb"));
    REQUIRE(path_allowed("/usb/x"));
}

TEST_CASE("Path allowed rejects traversal and non-usb paths") {
    REQUIRE(!path_allowed("/usb/../y"));
    REQUIRE(!path_allowed("/other"));
    REQUIRE(!path_allowed("../usb/x"));
}

TEST_CASE("Path allowed known gap: bare trailing /usb/.. is accepted") {
    // Not a fix target here - path_allowed only rejects the "/../" substring,
    // not a traversal segment at the very end of the path.
    REQUIRE(path_allowed("/usb/.."));
}
