#include "catch2/catch.hpp"

#include "common/variant8.h"
#include <float.h>

TEST_CASE("initialization", "[variant8]") {

    SECTION("empty") {
        variant8_t v = variant8_empty();
        CHECK(variant8_get_type(v) == VARIANT8_EMPTY);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_ui32(v) == 0);
        CHECK(variant8_get_i32(v) == 0);
        CHECK(variant8_get_i8(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(0.00));
    }

    SECTION("construct all value types") {
        variant8_t v = variant8_empty();
        REQUIRE(variant8_get_type(v) == VARIANT8_EMPTY);
        v = variant8_i8(0);
        REQUIRE(variant8_get_type(v) == VARIANT8_I8);
        v = variant8_ui8(0);
        REQUIRE(variant8_get_type(v) == VARIANT8_UI8);
        v = variant8_i16(0);
        REQUIRE(variant8_get_type(v) == VARIANT8_I16);
        v = variant8_ui16(0);
        REQUIRE(variant8_get_type(v) == VARIANT8_UI16);
        v = variant8_i32(0);
        REQUIRE(variant8_get_type(v) == VARIANT8_I32);
        v = variant8_ui32(0);
        REQUIRE(variant8_get_type(v) == VARIANT8_UI32);
        v = variant8_flt(0.0);
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
    }

    SECTION("flt") {
        variant8_t v = variant8_flt(0.001f);
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(0.001f));
    }

    SECTION("init.0") {
        variant8_t v = variant8_init(VARIANT8_FLT, 1, 0);
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
        CHECK(variant8_get_flt(v) == Approx(0.00f));
    }

    SECTION("init.1") {
        float fa[5] = { 0.00f, 1.00f, FLT_MIN, FLT_MAX, -0.00f };
        variant8_t v = variant8_init(VARIANT8_PFLT, 5, fa);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        float *pflt = (float *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PFLT);
        CHECK(fa[0] == Approx(*pflt));
        CHECK(fa[1] == Approx(*(pflt + 1)));
        CHECK(fa[2] == Approx(*(pflt + 2)));
        CHECK(fa[3] == Approx(*(pflt + 3)));
        CHECK(fa[4] == Approx(*(pflt + 4)));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&v);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    }
}

TEST_CASE("update", "[variant8]") {

    SECTION("variant8_empty") {
        variant8_t v = variant8_empty();
        CHECK(variant8_get_type(v) == VARIANT8_EMPTY);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_ui32(v) == 0);
        CHECK(variant8_get_i32(v) == 0);
        CHECK(variant8_get_i8(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(0.00));
        v = variant8_i8(127);
        CHECK(variant8_get_type(v) == VARIANT8_I8);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_i8(v) == 127);
    }

    SECTION("variant8_flt") {
        variant8_t v = variant8_flt(0.001f);
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(0.001f));
        v = variant8_i8(127);
        CHECK(variant8_get_type(v) == VARIANT8_I8);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_i8(v) == 127);
    }
}

TEST_CASE("failing tests", "[variant8][.]") {

    SECTION("variant8 init single pointer") {
        float f = 0.1f;
        variant8_t v = variant8_init(VARIANT8_PFLT, 1, &f);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        float *pflt = (float *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PFLT);
        CHECK(f == Approx(*pflt));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&v);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    }

    SECTION("variant8 init single value") {
        float f = 1.00f;
        variant8_t v = variant8_init(VARIANT8_PFLT, 5, &f);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        float *pflt = (float *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PFLT);
        CHECK(f == Approx(*pflt));
        CHECK(f == Approx(*(pflt + 1)));
        CHECK(f == Approx(*(pflt + 2)));
        CHECK(f == Approx(*(pflt + 3)));
        CHECK(f == Approx(*(pflt + 4)));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&v);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    }

    SECTION("init not initialize array") {
        variant8_t v = variant8_init(VARIANT8_PUI32, 5, 0);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        uint32_t *puint32 = (uint32_t *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_UI32);
        CHECK(0 == *puint32);
        CHECK(0 == *(puint32 + 1));
        CHECK(0 == *(puint32 + 2));
        CHECK(0 == *(puint32 + 3));
        CHECK(0 == *(puint32 + 4));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&v);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    }
}
