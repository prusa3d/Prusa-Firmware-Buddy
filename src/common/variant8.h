// variant8.h

#pragma once

#include <inttypes.h>
#include <stdbool.h>

enum {
    VARIANT8_EMPTY = 0x00, // empty - no data
    VARIANT8_I8 = 0x01, // signed char - 1byte
    VARIANT8_BOOL = VARIANT8_I8, // bool - 1byte
    VARIANT8_UI8 = 0x02, // unsigned char - 1byte
    VARIANT8_I16 = 0x03, // signed short - 2byte
    VARIANT8_UI16 = 0x04, // unsigned short - 2byte
    VARIANT8_I32 = 0x05, // signed long - 4byte
    VARIANT8_UI32 = 0x06, // unsigned long - 4byte
    VARIANT8_FLT = 0x07, // float - 4byte
    VARIANT8_CHAR = 0x08, // char - 1byte
    VARIANT8_USER = 0x09, // user - up to 7 bytes
    VARIANT8_PTR = 0x80, // pointer - 4 bytes,
    VARIANT8_ERROR = 0x3f, // error
    VARIANT8_PTR_OWNER = 0x40, // pointer ownership
                               // pointer types
    VARIANT8_PI8 = (VARIANT8_I8 | VARIANT8_PTR),
    VARIANT8_PUI8 = (VARIANT8_UI8 | VARIANT8_PTR),
    VARIANT8_PI16 = (VARIANT8_I16 | VARIANT8_PTR),
    VARIANT8_PUI16 = (VARIANT8_UI16 | VARIANT8_PTR),
    VARIANT8_PI32 = (VARIANT8_I32 | VARIANT8_PTR),
    VARIANT8_PUI32 = (VARIANT8_UI32 | VARIANT8_PTR),
    VARIANT8_PFLT = (VARIANT8_FLT | VARIANT8_PTR),
    VARIANT8_PCHAR = (VARIANT8_CHAR | VARIANT8_PTR),
};

// variant errors
enum {
    VARIANT8_ERR_MALLOC = 1, // memory allocation error (during conversion to strings and allocating pointer types)
    VARIANT8_ERR_UNSTYP, // unsupported conversion (during conversion)
    VARIANT8_ERR_UNSCON, // unsupported conversion (during conversion)
    VARIANT8_ERR_INVFMT, // invalid format (during conversion from string)
    VARIANT8_ERR_OOFRNG, // out of range (during conversion from bigger to lower range number)
};

#if INTPTR_MAX == INT32_MAX // 32 bit system
typedef uint64_t variant8_t;
#elif INTPTR_MAX == INT64_MAX // 64 bit system
typedef unsigned __int128 variant8_t;
#endif

#ifdef __cplusplus

