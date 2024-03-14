#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

#include "common/variant8.h"
#include <cstdint>
#include <float.h>

TEST_CASE("init & done", "[variant8]") {

    SECTION("empty") {
        variant8_t v = variant8_empty();
        variant8_t *pv = &v;
        CHECK(variant8_get_type(v) == VARIANT8_EMPTY);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_ui32(v) == 0);
        CHECK(variant8_get_i32(v) == 0);
        CHECK(variant8_get_i8(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(0.00));
        REQUIRE(variant8_data_ptr(&v) == nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("construct all value types") {
        variant8_t v = variant8_empty();
        variant8_t *pv = &v;
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
        v = variant8_flt(0.00);
        REQUIRE(variant8_get_type(v) == VARIANT8_FLT);
        v = variant8_ui32(0);
        REQUIRE(variant8_get_type(v) == VARIANT8_UI32);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("flt") {
        variant8_t v = variant8_flt(0.001f);
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(0.001f));
    };

    SECTION("init.0") {
        variant8_t v = variant8_init(VARIANT8_FLT, 1, 0);
        variant8_t *pv = &v;
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
        CHECK(variant8_get_flt(v) == Approx(0.00f));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("init.1") {
        float fa[5] = { 0.00f, 1.00f, FLT_MIN, FLT_MAX, -0.00f };
        variant8_t v = variant8_init(VARIANT8_PFLT, 5, fa);
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        float *pflt = (float *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PFLT);
        CHECK(fa[0] == Approx(*pflt));
        CHECK(fa[1] == Approx(*(pflt + 1)));
        CHECK(fa[2] == Approx(*(pflt + 2)));
        CHECK(fa[3] == Approx(*(pflt + 3)));
        CHECK(fa[4] == Approx(*(pflt + 4)));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("init.2") {
        variant8_t v = variant8_init(VARIANT8_PCHAR, 10, "1234567890");
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        const char *pchar = (const char *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PCHAR);
        CHECK(pchar[0] == '1');
        CHECK(pchar[1] == '2');
        CHECK(pchar[2] == '3');
        CHECK(pchar[8] == '9');
        CHECK(pchar[9] == '0');
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("init.2") {
        const char data[12] = "1234567890\0";
        variant8_t v = variant8_init(VARIANT8_PCHAR, 12, data);
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        const char *pchar = (const char *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PCHAR);
        CHECK(pchar[0] == '1');
        CHECK(pchar[1] == '2');
        CHECK(pchar[2] == '3');
        CHECK(pchar[8] == '9');
        CHECK(pchar[9] == '0');
        CHECK(pchar[10] == 0);
        CHECK(pchar[11] == 0);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };
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
    };

    SECTION("variant8_flt") {
        variant8_t v = variant8_flt(0.001f);
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(0.001f));
        v = variant8_i8(127);
        CHECK(variant8_get_type(v) == VARIANT8_I8);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_i8(v) == 127);
    };

    SECTION("set type") {
        variant8_t v = variant8_flt(0.001f);
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(0.001f));
        variant8_set_type(&v, VARIANT8_I8);
        CHECK(variant8_get_type(v) == VARIANT8_I8);
    };
}

TEST_CASE("size", "[variant8]") {

    SECTION("static check") {
        CHECK(variant8_type_size(VARIANT8_EMPTY) == 0);
        CHECK(variant8_type_size(VARIANT8_FLT) == 4);
        CHECK(variant8_type_size(VARIANT8_I8) == 1);
        CHECK(variant8_type_size(VARIANT8_UI16) == 2);
        CHECK(variant8_type_size(VARIANT8_I16) == 2);
        CHECK(variant8_type_size(VARIANT8_UI8) == 1);
        CHECK(variant8_type_size(VARIANT8_UI32) == 4);
        CHECK(variant8_type_size(VARIANT8_I32) == 4);
        CHECK(variant8_type_size(VARIANT8_CHAR) == 1);
#if INTPTR_MAX == INT32_MAX
        CHECK(variant8_type_size(VARIANT8_USER) == 7);
#elif INTPTR_MAX == INT64_MAX
        CHECK(variant8_type_size(VARIANT8_USER) == 15);
#endif
        CHECK(variant8_type_size(VARIANT8_ERROR) == 0);
    };

    SECTION("data types") {
        uint32_t data[5] = { 1, 2, 3, 4, 5 };
        variant8_t v = variant8_i32(0);
        variant8_t *pv = &v;
        CHECK(variant8_data_size(&v) == 4);
        v = variant8_init(VARIANT8_PUI32, 5, &data);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        CHECK(variant8_data_size(&v) == 20);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };
}

TEST_CASE("data pointer", "[variant8]") {
    SECTION("float type") {
        variant8_t v = variant8_flt(10.001f);
        variant8_t *pv = &v;
        void *data_ptr = nullptr;
        CHECK(variant8_get_type(v) == VARIANT8_FLT);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_flt(v) == Approx(10.001f));
        data_ptr = variant8_data_ptr(&v);
        REQUIRE(data_ptr != nullptr);
        CHECK(((*(float *)data_ptr)) == Approx(10.001f));
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("uint32 type") {
        variant8_t v = variant8_ui32(10);
        variant8_t *pv = &v;
        void *data_ptr = nullptr;
        CHECK(variant8_get_type(v) == VARIANT8_UI32);
        CHECK(variant8_get_usr16(v) == 0);
        CHECK(variant8_get_ui32(v) == 10);
        data_ptr = variant8_data_ptr(&v);
        REQUIRE(data_ptr != nullptr);
        CHECK(((*(uint32_t *)data_ptr)) == 10);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("pchar type initialized") {
        char data[12] = "1234567890\0";
        variant8_t v = variant8_pchar(data, 12, 1);
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        const char *pchar = (const char *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PCHAR);
        CHECK(pchar[0] == '1');
        CHECK(pchar[1] == '2');
        CHECK(pchar[2] == '3');
        CHECK(pchar[8] == '9');
        CHECK(pchar[9] == '0');
        CHECK(pchar[10] == 0);
        CHECK(pchar[11] == 0);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("pchar type not initialized") {
        char data[12] = "1234567890\0";
        variant8_t v = variant8_pchar(data, 12, 0);
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        const char *pchar = (const char *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PCHAR);
        CHECK(pchar[0] == '1');
        CHECK(pchar[1] == '2');
        CHECK(pchar[2] == '3');
        CHECK(pchar[8] == '9');
        CHECK(pchar[9] == '0');
        CHECK(pchar[10] == 0);
        CHECK(pchar[11] == 0);
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };
}

TEST_CASE("memory layout", "[variant8][.]") {
#pragma pack(push)
#pragma pack(1)
    struct origin_variant {
        uint8_t a;
        uint8_t b;
        uint16_t c;
        uint32_t d;
    };
#pragma pack(pop)
    struct new_variant {
        uint32_t d;
        uint16_t c;
        uint8_t a;
        uint8_t b;
    };

    REQUIRE(sizeof(origin_variant) == 8);
    REQUIRE(sizeof(new_variant) == 8);

    // const size_t bench_count = 100000ul;

    // BENCHMARK("access origin a") {
    //     uint8_t l = 0;
    //     for (size_t i = 0; i < bench_count; ++i) {
    //         origin_variant v;
    //         v.a = i % 0xFF;
    //         l = v.a;
    //     }
    //     return l;
    // };

    // BENCHMARK("access new a") {
    //     uint8_t l = 0;
    //     for (size_t i = 0; i < bench_count; ++i) {
    //         new_variant v;
    //         v.a = i % 0xFF;
    //         l = v.a;
    //     }
    //     return l;
    // };

    // BENCHMARK("access origin d") {
    //     uint8_t l = 0;
    //     for (size_t i = 0; i < bench_count; ++i) {
    //         origin_variant v;
    //         v.d = i % 0xFF;
    //         l = v.d;
    //     }
    //     return l;
    // };

    // BENCHMARK("access new d") {
    //     uint8_t l = 0;
    //     for (size_t i = 0; i < bench_count; ++i) {
    //         new_variant v;
    //         v.d = i % 0xFF;
    //         l = v.d;
    //     }
    //     return l;
    // };
}

TEST_CASE("failing tests", "[variant8][.]") {

    SECTION("variant8 init single pointer") {
        float f = 0.1f;
        variant8_t v = variant8_init(VARIANT8_PFLT, 1, &f);
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        float *pflt = (float *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PFLT);
        CHECK(f == Approx(*pflt));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("variant8 init single value") {
        float f = 1.00f;
        variant8_t v = variant8_init(VARIANT8_PFLT, 5, &f);
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        float *pflt = (float *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_PFLT);
        CHECK(f == Approx(*pflt));
        CHECK(f == Approx(*(pflt + 1)));
        CHECK(f == Approx(*(pflt + 2)));
        CHECK(f == Approx(*(pflt + 3)));
        CHECK(f == Approx(*(pflt + 4)));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("init not initialized array") {
        variant8_t v = variant8_init(VARIANT8_PUI32, 5, 0);
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        uint32_t *puint32 = (uint32_t *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_UI32);
        CHECK(0 == *puint32);
        CHECK(0 == *(puint32 + 1));
        CHECK(0 == *(puint32 + 2));
        CHECK(0 == *(puint32 + 3));
        CHECK(0 == *(puint32 + 4));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };

    SECTION("init different type of array") {
        uint8_t data[5] = { 0, 1, 2, 3, 4 };
        variant8_t v = variant8_init(VARIANT8_PUI32, 5, data);
        variant8_t *pv = &v;
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        uint32_t *puint32 = (uint32_t *)variant8_data_ptr(&v);
        CHECK(variant8_get_type(v) == VARIANT8_UI32);
        CHECK(0 == *puint32);
        CHECK(0 == *(puint32 + 1));
        CHECK(0 == *(puint32 + 2));
        CHECK(0 == *(puint32 + 3));
        CHECK(0 == *(puint32 + 4));
        REQUIRE(variant8_data_ptr(&v) != nullptr);
        variant8_done(&pv);
        REQUIRE(variant8_data_ptr(&v) == nullptr);
    };
}
