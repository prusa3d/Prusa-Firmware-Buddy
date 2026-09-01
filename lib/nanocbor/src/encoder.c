/*
 * SPDX-License-Identifier: CC0-1.0
 */

/**
 * @ingroup nanocbor
 * @{
 * @file
 * @brief   Minimalistic CBOR encoder implementation
 *
 * @author  Koen Zandberg <koen@bergzand.net>
 * @}
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nanocbor/config.h"
#include "nanocbor/nanocbor.h"

#include NANOCBOR_BYTEORDER_HEADER

/* memarray functions */
static bool _encoder_mem_fits(nanocbor_encoder_t *enc, void *ctx, size_t len)
{
    (void)ctx;
    return ((size_t)(enc->end - enc->cur) >= len);
}

static void _encoder_mem_append(nanocbor_encoder_t *enc, void *ctx, const uint8_t *data, size_t len)
{
    (void)ctx;
    memcpy(enc->cur, data, len);
    enc->cur += len;
}

void nanocbor_encoder_init(nanocbor_encoder_t *enc, uint8_t *buf, size_t len)
{
    enc->len = 0;
    enc->cur = buf;
    enc->end = buf + len;
    enc->append = _encoder_mem_append;
    enc->fits = _encoder_mem_fits;
}

void nanocbor_encoder_stream_init(nanocbor_encoder_t *enc, void *ctx,
                                  nanocbor_encoder_append append_func,
                                  nanocbor_encoder_fits fits_func)
{
    enc->len = 0;
    enc->append = append_func;
    enc->fits = fits_func;
    enc->context = ctx;
}

size_t nanocbor_encoded_len(nanocbor_encoder_t *enc)
{
    return enc->len;
}

static inline void _incr_len(nanocbor_encoder_t *enc, size_t len)
{
    enc->len += len;
}

static inline void _append(nanocbor_encoder_t *enc, const uint8_t *data, size_t len)
{
    enc->append(enc, enc->context, data, len);
}

static inline int _fits(nanocbor_encoder_t *enc, size_t len)
{
    return enc->fits(enc, enc->context, len) ? (int)len : NANOCBOR_ERR_END;
}

static int _fmt_single(nanocbor_encoder_t *enc, uint8_t single)
{
    _incr_len(enc, 1);
    int res = _fits(enc, 1);

    if (res == 1) {
        _append(enc, &single, 1);

    }
    return res;
}

int nanocbor_fmt_bool(nanocbor_encoder_t *enc, bool content)
{
    uint8_t single = NANOCBOR_MASK_FLOAT
        | (content ? NANOCBOR_SIMPLE_TRUE : NANOCBOR_SIMPLE_FALSE);

    return _fmt_single(enc, single);
}

static int _fmt_uint64(nanocbor_encoder_t *enc, uint64_t num, uint8_t type)
{
    unsigned extrabytes = 0;

    if (num < NANOCBOR_SIZE_BYTE) {
        type |= num;
    }
    else {
        if (num > UINT32_MAX) {
            /* Requires long size */
            type |= NANOCBOR_SIZE_LONG;
            extrabytes = sizeof(uint64_t);
        }
        else if (num > UINT16_MAX) {
            /* At least word size */
            type |= NANOCBOR_SIZE_WORD;
            extrabytes = sizeof(uint32_t);
        }
        else if (num > UINT8_MAX) {
            type |= NANOCBOR_SIZE_SHORT;
            extrabytes = sizeof(uint16_t);
        }
        else {
            type |= NANOCBOR_SIZE_BYTE;
            extrabytes = sizeof(uint8_t);
        }
    }
    _incr_len(enc, extrabytes + 1);
    int res = _fits(enc, extrabytes + 1);
    if (res > 0) {
        _append(enc, &type, 1);

        /* NOLINTNEXTLINE: user supplied function */
        uint64_t benum = NANOCBOR_HTOBE64_FUNC(num);


        _append(enc, (uint8_t *)&benum + sizeof(benum) - extrabytes,
               extrabytes);
    }
    return res;
}

