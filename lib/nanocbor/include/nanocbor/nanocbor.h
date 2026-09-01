/*
 * SPDX-License-Identifier: CC0-1.0
 */

/**
 * @defgroup    NanoCBOR minimalistic CBOR library
 * @brief       Provides a minimal CBOR library
 *
 * NanoCBOR is a minimal CBOR encoder. For protocols such as CoAP, OSCORE,
 * SenML and CORECONF a well defined and thus predictable CBOR structure is
 * required. NanoCBOR tries to fill this requirement by providing a very
 * minimal CBOR encoder. Supported is:
 *  - All major types
 *  - Arrays including indefinite length arrays
 *  - Maps including indefinite length maps
 *  - Safe for decoding untrusted input
 *
 * Not included:
 *  - Date and time
 *  - Big numbers (numbers encoded as byte strings)
 *
 * @{
 *
 * @file
 * @see         [rfc 7049](https://tools.ietf.org/html/rfc7049)
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef NANOCBOR_NANOCBOR_H
#define NANOCBOR_NANOCBOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NANOCBOR_TYPE_OFFSET (5U) /**< Bit shift for CBOR major types */
#define NANOCBOR_TYPE_MASK 0xE0U /**< Mask for CBOR major types */
#define NANOCBOR_VALUE_MASK 0x1FU /**< Mask for CBOR values */

/**
 * @name CBOR type numbers
 * @{
 */
#define NANOCBOR_TYPE_UINT (0x00U) /**< positive integer type */
#define NANOCBOR_TYPE_NINT (0x01U) /**< negative integer type */
#define NANOCBOR_TYPE_BSTR (0x02U) /**< byte string type */
#define NANOCBOR_TYPE_TSTR (0x03U) /**< text string type */
#define NANOCBOR_TYPE_ARR (0x04U) /**< array type */
#define NANOCBOR_TYPE_MAP (0x05U) /**< map type */
#define NANOCBOR_TYPE_TAG (0x06U) /**< tag type */
#define NANOCBOR_TYPE_FLOAT (0x07U) /**< float type */
/** @} */

/**
 * @name CBOR major types including the bit shift
 * @{
 */
#define NANOCBOR_MASK_UINT (NANOCBOR_TYPE_UINT << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_NINT (NANOCBOR_TYPE_NINT << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_BSTR (NANOCBOR_TYPE_BSTR << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_TSTR (NANOCBOR_TYPE_TSTR << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_ARR (NANOCBOR_TYPE_ARR << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_MAP (NANOCBOR_TYPE_MAP << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_TAG (NANOCBOR_TYPE_TAG << NANOCBOR_TYPE_OFFSET)
#define NANOCBOR_MASK_FLOAT (NANOCBOR_TYPE_FLOAT << NANOCBOR_TYPE_OFFSET)
/** @} */

/**
 * @name CBOR simple data types
 * @{
 */
#define NANOCBOR_SIMPLE_FALSE 20U /**< False     */
#define NANOCBOR_SIMPLE_TRUE 21U /**< True      */
#define NANOCBOR_SIMPLE_NULL 22U /**< NULL      */
#define NANOCBOR_SIMPLE_UNDEF 23U /**< Undefined */
/** @} */

/**
 * @name CBOR data sizes
 * @{
 */
#define NANOCBOR_SIZE_BYTE 24U /**< Value contained in a byte */
#define NANOCBOR_SIZE_SHORT 25U /**< Value contained in a short */
#define NANOCBOR_SIZE_WORD 26U /**< Value contained in a word */
#define NANOCBOR_SIZE_LONG 27U /**< Value contained in a long */
#define NANOCBOR_SIZE_INDEFINITE 31U /**< Indefinite sized container */
/** @} */

/**
 * @name CBOR Tag values
 * @{
 */
