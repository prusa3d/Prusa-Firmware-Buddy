/*
 * SPDX-License-Identifier: CC0-1.0
 */

#include "nanocbor/nanocbor.h"
#include "test.h"
#include <CUnit/CUnit.h>

/* NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers) */

static void test_decode_indefinite(void)
{
    /* Test vector, 3 integers in an indefinite array */
    static const uint8_t indefinite[] = { 0x9f, 0x01, 0x02, 0x03, 0xff };

    nanocbor_value_t val;
    nanocbor_value_t cont;

    uint32_t tmp = 0;

    nanocbor_decoder_init(&val, indefinite, sizeof(indefinite));

    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &cont), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_container_indefinite(&cont), true);

    /* Decode the three values */
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);

    CU_ASSERT_EQUAL(nanocbor_get_uint32(&cont, &tmp), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
}

static void test_decode_map(void)
{

    static const uint8_t map_empty[] = { 0xa0 };

    static const uint8_t map_one[] = { 0xa1, 0x01, 0x02 };

    static const uint8_t complex_map_decode[]
        = { 0xa5, 0x01, 0x02, 0x03, 0x80, 0x04, 0x9F,
            0xFF, 0x05, 0x9F, 0xff, 0x06, 0xf6 };

    nanocbor_value_t val;
    nanocbor_value_t cont;

    uint32_t tmp = 0;

    /* Init the decoder and assert the properties of the empty map */
    nanocbor_decoder_init(&val, map_empty, sizeof(map_empty));
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &cont), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    /* Init the decoder and verify the decoding of the map elements */
    nanocbor_decoder_init(&val, map_one, sizeof(map_one));
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_OK);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 1);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 2);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&val, &cont), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    /* Init the decoder and skip over the empty map */
    nanocbor_decoder_init(&val, map_empty, sizeof(map_empty));
    CU_ASSERT_EQUAL(nanocbor_skip(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    /* Init the decoder and skip over the non-empty map */
    nanocbor_decoder_init(&val, map_one, sizeof(map_one));
    CU_ASSERT_EQUAL(nanocbor_skip(&val), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);

    nanocbor_value_t array;
    /* Init decoder and start decoding */
    nanocbor_decoder_init(&val, complex_map_decode, sizeof(complex_map_decode));
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_OK);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 1);
    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 2);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 3);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&array), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), false);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 4);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&array), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), false);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 5);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&array), true);
    CU_ASSERT_EQUAL(nanocbor_leave_container(&cont, &array), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), false);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 6);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), false);
    CU_ASSERT_EQUAL(nanocbor_get_null(&cont), NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
}

static void test_tag(void)
{
    static const uint8_t arraytag[] = { 0x82, 0xd8, 0x37, 0x01, 0x02 };

    nanocbor_value_t val;
    nanocbor_value_t cont;

    uint32_t tmp = 0x12345678;

    nanocbor_decoder_init(&val, arraytag, sizeof(arraytag));

    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &cont), NANOCBOR_OK);

    CU_ASSERT_EQUAL(nanocbor_get_tag(&cont, &tmp), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tmp, 0x37);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 1);

    CU_ASSERT(nanocbor_get_uint32(&cont, &tmp) > 0);
    CU_ASSERT_EQUAL(tmp, 2);

    CU_ASSERT_EQUAL(nanocbor_at_end(&cont), true);
}

static void test_double_tag(void)
{
    static const uint8_t arraytag[] = {
        0xD9, 0xD9, 0xF7, // tag(55799)
        0xDA, 0x52, 0x49, 0x4F, 0x54, // tag(1380536148) 'RIOT'
        0x43, // bytes(3) -> 'C'
        0x42, 0x4F, 0x52 // 'BOR'
    };

    nanocbor_value_t val;
    const uint8_t *bytes = NULL;
    size_t bytes_len = 0;

    uint32_t tmp = 0x12345678;

    nanocbor_decoder_init(&val, arraytag, sizeof(arraytag));

    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tmp), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tmp, 55799);

    CU_ASSERT_EQUAL(nanocbor_get_tag(&val, &tmp), NANOCBOR_OK);
    CU_ASSERT_EQUAL(tmp, 1380536148);

    CU_ASSERT_EQUAL(nanocbor_get_bstr(&val, &bytes, &bytes_len), NANOCBOR_OK);
    CU_ASSERT_EQUAL(bytes_len, 3);
    CU_ASSERT_EQUAL(bytes[0], 'B');
    CU_ASSERT_EQUAL(bytes[1], 'O');
    CU_ASSERT_EQUAL(bytes[2], 'R');

    CU_ASSERT_EQUAL(nanocbor_at_end(&val), true);
}