int nanocbor_fmt_uint(nanocbor_encoder_t *enc, uint64_t num)
{
    return _fmt_uint64(enc, num, NANOCBOR_MASK_UINT);
}

int nanocbor_fmt_tag(nanocbor_encoder_t *enc, uint64_t num)
{
    return _fmt_uint64(enc, num, NANOCBOR_MASK_TAG);
}

int nanocbor_fmt_int(nanocbor_encoder_t *enc, int64_t num)
{
    if (num < 0) {
        /* Always negative at this point */
        num = -(num + 1);
        return _fmt_uint64(enc, (uint64_t)num, NANOCBOR_MASK_NINT);
    }
    return nanocbor_fmt_uint(enc, (uint64_t)num);
}

int nanocbor_fmt_simple(nanocbor_encoder_t *enc, uint8_t value)
{
    /* Exclude assigned or reserved simple values between 20 and 31 */
    if (value >= NANOCBOR_SIMPLE_FALSE && value <= NANOCBOR_SIZE_INDEFINITE) {
        return NANOCBOR_ERR_INVALID_TYPE;
    }
    return _fmt_uint64(enc, value, NANOCBOR_MASK_FLOAT);
}

int nanocbor_fmt_bstr(nanocbor_encoder_t *enc, size_t len)
{
    return _fmt_uint64(enc, (uint64_t)len, NANOCBOR_MASK_BSTR);
}

int nanocbor_fmt_tstr(nanocbor_encoder_t *enc, size_t len)
{
    return _fmt_uint64(enc, (uint64_t)len, NANOCBOR_MASK_TSTR);
}

static int _put_bytes(nanocbor_encoder_t *enc, const uint8_t *str, size_t len)
{
    _incr_len(enc, len);
    int res = _fits(enc, len);

    if (res >= 0) {
        _append(enc, str, len);
        return NANOCBOR_OK;
    }
    return res;
}

int nanocbor_put_tstr(nanocbor_encoder_t *enc, const char *str)
{
    size_t len = strlen(str);

    nanocbor_fmt_tstr(enc, len);
    return _put_bytes(enc, (const uint8_t *)str, len);
}

int nanocbor_put_tstrn(nanocbor_encoder_t *enc, const char *str, size_t len)
{
    nanocbor_fmt_tstr(enc, len);
    return _put_bytes(enc, (const uint8_t *)str, len);
}

int nanocbor_put_bstr(nanocbor_encoder_t *enc, const uint8_t *str, size_t len)
{
    nanocbor_fmt_bstr(enc, len);
    return _put_bytes(enc, str, len);
}

int nanocbor_fmt_array(nanocbor_encoder_t *enc, size_t len)
{
    return _fmt_uint64(enc, (uint64_t)len, NANOCBOR_MASK_ARR);
}

int nanocbor_fmt_map(nanocbor_encoder_t *enc, size_t len)
{
    return _fmt_uint64(enc, (uint64_t)len, NANOCBOR_MASK_MAP);
}

int nanocbor_fmt_array_indefinite(nanocbor_encoder_t *enc)
{
    return _fmt_single(enc, NANOCBOR_MASK_ARR | NANOCBOR_SIZE_INDEFINITE);
}

int nanocbor_fmt_map_indefinite(nanocbor_encoder_t *enc)
{
    return _fmt_single(enc, NANOCBOR_MASK_MAP | NANOCBOR_SIZE_INDEFINITE);
}

int nanocbor_fmt_end_indefinite(nanocbor_encoder_t *enc)
{
    /* End is marked with float major and indefinite minor number */
    return _fmt_single(enc, NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_INDEFINITE);
}

int nanocbor_fmt_null(nanocbor_encoder_t *enc)
{
    return _fmt_single(enc, NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_NULL);
}

int nanocbor_fmt_undefined(nanocbor_encoder_t *enc)
{
    return _fmt_single(enc, NANOCBOR_MASK_FLOAT | NANOCBOR_SIMPLE_UNDEF);
}

