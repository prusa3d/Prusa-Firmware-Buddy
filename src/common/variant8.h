// variant8.h

#ifndef _VARIANT8_H
#define _VARIANT8_H

#include <inttypes.h>

#define VARIANT8_EMPTY 0x00 // empty - no data
#define VARIANT8_I8    0x01 // signed char - 1byte
#define VARIANT8_UI8   0x02 // unsigned char - 1byte
#define VARIANT8_I16   0x03 // signed short - 2byte
#define VARIANT8_UI16  0x04 // unsigned short - 2byte
#define VARIANT8_I32   0x05 // signed long - 4byte
#define VARIANT8_UI32  0x06 // unsigned long - 4byte
#define VARIANT8_FLT   0x07 // float - 4byte
#define VARIANT8_USER  0x3f // user - up to 7 bytes
#define VARIANT8_PTR   0x40 // pointer - 4 bytes
#define VARIANT8_ARRAY 0x80 // array
#define VARIANT8_ERROR 0xff // error

#pragma pack(push)
#pragma pack(1)

typedef struct _variant8_t {
    uint8_t type;
    uint8_t usr8;
    uint16_t usr16;
    union {
        char *pc;
        uint32_t usr32;
        float flt;
        uint32_t ui32;
        int32_t i32;
        uint16_t ui16;
        int16_t i16;
        uint8_t ui8;
        int8_t i8;
    };
} variant8_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern variant8_t variant8_empty(void); // returns VARIANT8_EMPTY

extern variant8_t variant8_i8(int8_t i8); // returns VARIANT8_I8

extern variant8_t variant8_ui8(uint8_t ui8); // returns VARIANT8_UI8

extern variant8_t variant8_i16(int16_t i16); // returns VARIANT8_I16

extern variant8_t variant8_ui16(uint16_t ui16); // returns VARIANT8_UI16

extern variant8_t variant8_i32(int32_t i32); // returns VARIANT8_I32

extern variant8_t variant8_ui32(uint32_t ui32); // returns VARIANT8_UI32

extern variant8_t variant8_flt(float flt); // returns VARIANT8_FLT

extern variant8_t variant8_user(uint32_t usr32); // returns VARIANT8_USER

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_VARIANT8_H
