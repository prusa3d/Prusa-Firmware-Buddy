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
#define VARIANT8_PI8   (VARIANT8_I8 | VARIANT8_PTR)
#define VARIANT8_PUI8  (VARIANT8_UI8 | VARIANT8_PTR)
#define VARIANT8_PI16  (VARIANT8_I16 | VARIANT8_PTR)
#define VARIANT8_PUI16 (VARIANT8_UI16 | VARIANT8_PTR)
#define VARIANT8_PI32  (VARIANT8_I32 | VARIANT8_PTR)
#define VARIANT8_PUI32 (VARIANT8_UI32 | VARIANT8_PTR)
#define VARIANT8_PFLT  (VARIANT8_FLT | VARIANT8_PTR)
#define VARIANT8_PCHAR (VARIANT8_CHAR | VARIANT8_PTR)

//variant errors
#define VARIANT8_ERR_MALLOC 1 // memory allocation error (during conversion to strings and allocating pointer types)
#define VARIANT8_ERR_UNSTYP 2 // unsupported conversion (during conversion)
#define VARIANT8_ERR_UNSCON 3 // unsupported conversion (during conversion)
#define VARIANT8_ERR_INVFMT 4 // invalid format (during conversion from string)
#define VARIANT8_ERR_OOFRNG 5 // out of range (during conversion from bigger to lower range number)

//macros for variant8 structure constants
#define _VARIANT8_TYPE(type, _8, _16, _32) ((variant8_t) { type, _8, { _16 }, { _32 } })
#define _VARIANT8_EMPTY()                  _VARIANT8_TYPE(VARIANT8_EMPTY, 0, 0, 0)

#pragma pack(push)
#pragma pack(1)

typedef struct _variant8_t {
    uint8_t type;
    uint8_t usr8;
    union {
        uint16_t usr16;
        uint16_t size;
        uint16_t err16;
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
        uint32_t err32;
    };
} variant8_t;

#pragma pack(pop)

#ifdef __cplusplus

class cvariant8 : public variant8_t {
public: // construction/destruction
    cvariant8();
    cvariant8(const cvariant8 &var8);
    #if __cplusplus >= 201103L
    cvariant8(cvariant8 &&var8);
    #endif
    cvariant8(int8_t val);
    cvariant8(uint8_t val);
    cvariant8(int16_t val);
    cvariant8(uint16_t val);
    cvariant8(int32_t val);
    cvariant8(uint32_t val);
    cvariant8(float val);
    cvariant8(const char *val);
    ~cvariant8();

public: // public functions
    cvariant8 copy();
    cvariant8 &attach(variant8_t var8);
    variant8_t detach();
    cvariant8 &change_type(uint8_t new_type);

public: //
    bool is_empty() const;
    bool is_error() const;
    bool is_signed() const;
    bool is_unsigned() const;
    bool is_integer() const;
    bool is_number() const;

public: // assignment operators
    cvariant8 &operator=(const cvariant8 &var8);
    #if __cplusplus >= 201103L
    cvariant8 &operator=(cvariant8 &&var8);
    #endif
    cvariant8 &operator=(int8_t val);
    cvariant8 &operator=(uint8_t val);
    cvariant8 &operator=(int16_t val);
    cvariant8 &operator=(uint16_t val);
    cvariant8 &operator=(int32_t val);
    cvariant8 &operator=(uint32_t val);
    cvariant8 &operator=(float val);
    cvariant8 &operator=(const char *val);

private:
    int32_t get_valid_int() const; //helper for extractors, works ony on integer values
public:                            // extractors
    // clang-format off
    operator int8_t()       const { return (is_integer())           ?   int8_t(get_valid_int()) : ((type == VARIANT8_FLT) ?   int8_t(flt)           : 0); }
    operator uint8_t()      const { return (is_integer())           ?  uint8_t(get_valid_int()) : ((type == VARIANT8_FLT) ?  uint8_t(flt)           : 0); }
    operator int16_t()      const { return (is_integer())           ?  int16_t(get_valid_int()) : ((type == VARIANT8_FLT) ?  int16_t(flt)           : 0); }
    operator uint16_t()     const { return (is_integer())           ? uint16_t(get_valid_int()) : ((type == VARIANT8_FLT) ? uint16_t(flt)           : 0); }
    operator int32_t()      const { return (is_integer())           ?  int32_t(get_valid_int()) : ((type == VARIANT8_FLT) ?  int32_t(flt)           : 0); }
    operator uint32_t()     const { return (is_integer())           ? uint32_t(get_valid_int()) : ((type == VARIANT8_FLT) ? uint32_t(flt)           : 0); }
    operator float()        const { return (type == VARIANT8_FLT)   ? flt                       : ((is_integer())         ? float(get_valid_int())  : 0); }
    operator const char *() const { return (type == VARIANT8_PCHAR) ? pch : 0; }
    // clang-format on
private:
    enum class operator_x { plus,
        minus,
        multiplies,
        divides }; //float has no modulus
    //T should be int types or float - better pass by value
    template <class T>
    T calc(T lhs, T rhs, operator_x op) {
        switch (op) {
        case operator_x::minus:
            return lhs - rhs;
        case operator_x::multiplies:
            return lhs * rhs;
        case operator_x::divides:
            return lhs / rhs;
        case operator_x::plus:
        default: //avoid warning
            return lhs + rhs;
        }
    }
    //used by assigment operators like +=
    cvariant8 &assigment_operator_x(const cvariant8 &rhs, operator_x op);

public: //arithmetic operators
    cvariant8 &operator+=(const cvariant8 &rhs) {
        return assigment_operator_x(rhs, operator_x::plus);
    }
    cvariant8 &operator-=(const cvariant8 &rhs) {
        return assigment_operator_x(rhs, operator_x::minus);
    }
    cvariant8 &operator*=(const cvariant8 &rhs) {
        return assigment_operator_x(rhs, operator_x::multiplies);
    }
    cvariant8 &operator/=(const cvariant8 &rhs) {
        return assigment_operator_x(rhs, operator_x::divides);
    }