#define NANOCBOR_TAG_DATE_TIME (0x0) /**< Standard date/time string */
#define NANOCBOR_TAG_EPOCH (0x1) /**< Epoch-based date/time */
#define NANOCBOR_TAG_BIGNUMS_P (0x2) /**< Positive bignum */
#define NANOCBOR_TAG_BIGNUMS_N (0x3) /**< Negative bignum */
#define NANOCBOR_TAG_DEC_FRAC (0x4) /**< Decimal Fraction */
#define NANOCBOR_TAG_BIGFLOATS (0x5) /**< Bigfloat */
/** @} */

/**
 * @brief NanoCBOR decoder errors
 */
typedef enum {
    /**
     * @brief No error
     */
    NANOCBOR_OK = 0,

    /**
     * @brief Overflow in the getter. This can happen due to retrieving a
     *        number size larger than the function provides
     */
    NANOCBOR_ERR_OVERFLOW = -1,

    /**
     * Decoder get function attempts to retrieve the wrong type
     */
    NANOCBOR_ERR_INVALID_TYPE = -2,

    /**
     * @brief decoder is beyond the end of the buffer
     */
    NANOCBOR_ERR_END = -3,

    /**
     * @brief Decoder hits the recursion limit
     */
    NANOCBOR_ERR_RECURSION = -4,

    /**
     * @brief Decoder could not find the requested entry
     */
    NANOCBOR_NOT_FOUND = -5,

    /**
     * @brief Decoder detected wrong usage, e.g., leaving a container
     *        before reaching its end. This is a programming mistake
     *        and can be asserted upon by the caller.
     */
    NANOCOBR_PANIC_INVALID_OPERATION = -6,
} nanocbor_error_t;

/**
 * @brief decoder context
 */
typedef struct nanocbor_value {
    const uint8_t *cur; /**< Current position in the buffer             */
    const uint8_t *end; /**< End of the buffer                          */
    uint64_t remaining; /**< Number of items remaining in the container */
    uint8_t flags; /**< Flags for decoding hints                   */
} nanocbor_value_t;

/**
 * @brief Encoder context forward declaration
 */
typedef struct nanocbor_encoder nanocbor_encoder_t;

/**
 * @name encoder streaming functions
 * @{
 */

/**
 * @brief Fits function for streaming encoder data
 *
 * This function is called to check whether the data that will be supplied can
 * be consumed by the interface.
 *
 * The append function will only be called if this function returns true. The
 * amount of bytes checked by this call can be used by multiple successive
 * @ref nanocbor_encoder_append calls
 *
 * @param   enc     The encoder context struct
 * @param   ctx     The context ptr supplied in the
 *                  @ref nanocbor_encoder_stream_init call
 * @param   len     Length of the data in bytes that will be supplied
 */
typedef bool (*nanocbor_encoder_fits)(nanocbor_encoder_t *enc, void *ctx, size_t len);

/**
 * @brief Append function for streaming encoder data
 *
 * This is a user supplied function for the polymorphic encoder interface. It
 * will only be called if a previous call to @ref nanocbor_encoder_fits returned
 * true.
 *
 * @param   enc     The encoder context struct
 * @param   ctx     The context ptr supplied in the
 *                  @ref nanocbor_encoder_stream_init call
 * @param   data    Data emitted by the encoder
 * @param   len     Length of the data in bytes
 */
typedef void (*nanocbor_encoder_append)(nanocbor_encoder_t *enc, void *ctx, const uint8_t *data, size_t len);

/** @} */

/**
 * @brief encoder context
 */
struct nanocbor_encoder {
    size_t len; /**< Length in bytes of supplied cbor data. Incremented
                 *  separate from the buffer check  */
    nanocbor_encoder_append append; /**< Function used to append encoded data */
    nanocbor_encoder_fits fits; /** Function used to check encoded length */
    union {
        uint8_t *cur; /**< Current position in the buffer, unioned to keep
                       *   compatibility */
        void *context; /**< Context ptr supplied to the custom functions */
    };
    uint8_t *end; /**< end of the buffer                      */
};

