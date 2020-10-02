#include "catch2/catch.hpp"

#include <float.h>
#include <inttypes.h>

union float_int_t {
    float f;
    uint32_t i;
};

TEST_CASE("type tests", "[system]") {
    REQUIRE(sizeof(uint64_t) == 8);
    REQUIRE(sizeof(char *) == 4);
};

TEST_CASE("float representation", "[system]") {
    float_int_t var;
    uint64_t storage = 0ull;
    var.f = 0.001f;
    storage = static_cast<uint64_t>(var.i) << 32;
    var.f = FLT_MAX;

    REQUIRE(storage != 0ull);
    REQUIRE(var.f != Approx(0.001f));
    var.i = storage >> 32;
    REQUIRE(var.f == Approx(0.001f));
};