/* Double bit mask related defines */
#define DOUBLE_EXP_OFFSET (1023U)
#define DOUBLE_SIZE (64U)
#define DOUBLE_EXP_POS (52U)
#define DOUBLE_SIGN_POS (63U)
#define DOUBLE_EXP_MASK ((uint64_t)0x7FFU)
#define DOUBLE_SIGN_MASK ((uint64_t)1U << DOUBLE_SIGN_POS)
#define DOUBLE_EXP_IS_NAN (0x7FFU)
#define DOUBLE_IS_ZERO (~(DOUBLE_SIGN_MASK))
#define DOUBLE_FLOAT_LOSS (0x1FFFFFFFU)

/* float bit mask related defines */
#define FLOAT_EXP_OFFSET (127U)
#define FLOAT_SIZE (32U)
#define FLOAT_EXP_POS (23U)
#define FLOAT_EXP_MASK ((uint32_t)0xFFU)
#define FLOAT_SIGN_POS (31U)
#define FLOAT_FRAC_MASK (0x7FFFFFU)
#define FLOAT_SIGN_MASK ((uint32_t)1U << FLOAT_SIGN_POS)
#define FLOAT_EXP_IS_NAN (0xFFU)
#define FLOAT_IS_ZERO (~(FLOAT_SIGN_MASK))
/* Part where a float to halffloat leads to precision loss */
#define FLOAT_HALF_LOSS (0x1FFFU)

/* halffloat bit mask related defines */
#define HALF_EXP_OFFSET (15U)
#define HALF_SIZE (16U)
#define HALF_EXP_POS (10U)
#define HALF_EXP_MASK (0x1FU)
#define HALF_SIGN_POS (15U)
#define HALF_FRAC_MASK (0x3FFU)
#define HALF_SIGN_MASK ((uint16_t)(1U << HALF_SIGN_POS))
#define HALF_MASK_HALF (0xFFU)

/* Check special cases for single precision floats */
static bool _single_is_inf_nan(uint8_t exp)
{
    return exp == FLOAT_EXP_IS_NAN;
}

static bool _single_is_zero(uint32_t num)
{
    return (num & FLOAT_IS_ZERO) == 0;
}

static bool _single_in_range(uint8_t exp, uint32_t num)
{
    /* Check if lower 13 bits of fraction are zero, if so we might be able to
     * convert without precision loss */
    if (exp <= (HALF_EXP_OFFSET + FLOAT_EXP_OFFSET)
        && exp >= ((-HALF_EXP_OFFSET + 1) + FLOAT_EXP_OFFSET)
        && ((num & FLOAT_HALF_LOSS) == 0)) {
        return true;
    }
    return false;
}

static int _fmt_halffloat(nanocbor_encoder_t *enc, uint16_t half)
{
    _incr_len(enc, sizeof(uint16_t) + 1);
    int res = _fits(enc, sizeof(uint16_t) + 1);
    if (res > 0) {
        uint8_t tmp[3] = {
            NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_SHORT,
            (half >> HALF_SIZE / 2),
            half & HALF_MASK_HALF };
        _append(enc, tmp, sizeof(tmp));
        res = sizeof(uint16_t) + 1;
    }
    return res;
}

#if __SIZEOF_DOUBLE__ != __SIZEOF_FLOAT__
/* Check special cases for single precision floats */
static bool _double_is_inf_nan(uint16_t exp)
{
    return (exp == DOUBLE_EXP_IS_NAN);
}

static bool _double_is_zero(uint64_t num)
{
    return (num & DOUBLE_IS_ZERO) == 0;
}

static bool _double_in_range(uint16_t exp, uint64_t num)
{
    /* Check if lower 13 bits of fraction are zero, if so we might be able to
     * convert without precision loss */
    if (exp <= (DOUBLE_EXP_OFFSET + FLOAT_EXP_OFFSET)
        && exp >= (DOUBLE_EXP_OFFSET - FLOAT_EXP_OFFSET + 1)
        && ((num & DOUBLE_FLOAT_LOSS) == 0)) { /* First 29 bits must be zero */
        return true;
    }
    return false;
}
#endif