/**
 * @name decoder flags
 * @{
 */

/**
 * @brief decoder value is inside a container
 */
#define NANOCBOR_DECODER_FLAG_CONTAINER (0x01U)

/**
 * @brief decoder value is inside an indefinite length container
 */
#define NANOCBOR_DECODER_FLAG_INDEFINITE (0x02U)
/** @} */

/**
 * @name NanoCBOR parser functions
 * @{
 */

/**
 * @brief Initialize a decoder context decoding the CBOR structure from @p buf
 *        with @p len bytes
 *
 * The decoder will attempt to decode CBOR types until the buffer is exhausted
 *
 * @param[in]   value   decoder value context
 * @param[in]   buf     Buffer to decode from
 * @param[in]   len     Length in bytes of the buffer
 */
void nanocbor_decoder_init(nanocbor_value_t *value, const uint8_t *buf,
                           size_t len);

/**
 * @brief Retrieve the type of the CBOR value at the current position
 *
 * @param[in]   value   decoder value context
 *
 * @return              major type
 * @return              NANOCBOR_ERR_END if the buffer is exhausted
 */
int nanocbor_get_type(const nanocbor_value_t *value);

/**
 * @brief Check if the current buffer or container is exhausted
 *
 * @param[in]   it      decoder value context
 *
 * @return              true if it is exhausted
 * @return              false if there are more items
 */
bool nanocbor_at_end(const nanocbor_value_t *it);

/**
 * @brief Retrieve a positive integer as uint8_t from the stream
 *
 * If the value at `cvalue` is greater than 8 bit (> 255), error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   returned positive integer
 *
 * @return              number of bytes read
 * @return              negative on error
 */
int nanocbor_get_uint8(nanocbor_value_t *cvalue, uint8_t *value);

/**
 * @brief Retrieve a positive integer as uint16_t from the stream
 *
 * If the value at `cvalue` is greater than 16 bit (> 65535), error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   returned positive integer
 *
 * @return              number of bytes read
 * @return              negative on error
 */
int nanocbor_get_uint16(nanocbor_value_t *cvalue, uint16_t *value);

/**
 * @brief Retrieve a positive integer as uint32_t from the stream
 *
 * If the value at `cvalue` is greater than 32 bit, error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   returned positive integer
 *
 * @return              number of bytes read
 * @return              negative on error
 */
int nanocbor_get_uint32(nanocbor_value_t *cvalue, uint32_t *value);

/**
 * @brief Retrieve a positive integer as uint64_t from the stream
 *
 * If the value at `cvalue` is greater than 64 bit, error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   returned positive integer
 *
 * @return              number of bytes read
 * @return              negative on error
 */
int nanocbor_get_uint64(nanocbor_value_t *cvalue, uint64_t *value);

/**
 * @brief Retrieve a signed integer as int8_t from the stream
 *
 * If the value at `cvalue` is greater than 8 bit (< -128 or > 127),
 * error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   returned signed integer
 *
 * @return              number of bytes read
 * @return              negative on error
 */
int nanocbor_get_int8(nanocbor_value_t *cvalue, int8_t *value);

/**
 * @brief Retrieve a signed integer as int16_t from the stream
 *
 * If the value at `cvalue` is greater than 16 bit (< -32768 or > 32767),
 * error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   returned signed integer
 *
 * @return              number of bytes read
 * @return              negative on error
 */
int nanocbor_get_int16(nanocbor_value_t *cvalue, int16_t *value);

/**
 * @brief Retrieve a signed integer as int32_t from the stream
 *
 * If the value at `cvalue` is greater than 32 bit, error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   returned signed integer
 *
 * @return              number of bytes read
 * @return              negative on error
 */
int nanocbor_get_int32(nanocbor_value_t *cvalue, int32_t *value);

/**
 * @brief Retrieve a signed integer as int64_t from the stream
 *
 * If the value at `cvalue` is greater than 64 bit, error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   returned signed integer
 *
 * @return              number of bytes read
 * @return              negative on error
 */