    // friends defined inside class body are inline and are hidden from non-ADL lookup
    // passing lhs by value helps optimize chained a+b+c
    // otherwise, both parameters may be const references
    friend cvariant8 operator+(cvariant8 lhs, const cvariant8 &rhs) {
        lhs += rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }
    friend cvariant8 operator-(cvariant8 lhs, const cvariant8 &rhs) {
        lhs -= rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }
    friend cvariant8 operator*(cvariant8 lhs, const cvariant8 &rhs) {
        lhs *= rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }
    friend cvariant8 operator/(cvariant8 lhs, const cvariant8 &rhs) {
        lhs /= rhs; // reuse compound assignment
        return lhs; // return the result by value (uses move constructor)
    }

protected:
};

extern "C" {
#endif //__cplusplus

// returns newly allocated variant8, copy data from pdata if not null
extern variant8_t variant8_init(uint8_t type, uint16_t size, void *pdata);

// free allocated pointer for VARIANT8_PTR types, sets pvar8 to VARIANT8_EMPTY
extern void variant8_done(variant8_t *pvar8);

// returns copy of pvar8, allocate pointer and copy data for VARIANT8_PTR types
extern variant8_t variant8_copy(const variant8_t *pvar8);

// perform implicit conversion to desired data type (supported conversions described at end of .c file)
extern int variant8_change_type(variant8_t *pvar8, uint8_t type);

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

// returns VARIANT8_PCHAR
extern variant8_t variant8_pchar(char *pch, uint16_t count, int init);

// returns VARIANT8_USER
extern variant8_t variant8_user(uint32_t usr32, uint16_t usr16, uint8_t usr8);

// returns VARIANT8_ERROR
extern variant8_t variant8_error(uint32_t err32, uint16_t err16, uint8_t err8);

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
extern variant8_t variant8_from_str(uint8_t type, char *str, const char *fmt);

// variant8 malloc function
extern void *variant8_malloc(uint16_t size);

// variant8 free function
extern void variant8_free(void *ptr);

// variant8 realloc function
extern void *variant8_realloc(void *ptr, uint16_t size);

// returns 1 for signed integer types (I8, I16, I32), otherwise returns 0
inline static int variant8_is_signed(const variant8_t *pvar8) { return (pvar8) ? (((pvar8->type == VARIANT8_I8) || (pvar8->type == VARIANT8_I16) || (pvar8->type == VARIANT8_I32)) ? 1 : 0) : 0; }

// returns 1 for unsigned integer types (UI8, UI16, UI32), otherwise returns 0
inline static int variant8_is_unsigned(const variant8_t *pvar8) { return (pvar8) ? (((pvar8->type == VARIANT8_I8) || (pvar8->type == VARIANT8_I16) || (pvar8->type == VARIANT8_I32)) ? 1 : 0) : 0; }

// returns 1 for integer types (I8, I16, I32, UI8, UI16, UI32), otherwise returns 0
inline static int variant8_is_integer(const variant8_t *pvar8) { return (pvar8) ? ((variant8_is_signed(pvar8) || variant8_is_unsigned(pvar8)) ? 1 : 0) : 0; }

// returns 1 for numeric types (I8, I16, I32, UI8, UI16, UI32, float), otherwise returns 0
inline static int variant8_is_number(const variant8_t *pvar8) { return (pvar8) ? ((variant8_is_integer(pvar8) || (pvar8->type == VARIANT8_FLT)) ? 1 : 0) : 0; }

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_VARIANT8_H