int nanocbor_fmt_float(nanocbor_encoder_t *enc, float num)
{
    /* Allow bitwise access to float */
    uint32_t *unum = (uint32_t *)&num;

    /* Retrieve exponent */
    uint8_t exp = (*unum >> FLOAT_EXP_POS) & FLOAT_EXP_MASK;
    if (_single_is_inf_nan(exp) || _single_is_zero(*unum)
        || _single_in_range(exp, *unum)) {
        /* Copy sign bit */
        uint16_t half = ((*unum >> (FLOAT_SIZE - HALF_SIZE)) & HALF_SIGN_MASK);
        /* Shift exponent */
        if (exp != FLOAT_EXP_IS_NAN && exp != 0) {
            exp = exp + (uint8_t)(HALF_EXP_OFFSET - FLOAT_EXP_OFFSET);
        }
        /* Add exponent */
        half |= ((exp & HALF_EXP_MASK) << HALF_EXP_POS)
            | ((*unum >> (FLOAT_EXP_POS - HALF_EXP_POS)) & HALF_FRAC_MASK);
        return _fmt_halffloat(enc, half);
    }
    /* normal float */
    _incr_len(enc, sizeof(float) + 1);
    int res = _fits(enc, 1 + sizeof(float));
    if (res > 0) {
        const uint8_t tmp = NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_WORD;
        _append(enc, &tmp, sizeof(tmp));
        /* NOLINTNEXTLINE: user supplied function */
        uint32_t bnum = NANOCBOR_HTOBE32_FUNC(*unum);
        _append(enc, (uint8_t*)&bnum, sizeof(bnum));
    }
    return res;
}

int nanocbor_fmt_double(nanocbor_encoder_t *enc, double num)
{
#if __SIZEOF_DOUBLE__ == __SIZEOF_FLOAT__
    return nanocbor_fmt_float(enc, num);
#else
    uint64_t *unum = (uint64_t *)&num;
    uint16_t exp = (*unum >> DOUBLE_EXP_POS) & DOUBLE_EXP_MASK;
    if (_double_is_inf_nan(exp) || _double_is_zero(*unum)
        || _double_in_range(exp, *unum)) {
        /* copy sign bit over */
        uint32_t single
            = (*unum >> (DOUBLE_SIZE - FLOAT_SIZE)) & (FLOAT_SIGN_MASK);
        /* Shift exponent */
        if (exp != DOUBLE_EXP_IS_NAN && exp != 0) {
            exp = exp + FLOAT_EXP_OFFSET - DOUBLE_EXP_OFFSET;
        }
        single |= ((exp & FLOAT_EXP_MASK) << FLOAT_EXP_POS)
            | ((*unum >> (DOUBLE_EXP_POS - FLOAT_EXP_POS)) & FLOAT_FRAC_MASK);
        float *fsingle = (float *)&single;
        return nanocbor_fmt_float(enc, *fsingle);
    }
    _incr_len(enc, sizeof(double) + 1);
    int res = _fits(enc, 1 + sizeof(double));
    if (res > 0) {
        const uint8_t tmp = NANOCBOR_MASK_FLOAT | NANOCBOR_SIZE_LONG;
        _append(enc, &tmp, 1);
        /* NOLINTNEXTLINE: user supplied function */
        uint64_t bnum = NANOCBOR_HTOBE64_FUNC(*unum);
        _append(enc, (uint8_t*)&bnum, sizeof(bnum));
    }
    return res;
#endif
}

int nanocbor_fmt_decimal_frac(nanocbor_encoder_t *enc, int32_t e, int32_t m)
{
    int res = nanocbor_fmt_tag(enc, NANOCBOR_TAG_DEC_FRAC);
    res += nanocbor_fmt_array(enc, 2);
    res += nanocbor_fmt_int(enc, e);
    res += nanocbor_fmt_int(enc, m);
    return res;
}