int nanocbor_get_int64(nanocbor_value_t *cvalue, int64_t *value);

/**
 * @brief Retrieve a decimal fraction from the stream as a int32_t mantisa and
 *        int32_t exponent
 *
 * If the value at `cvalue` is greater than 32 bit, error is returned.
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  m       returned mantisa
 * @param[out]  e       returned exponent
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_get_decimal_frac(nanocbor_value_t *cvalue, int32_t *e, int32_t *m);

/**
 * @brief Retrieve a byte string from the stream
 *
 * The resulting @p buf and @p len are undefined if the result is an error
 * condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  buf     pointer to the byte string
 * @param[out]  len     length of the byte string
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_get_bstr(nanocbor_value_t *cvalue, const uint8_t **buf,
                      size_t *len);

/**
 * @brief Retrieve a text string from the stream
 *
 * The resulting @p buf and @p len are undefined if the result is an error
 * condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  buf     pointer to the text string
 * @param[out]  len     length of the text string
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_get_tstr(nanocbor_value_t *cvalue, const uint8_t **buf,
                      size_t *len);

/**
 * @brief Search for a tstr key in a map.
 *
 * The resulting @p value is undefined if @p key was not found.
 *
 * @pre @p start is inside a map
 *
 * @param[in]   start   pointer to the map to search
 * @param[in]   key     pointer to the text string key
 * @param[out]  value   pointer to the tstr value containing @p key if found
 *
 * @return              NANOCBOR_OK if @p key was found
 * @return              negative on error / not found
 */
int nanocbor_get_key_tstr(nanocbor_value_t *start, const char *key,
                          nanocbor_value_t *value);

/**
 * @brief Enter a array type
 *
 * @param[in]   it      CBOR value to decode from
 * @param[out]  array   CBOR value to decode the array members with
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_enter_array(const nanocbor_value_t *it, nanocbor_value_t *array);

/**
 * @brief Enter a map type
 *
 * @param[in]   it      CBOR value to decode from
 * @param[out]  map     CBOR value to decode the map members with
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_enter_map(const nanocbor_value_t *it, nanocbor_value_t *map);

/**
 * @brief leave the container
 *
 * This must be called with the same @ref nanocbor_value_t struct that was used
 * to enter the container. Furthermore, the @p container must be at the end of
 * the container.
 *
 * @param[in]   it          parent CBOR structure
 * @param[in]   container   exhausted CBOR container
 *
 * @return                  NANOCBOR_OK on success
 * @return                  NANOCBOR_PANIC_INVALID_OPERATION on detection of wrong usage
 */
int nanocbor_leave_container(nanocbor_value_t *it,
                              nanocbor_value_t *container);

/**
 * @brief Retrieve a tag as positive uint32_t from the stream
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  tag     returned tag as positive integer
 *
 * @return              NANOCBOR_OK on success
 */
int nanocbor_get_tag(nanocbor_value_t *cvalue, uint32_t *tag);

/**
 * @brief Retrieve a tag as positive uint64_t from the stream
 *
 * The resulting @p value is undefined if the result is an error condition
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  tag     returned tag as positive integer
 *
 * @return              NANOCBOR_OK on success
 */
int nanocbor_get_tag64(nanocbor_value_t *cvalue, uint64_t *tag);