static void test_decode_none(void)
{
    nanocbor_value_t val;
    nanocbor_value_t cont;
    uint64_t tmp = 0;
    nanocbor_decoder_init(&val, NULL, 0);

    CU_ASSERT_EQUAL(nanocbor_get_type(&val), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_uint32(&val, (uint32_t *)&tmp),
                    NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_int32(&val, (int32_t *)&tmp),
                    NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_enter_array(&val, &cont), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &cont), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_null(&val), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_get_bool(&val, (bool *)&tmp), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_skip(&val), NANOCBOR_ERR_END);
    CU_ASSERT_EQUAL(nanocbor_skip_simple(&val), NANOCBOR_ERR_END);
}

static void test_decode_basic(void)
{
    nanocbor_value_t decoder;
    uint8_t byteval = 5; /* unsigned integer, value 5 */
    uint32_t value = 0;

    nanocbor_decoder_init(&decoder, &byteval, sizeof(byteval));
    CU_ASSERT_EQUAL(nanocbor_get_type(&decoder), NANOCBOR_TYPE_UINT);
    printf("\"val: %u\"\n", value);
    CU_ASSERT_EQUAL(nanocbor_get_uint32(&decoder, &value), 1);
    printf("\"val: %u\"\n", value);
    CU_ASSERT_EQUAL(5, value);

    int32_t intval = 0;
    nanocbor_decoder_init(&decoder, &byteval, sizeof(byteval));
    CU_ASSERT_EQUAL(nanocbor_get_int32(&decoder, &intval), 1);
    CU_ASSERT_EQUAL(5, intval);

    const uint8_t decimal_frac[] = { 0xC4, 0x82, 0x21, 0x19, 0x6a, 0xb3 };
    int32_t mantissa = 0;
    int32_t exponent = 0;
    nanocbor_decoder_init(&decoder, decimal_frac, sizeof(decimal_frac));
    CU_ASSERT_EQUAL(nanocbor_get_decimal_frac(&decoder, &exponent, &mantissa),
                    0);
    CU_ASSERT_EQUAL(exponent, -2);
    CU_ASSERT_EQUAL(mantissa, 27315);
}

static void _decode_skip(const uint8_t *test_case, size_t test_case_len, bool simple)
{
    nanocbor_value_t decoder;

    nanocbor_decoder_init(&decoder, test_case, test_case_len);
    CU_ASSERT_EQUAL(nanocbor_at_end(&decoder), false);
    int res = nanocbor_skip(&decoder);
    CU_ASSERT_EQUAL(res, NANOCBOR_OK);
    CU_ASSERT_EQUAL(nanocbor_at_end(&decoder), true);

    if (simple) {
        nanocbor_decoder_init(&decoder, test_case, test_case_len);
        CU_ASSERT_EQUAL(nanocbor_at_end(&decoder), false);
        int res = nanocbor_skip_simple(&decoder);
        CU_ASSERT_EQUAL(res, NANOCBOR_OK);
        CU_ASSERT_EQUAL(nanocbor_at_end(&decoder), true);
    }
}

