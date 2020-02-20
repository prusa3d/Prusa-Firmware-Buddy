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
#define VARIANT8_CHAR  0x08 // char - 1byte
#define VARIANT8_USER  0x3f // user - up to 7 bytes
#define VARIANT8_PTR   0x80 // pointer - 4 bytes,
#define VARIANT8_ERROR 0xff // error

//pointer types
#define VARIANT8_PI8   (VARIANT8_I8   | VARIANT8_PTR)
#define VARIANT8_PUI8  (VARIANT8_UI8  | VARIANT8_PTR)
#define VARIANT8_PI16  (VARIANT8_I16  | VARIANT8_PTR)
#define VARIANT8_PUI16 (VARIANT8_UI16 | VARIANT8_PTR)
#define VARIANT8_PI32  (VARIANT8_I32  | VARIANT8_PTR)
#define VARIANT8_PUI32 (VARIANT8_UI32 | VARIANT8_PTR)
#define VARIANT8_PFLT  (VARIANT8_FLT  | VARIANT8_PTR)
#define VARIANT8_PCHAR (VARIANT8_CHAR | VARIANT8_PTR)


#pragma pack(push)
#pragma pack(1)

typedef struct _variant8_t {
    uint8_t type;
    uint8_t usr8;
    union {
        uint16_t usr16;
        uint16_t size;
    };
    union {
        void *ptr;
        char *pch;
        float *pflt;
        uint32_t *pui32;
        int32_t *pi32;
        uint16_t *pui16;
        int16_t *pi16;
        uint8_t *pui8;
        int8_t *pi8;
        uint32_t usr32;
        char ch;
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


// returns newly allocated variant8, copy data from pdata if not null
extern variant8_t variant8_init(uint8_t type, uint16_t size, void* pdata);

// free allocated pointer for VARIANT8_PTR types, sets EMPTY
extern void variant8_done(variant8_t* pvar8);

// returns copy of pvar8, allocate pointer and copy data for VARIANT8_PTR types
extern variant8_t variant8_copy(variant8_t* pvar8);

// returns VARIANT8_EMPTY
extern variant8_t variant8_empty(void);

// returns VARIANT8_I8
extern variant8_t variant8_i8(int8_t i8);

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

// returns VARIANT8_PCHAR
extern variant8_t variant8_pchar(const char* pch);

// returns VARIANT8_USER
extern variant8_t variant8_user(uint32_t usr32, uint16_t usr16, uint8_t usr8);

// returns size of data type
extern uint16_t variant8_type_size(uint8_t type);

// returns size of data stored in variant
extern uint16_t variant8_data_size(variant8_t* pvar8);

// returns pointer to data stored in variant
extern void* variant8_data_ptr(variant8_t* pvar8);

// variant8 malloc function
extern void* variant8_malloc(uint16_t size);

// variant8 free function
extern void variant8_free(void* ptr);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_VARIANT8_H