/**
 * @brief Retrieve a null value from the stream
 *
 * This function checks if the next CBOR value is a NULL value and advances to
 * the next value if no error is detected
 *
 * @param[in]   cvalue  CBOR value to decode from
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_get_null(nanocbor_value_t *cvalue);

/**
 * @brief Retrieve a boolean value from the stream
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   Boolean value retrieved from the stream
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_get_bool(nanocbor_value_t *cvalue, bool *value);

/**
 * @brief Retrieve a simple value of undefined from the stream
 *
 * @param[in]   cvalue  CBOR value to decode from
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_get_undefined(nanocbor_value_t *cvalue);

/**
 * @brief Retrieve a simple value as integer from the stream
 *
 * This function returns the simple value as uint8_t value and skips decoding
 * the meaning of the values. For example, a CBOR true is returned as value 21.
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   Simple value retrieved from the stream
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_get_simple(nanocbor_value_t *cvalue, uint8_t *value);

/**
 * @brief Retrieve a float value from the stream.
 *
 * This function automatically converts CBOR half floats into 32 bit floating
 * points.
 *
 * @note This function assumes the host uses ieee754 to represent floating point
 * numbers
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   Simple value retrieved from the stream
 *
 * @return              number of bytes read on success
 * @return              negative on error
 */
int nanocbor_get_float(nanocbor_value_t *cvalue, float *value);

/**
 * @brief Retrieve a double-sized floating point value from the stream.
 *
 * This function automatically converts CBOR half floats and 32 bit floats into
 * into 64 bit floating points.
 *
 * @note This function assumes the host uses ieee754 to represent floating point
 * numbers
 *
 * @param[in]   cvalue  CBOR value to decode from
 * @param[out]  value   Simple value retrieved from the stream
 *
 * @return              number of bytes read on success
 * @return              negative on error
 */
int nanocbor_get_double(nanocbor_value_t *cvalue, double *value);

/**
 * @brief Skip to the next value in the CBOR stream
 *
 * This function is able to skip over nested structures in the CBOR stream
 * such as (nested) arrays and maps. For indefinite length containers,
 * it resorts to limited recursion to do so.
 *
 * Recursion is limited with @ref NANOCBOR_RECURSION_MAX
 *
 * @param[in]   it  CBOR stream to skip a value from
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_skip(nanocbor_value_t *it);

/**
 * @brief Skip a single simple value in the CBOR stream
 *
 * This is a cheaper version of @ref nanocbor_skip, the downside is that this
 * function is unable to skip nested structures, i.e., tags, maps and arrays.
 *
 * @param[in]   it  CBOR value to skip
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_skip_simple(nanocbor_value_t *it);

/**
 * @brief Retrieve part of the CBOR stream for separate parsing
 *
 * This function retrieves the pointer and length of a single CBOR item. This
 * item can be stored for later processing.
 *
 * @param[in]   it      CBOR value to retrieve
 * @param[out]  start   start of the CBOR item
 * @param[out]  len     length of the CBOR item
 *
 * @return              NANOCBOR_OK on success
 * @return              negative on error
 */
int nanocbor_get_subcbor(nanocbor_value_t *it, const uint8_t **start,
                         size_t *len);

/**
 * @brief Retrieve the number of remaining values in a CBOR container: either array or map
 *
 * The returned value is undefined when not inside a container or when the
 * container is of indefinite length. For a map, the number is the full number
 * of CBOR items remaining (twice the number of key/value pairs).
 *
 * See also nanocbor_array_items_remaining and nanocbor_map_items_remaining
 *
 * @param[in]   value   value inside a CBOR container
 *
 * @return              number of items remaining
 */
static inline uint32_t
nanocbor_container_remaining(const nanocbor_value_t *value)
{
    return value->remaining;
}

/**
 * @brief Retrieve the number of remaining items in a CBOR array
 *
 * The returned value is undefined when not inside an array or when the
 * array is of indefinite length.
 *
 * @param[in]   value   nanocbor value inside a CBOR array
 *
 * @return              number of array items remaining
 */
static inline uint32_t
nanocbor_array_items_remaining(const nanocbor_value_t *value)
{
    return nanocbor_container_remaining(value);
}

/**
 * @brief Retrieve the number of remaining values in a CBOR map
 *
 * The returned value is undefined when not inside a map or when the
 * container is of indefinite length.
 *
 * @param[in]   value   nanocbor value inside a CBOR map
 *
 * @return              number of key/value pairs remaining
 */