static void test_decode_skip(void)
{
    static const uint8_t test_uint[] = { 0x00 };
    _decode_skip(test_uint, sizeof(test_uint), true);
    static const uint8_t test_nint[] = { 0x20 };
    _decode_skip(test_nint, sizeof(test_nint), true);
    static const uint8_t test_bstr_empty[] = { 0x40 };
    _decode_skip(test_bstr_empty, sizeof(test_bstr_empty), true);
    static const uint8_t test_bstr[] = { 0x42, 0xAA, 0xBB };
    _decode_skip(test_bstr, sizeof(test_bstr), true);
    static const uint8_t test_tstr[] = { 0x65, 0x68, 0x65, 0x6C, 0x6C, 0x6F };
    _decode_skip(test_tstr, sizeof(test_tstr), true);

    static const uint8_t test_float[] = { 0xF9, 0x42, 0x00 };
    _decode_skip(test_float, sizeof(test_float), true);
    static const uint8_t test_simple[] = { 0xF4 };
    _decode_skip(test_simple, sizeof(test_simple), true);

    static const uint8_t test_tag[] = { 0xD8, 0x29, 0x82, 0xF5, 0xF4 };
    _decode_skip(test_tag, sizeof(test_tag), false);
    static const uint8_t test_tag_within_array[] = { 0x81, 0xD8, 0x29, 0x80 };
    _decode_skip(test_tag_within_array, sizeof(test_tag_within_array), false);
    static const uint8_t test_array_within_tag[] = { 0x81, 0xC4, 0x82, 0x21, 0x19, 0x6A, 0xB3 };
    _decode_skip(test_array_within_tag, sizeof(test_array_within_tag), false);

    static const uint8_t test_array_empty[] = { 0x80 };
    _decode_skip(test_array_empty, sizeof(test_array_empty), false);
    static const uint8_t test_array[] = { 0x81, 0xF6 };
    _decode_skip(test_array, sizeof(test_array), false);
    static const uint8_t test_array_nested[] = { 0x83, 0xF4, 0x81, 0xF5, 0xF6 };
    _decode_skip(test_array_nested, sizeof(test_array_nested), false);

    static const uint8_t test_map_empty[] = { 0xA0 };
    _decode_skip(test_map_empty, sizeof(test_map_empty), false);
    static const uint8_t test_map[] = { 0xA1, 0x61, 0x61, 0xF6 };
    _decode_skip(test_map, sizeof(test_map), false);
    static const uint8_t test_map_nested[] = { 0xA3, 0x00, 0xF4, 0x01, 0xA1, 0x00, 0xF5, 0x02, 0xF6 };
    _decode_skip(test_map_nested, sizeof(test_map_nested), false);

    static const uint8_t test_array_indefinite_empty[] = { 0x9F, 0xFF };
    _decode_skip(test_array_indefinite_empty, sizeof(test_array_indefinite_empty), false);
    static const uint8_t test_array_indefinite_flat[] = { 0x9F, 0xF5, 0xFF };
    _decode_skip(test_array_indefinite_flat, sizeof(test_array_indefinite_flat), false);
    static const uint8_t test_array_indefinite_nested[] = { 0x9F, 0xF4, 0x81, 0xF5, 0xF6, 0xFF };
    _decode_skip(test_array_indefinite_nested, sizeof(test_array_indefinite_nested), false);
    static const uint8_t test_array_indefinite_nested2[] = { 0x9F, 0xF4, 0x9F, 0xF5, 0xFF, 0xF6, 0xFF };
    _decode_skip(test_array_indefinite_nested2, sizeof(test_array_indefinite_nested2), false);

    static const uint8_t test_map_indefinite_empty[] = { 0xBF, 0xFF };
    _decode_skip(test_map_indefinite_empty, sizeof(test_map_indefinite_empty), false);
    static const uint8_t test_map_indefinite_flat[] = { 0xBF, 0x01, 0xF5, 0xFF };
    _decode_skip(test_map_indefinite_flat, sizeof(test_map_indefinite_flat), false);
    static const uint8_t test_map_indefinite_nested[] = { 0xBF, 0x00, 0xF4, 0x01, 0x81, 0xF5, 0x02, 0xF6, 0xFF };
    _decode_skip(test_map_indefinite_nested, sizeof(test_map_indefinite_nested), false);
    static const uint8_t test_map_indefinite_nested2[] = { 0xBF, 0x00, 0xF4, 0x01, 0xBF, 0x00, 0xF5, 0xFF, 0x02, 0xF6, 0xFF };
    _decode_skip(test_map_indefinite_nested2, sizeof(test_map_indefinite_nested2), false);
}

static void test_issue_56(void)
{
    static const uint8_t input[] = {
        0xa2,
        0x64, 'k', 'e', 'y', '1',
        0x66, 'v', 'a', 'l', 'u', 'e', '1',
        0x64, 'k', 'e', 'y', '2',
        0x66, 'v', 'a', 'l', 'u', 'e', '2',
    };

    nanocbor_value_t val;
    nanocbor_value_t map;
    nanocbor_value_t value;

    nanocbor_decoder_init(&val, input, sizeof(input));

    CU_ASSERT_EQUAL(nanocbor_enter_map(&val, &map), NANOCBOR_OK);
    CU_ASSERT_NOT_EQUAL(
        nanocbor_get_key_tstr(&map, "key3", &value),
        NANOCBOR_OK
    );
}

static void test_issue_101(void)
{
    static const uint8_t input[] =  {
        0x5b,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,
        0xaa,
    };

    nanocbor_value_t val;

    nanocbor_decoder_init(&val, input, sizeof(input));

    const uint8_t *ptr = NULL;
    size_t len = 0;
    int res = nanocbor_get_bstr(&val, &ptr, &len);
    CU_ASSERT_EQUAL(res, NANOCBOR_ERR_END);
}

const test_t tests_decoder[] = {
    {
        .f = test_decode_none,
        .n = "get type on empty buffer",
    },
    {
        .f = test_decode_basic,
        .n = "Simple CBOR integer tests",
    },
    {
        .f = test_decode_indefinite,
        .n = "CBOR indefinite array decode tests",
    },
    {
        .f = test_decode_map,
        .n = "CBOR map decode tests",
    },
    {
        .f = test_tag,
        .n = "CBOR tag decode test",
    },
    {
        .f = test_double_tag,
        .n = "CBOR double tag decode test",
    },
    {
        .f = test_decode_skip,
        .n = "CBOR skip test",
    },
    {
        .f = test_issue_56,
        .n = "Regression test for GitHub issue 56",
    },
    {
        .f = test_issue_101,
        .n = "Regression test for GitHub issue #101",
    },
    {
        .f = NULL,
        .n = NULL,
    },
};

/* NOLINTEND(cppcoreguidelines-avoid-magic-numbers) */