extern "C" {
#endif //__cplusplus

// returns newly allocated variant8, copy data from pdata if not null
extern variant8_t variant8_init(uint8_t type, uint16_t count, void const *pdata);

// free allocated pointer for VARIANT8_PTR types, sets pvar8 to VARIANT8_EMPTY
extern void variant8_done(variant8_t **pvar8);

// returns copy of pvar8, allocate pointer and copy data for VARIANT8_PTR types
extern variant8_t variant8_copy(const variant8_t *pvar8);

// perform implicit conversion to desired data type (supported conversions described at end of .c file)
extern int variant8_change_type(variant8_t *pvar8, uint8_t type);

// returns VARIANT8_EMPTY
extern variant8_t variant8_empty(void);

// returns VARIANT8_I8
extern variant8_t variant8_i8(int8_t i8);

// returns VARIANT8_BOOL
extern variant8_t variant8_bool(bool b);

// returns VARIANT8_UI8
extern variant8_t variant8_ui8(uint8_t ui8);

// returns VARIANT8_I16
extern variant8_t variant8_i16(int16_t i16);

// returns VARIANT8_UI16
extern variant8_t variant8_ui16(uint16_t ui16);

// returns VARIANT8_I32
extern variant8_t variant8_i32(int32_t i32);

// returns VARIANT8_UI32
extern variant8_t variant8_ui32(uint32_t ui32);

// returns VARIANT8_FLT
extern variant8_t variant8_flt(float flt);

// returns VARIANT8_PI8
extern variant8_t variant8_pi8(int8_t *pi8, uint16_t count, int init);

// returns VARIANT8_PUI8
extern variant8_t variant8_pui8(uint8_t *pui8, uint16_t count, int init);

// returns VARIANT8_PI16
extern variant8_t variant8_pi16(int16_t *pi16, uint16_t count, int init);

// returns VARIANT8_PUI16
extern variant8_t variant8_pui16(uint16_t *pui16, uint16_t count, int init);

// returns VARIANT8_PI32
extern variant8_t variant8_pi32(int32_t *pi32, uint16_t count, int init);

// returns VARIANT8_PUI32
extern variant8_t variant8_pui32(uint32_t *pui32, uint16_t count, int init);

// returns VARIANT8_PFLT
extern variant8_t variant8_pflt(float *pflt, uint16_t count, int init);

// returns VARIANT8_ERROR
extern variant8_t variant8_error(uint32_t err32, uint16_t err16, uint8_t err8);

// returns variant8_t type
extern uint8_t variant8_get_type(variant8_t v);
// returns variant8_t usr8
extern uint8_t variant8_get_usr8(variant8_t v);
// returns variant8_t usr16
extern uint16_t variant8_get_usr16(variant8_t v);
// returns variant8_t flt
extern float variant8_get_flt(variant8_t v);
// returns variant8_t pch
extern char *variant8_get_pch(variant8_t v);
// returns variant8_t ui8
extern uint8_t variant8_get_uia(variant8_t v, uint8_t index);

// returns variant8_t ui32
extern uint32_t variant8_get_ui32(variant8_t v);

// returns variant8_t i32
extern int32_t variant8_get_i32(variant8_t v);

// returns variant8_t ui16
extern uint16_t variant8_get_ui16(variant8_t v);

// returns variant8_t i16
extern int16_t variant8_get_i16(variant8_t v);

// returns variant8_t ui8
extern uint8_t variant8_get_ui8(variant8_t v);

// returns variant8_t i8
extern int8_t variant8_get_i8(variant8_t v);

// returns variant8_t bool
extern bool variant8_get_bool(variant8_t v);

// set variant8_t usr8 member
extern void variant8_set_usr8(variant8_t *, uint8_t);

// set variant8_t usr8 member
extern void variant8_set_type(variant8_t *, uint8_t);

// returns VARIANT8_PCHAR
// Because PCHAR is special case of pointer type, there is a simplification for defining size.
// In case that count=0 and init=1 is used strlen to measure size of original string.
extern variant8_t variant8_pchar(char *pch, uint16_t count, int init);

// returns VARIANT8_USER
extern variant8_t variant8_user(uint32_t usr32, uint16_t usr16, uint8_t usr8);

// returns size of data type
extern uint16_t variant8_type_size(uint8_t type);

// returns size of data stored in variant
extern uint16_t variant8_data_size(variant8_t *pvar8);

// returns pointer to data stored in variant
extern void *variant8_data_ptr(variant8_t *pvar8);

// returns pointer to typename (e.g. "PCHAR" for VARIANT8_PCHAR)
const char *variant8_typename(uint8_t type);

// format simple variant8 types to string
// same behavior as normal snprintf except for fmt==null - in this case default formating will be used
extern int variant8_snprintf(char *str, unsigned int size, const char *fmt, variant8_t *pvar8);

// format variant8 with variant8_snprintf, returns pointer to string allocated using variant8_malloc
extern char *variant8_to_str(variant8_t *pvar8, const char *fmt);

// returns variant8 with desired type parsed from string with sscanf
extern variant8_t variant8_from_str(uint8_t type, char *str);

// variant8 realloc function
extern void *variant8_realloc(void *ptr, uint16_t size);

// // returns 1 for signed integer types (I8, I16, I32), otherwise returns 0
// inline int variant8_is_signed(const variant8_t *pvar8) { return (pvar8) ? (((pvar8->type == VARIANT8_I8) || (pvar8->type == VARIANT8_I16) || (pvar8->type == VARIANT8_I32)) ? 1 : 0) : 0; }

// // returns 1 for unsigned integer types (UI8, UI16, UI32), otherwise returns 0
// inline int variant8_is_unsigned(const variant8_t *pvar8) { return (pvar8) ? (((pvar8->type == VARIANT8_I8) || (pvar8->type == VARIANT8_I16) || (pvar8->type == VARIANT8_I32)) ? 1 : 0) : 0; }

// // returns 1 for integer types (I8, I16, I32, UI8, UI16, UI32), otherwise returns 0
// inline int variant8_is_integer(const variant8_t *pvar8) { return (pvar8) ? ((variant8_is_signed(pvar8) || variant8_is_unsigned(pvar8)) ? 1 : 0) : 0; }

// // returns 1 for numeric types (I8, I16, I32, UI8, UI16, UI32, float), otherwise returns 0
// inline int variant8_is_number(const variant8_t *pvar8) { return (pvar8) ? ((variant8_is_integer(pvar8) || (pvar8->type == VARIANT8_FLT)) ? 1 : 0) : 0; }

#ifdef __cplusplus
}
#endif //__cplusplus