static inline uint32_t
nanocbor_map_items_remaining(const nanocbor_value_t *value)
{
    return nanocbor_container_remaining(value)/2;
}

/**
 * @brief Check whether a container is an indefinite-length container
 *
 * @param[in]   container   value inside a CBOR container
 *
 * @return                  True when the container is indefinite in length
 * @return                  False when not indefinite-length or not in a
 *                          container
 */
static inline bool
nanocbor_container_indefinite(const nanocbor_value_t *container)
{
    return (container->flags
            == (NANOCBOR_DECODER_FLAG_INDEFINITE
                | NANOCBOR_DECODER_FLAG_CONTAINER));
}

static inline bool nanocbor_in_container(const nanocbor_value_t *container)
{
    return container->flags & (NANOCBOR_DECODER_FLAG_CONTAINER);
}

/** @} */

/**
 * @name NanoCBOR encoder functions
 * @{
 */

/**
 * @brief Initializes an encoder context with a memory buffer.
 *
 * It is safe to pass `NULL` to @p buf with @p len is `0` to determine the size
 * of a CBOR structure.
 *
 * @param[in]   enc     Encoder context
 * @param[in]   buf     Buffer to write into
 * @param[in]   len     length of the buffer
 */
void nanocbor_encoder_init(nanocbor_encoder_t *enc, uint8_t *buf, size_t len);

/**
 * @brief Initializes an encoder context with custom append and fits functions.
 *
 * @param[in]   enc         Encoder context
 * @param[in]   ctx         Context pointer
 * @param[in]   append_func Called to append emitted encoder data
 * @param[in]   fits_func   Called to check if data can be consumed
 */
void nanocbor_encoder_stream_init(nanocbor_encoder_t *enc, void *ctx,
                                  nanocbor_encoder_append append_func,
                                  nanocbor_encoder_fits fits_func);

/**
 * @brief Retrieve the encoded length of the CBOR structure
 *
 * This function doesn't take the length of the buffer supplied to
 * @ref nanocbor_encoder_init into account, it only returns the number of bytes
 * the current CBOR structure would take up.
 *
 * @param[in]   enc     Encoder context
 *
 * @return              Length of the encoded structure
 */
size_t nanocbor_encoded_len(nanocbor_encoder_t *enc);

/**
 * @brief Write a CBOR boolean value into a buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   content Boolean value to write
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_bool(nanocbor_encoder_t *enc, bool content);

/**
 * @brief Write an unsigned integer of at most sizeof uint64_t into the buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   num     unsigned integer to write
 *
 * @return              number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_uint(nanocbor_encoder_t *enc, uint64_t num);

/**
 * @brief Write a CBOR tag of at most sizeof uint64_t into the buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   num     tag value to write into the buffer
 *
 * @return              number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_tag(nanocbor_encoder_t *enc, uint64_t num);

/**
 * @brief Write a signed integer of at most sizeof int32_t into the buffer
 *
 * If it is not certain if the data is signed, use this function.
 *
 * @param[in]   enc     Encoder context
 * @param[in]   num     unsigned integer to write
 *
 * @return              number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_int(nanocbor_encoder_t *enc, int64_t num);

/**
 * @brief Write a CBOR simple value into the encoder buffer
 *
 * Disallows assigned and reserved simple values between 20 and 31.
 * Please use the respective functions for emitting true, false, undefined, and null.
 *
 * @param[in]   enc     Encoder context
 * @param[in]   value   Simple value to write
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_simple(nanocbor_encoder_t *enc, uint8_t value);

/**
 * @brief Write a byte string indicator for a byte string with specific length
 * into the encoder buffer
 *
 * This doesn't write any byte string into the encoder buffer, only the type
 * and length indicator for the byte string
 *
 * @param[in]   enc     Encoder context
 * @param[in]   len     Length of the byte string
 *
 * @return              number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_bstr(nanocbor_encoder_t *enc, size_t len);

/**
 * @brief Write a text string indicator for a string with specific length
 * into the encoder buffer
 *
 * This doesn't write any text string into the encoder buffer, only the type
 * and length indicator for the text string
 *
 * @param[in]   enc     Encoder context
 * @param[in]   len     Length of the text string
 *
 * @return              number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_tstr(nanocbor_encoder_t *enc, size_t len);

/**
 * @brief Copy a byte string with indicator into the encoder buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   str     byte string to encode
 * @param[in]   len     Length of the string
 *
 * @return              NANOCBOR_OK if the string fits
 * @return              Negative on error
 */
int nanocbor_put_bstr(nanocbor_encoder_t *enc, const uint8_t *str, size_t len);

/**
 * @brief Copy a text string with indicator into the encoder buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   str     null terminated text string to encode
 *
 * @return              NANOCBOR_OK if the string fits
 * @return              Negative on error
 */
int nanocbor_put_tstr(nanocbor_encoder_t *enc, const char *str);

/**
 * @brief Copy n bytes of a text string with indicator into the encoder buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   str     text string to encode
 * @param[in]   len     number of string bytes to copy
 *
 * @return              NANOCBOR_OK if the string fits
 * @return              Negative on error
 */
int nanocbor_put_tstrn(nanocbor_encoder_t *enc, const char *str, size_t len);

/**
 * @brief Write an array indicator with @p len items
 *
 * It is assumed that the calling code will encode @p len items after calling
 * this function. The array automatically terminates after @p len items are
 * added, no function to close the container is necessary.
 *
 * @param[in]   enc     Encoder context
 * @param[in]   len     Number of items in the array
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_array(nanocbor_encoder_t *enc, size_t len);

/**
 * @brief Write a map indicator with @p len pairs
 *
 * It is assumed that the calling code will encode @p len item pairs after
 * calling this function. The array automatically terminates after @p len item
 * pairs are added, no function to close the container is necessary.
 *
 * @param[in]   enc     Encoder context
 * @param[in]   len     Number of pairs in the map
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_map(nanocbor_encoder_t *enc, size_t len);

/**
 * @brief Write an indefinite-length array indicator
 *
 * @param[in]   enc     Encoder context
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_array_indefinite(nanocbor_encoder_t *enc);

/**
 * @brief Write an indefinite-length map indicator
 *
 * @param[in]   enc     Encoder context
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_map_indefinite(nanocbor_encoder_t *enc);

/**
 * @brief Write a stop code for indefinite length containers
 *
 * @param[in]   enc     Encoder context
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_end_indefinite(nanocbor_encoder_t *enc);

/**
 * @brief Write a Null value into the encoder buffer
 *
 * @param[in]   enc     Encoder context
 *
 * @return              NANOCBOR_OK
 * @return              Negative on error
 */
int nanocbor_fmt_null(nanocbor_encoder_t *enc);

/**
 * @brief Write an Undefined value into the encoder buffer
 *
 * @param[in]   enc     Encoder context
 *
 * @return              NANOCBOR_OK
 * @return              Negative on error
 */
int nanocbor_fmt_undefined(nanocbor_encoder_t *enc);

/**
 * @brief Write a float value into the encoder buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   num     Floating point to encode
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_float(nanocbor_encoder_t *enc, float num);

/**
 * @brief Write a double floating point value into the encoder buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   num     Floating point to encode
 *
 * @return              Number of bytes written
 * @return              Negative on error
 */
int nanocbor_fmt_double(nanocbor_encoder_t *enc, double num);

/**
 * @brief Write a decimal fraction into the encoder buffer
 *
 * @param[in]   enc     Encoder context
 * @param[in]   m       Mantisa
 * @param[in]   e       Exponent
 *
 * @return              Number of bytes written
 */
int nanocbor_fmt_decimal_frac(nanocbor_encoder_t *enc, int32_t e, int32_t m);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NANOCBOR_NANOCBOR_H */
/** @} */
