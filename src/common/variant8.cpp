// variant8.cpp

#include "variant8.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>

#define VARIANT8_DBG_MALLOC

#define _VARIANT8_TYPE(type, _8, _16, _32) ((_variant8_t) { { _32 }, { _16 }, type, _8 })
#define _VARIANT8_EMPTY()                  _VARIANT8_TYPE(VARIANT8_EMPTY, 0, .size = 0, .ui32 = 0)

struct _variant8_t {
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
        uint8_t ui8a[4]; // array for easy 8 <-> 32 bit conversion
        int32_t i32;
        uint16_t ui16;
        int16_t i16;
        uint8_t ui8;
        int8_t i8;
        uint32_t err32;
    };
    union {
        uint16_t usr16;
        uint16_t size;
        uint16_t err16;
    };
    uint8_t type;
    uint8_t usr8;
};

static_assert(sizeof(_variant8_t) == sizeof(variant8_t), "Incompatible types");

extern "C" {

static inline variant8_t *pack(_variant8_t *v) {
    return (variant8_t *)v;
}

static inline _variant8_t *unpack(variant8_t *v) {
    return (_variant8_t *)v;
}

// variant8 malloc function
static void *variant8_malloc(uint16_t size);

// variant8 free function
static void variant8_free(void *ptr);

// macros for variant8 structure constants
variant8_t variant8_init(uint8_t type, uint16_t count, void const *pdata) {
    _variant8_t var8;
    uint16_t size;
    if ((count == 1) && !(type & VARIANT8_PTR)) {
        var8 = _VARIANT8_TYPE(type, 0, .size = 0, .ui32 = 0);
        if (pdata) {
            switch (type) {
            case VARIANT8_I8:
                var8.i8 = *((int8_t *)pdata);
                break;
            case VARIANT8_UI8:
                var8.ui8 = *((uint8_t *)pdata);
                break;
            case VARIANT8_I16:
                var8.i16 = *((int16_t *)pdata);
                break;
            case VARIANT8_UI16:
                var8.ui16 = *((uint16_t *)pdata);
                break;
            case VARIANT8_I32:
                var8.i32 = *((int32_t *)pdata);
                break;
            case VARIANT8_UI32:
                var8.ui32 = *((uint32_t *)pdata);
                break;
            case VARIANT8_FLT:
                var8.flt = *((float *)pdata);
                break;
            }
        }
    } else if ((count > 1) && (type & VARIANT8_PTR)) {
        size = variant8_type_size(type & ~VARIANT8_PTR) * count;
        var8 = _VARIANT8_TYPE(type, 0, .size = size, 0);
        if (size) {
            var8.ptr = variant8_malloc(size);
            if (var8.ptr) {
                var8.type |= VARIANT8_PTR_OWNER;
            } else {
                return variant8_error(VARIANT8_ERR_MALLOC, 0, 0);
            }
        }
        if (pdata) {
            memcpy(var8.ptr, pdata, size);
        }
    } else {
        return variant8_error(VARIANT8_ERR_UNSTYP, 0, 0);
    }
    return *pack(&var8);
}

void variant8_done(variant8_t **pvar8) {
    if (pvar8) {
        _variant8_t *v = unpack(*pvar8);
        if ((v->type & (~VARIANT8_ERROR)) == (VARIANT8_PTR_OWNER | VARIANT8_PTR)
            && v->size) {
            variant8_free(v->ptr);
        }
        *v = _VARIANT8_EMPTY();
        *pvar8 = pack(v);
    }
}

#ifdef CLEAN_UNUSED

variant8_t variant8_copy(const variant8_t *pvar8) {
    variant8_t v = *pvar8;
    _variant8_t var8 = pvar8 ? *unpack(&v) : _VARIANT8_EMPTY();
    if ((var8.type & VARIANT8_PTR) && var8.size && var8.ptr) {
        void *ptr = var8.ptr;
        var8.ptr = variant8_malloc(var8.size);
        if (var8.ptr) {
            memcpy(var8.ptr, ptr, var8.size);
        } else {
            return variant8_error(VARIANT8_ERR_MALLOC, 0, 0);
        }
    }
    return *pack(&var8);
}

int variant8_change_type(variant8_t *pvar8, uint8_t type) {
    if (pvar8 == 0) {
        return 0;
    }
    if (pvar8->type == type) {
        return 1;
    }
    int ret = 1;
    variant8_t var8 = *pvar8;
    if (type == VARIANT8_PCHAR) {
        char *pch = variant8_to_str(&var8, 0);
        if (pch) {
            *pvar8 = variant8_pchar(pch, 0, 0);
        } else {
            *pvar8 = variant8_error(VARIANT8_ERR_MALLOC, 0, 0);
        }
    } else if (var8.type == VARIANT8_PCHAR) {
        *pvar8 = variant8_from_str(type, var8.pch, 0);
    } else if (type == VARIANT8_FLT) {
        switch (pvar8->type) {
        case VARIANT8_I8:
            pvar8->flt = pvar8->i8;
            break;
        case VARIANT8_I16:
            pvar8->flt = pvar8->i16;
            break;
        case VARIANT8_I32:
            pvar8->flt = pvar8->i32;
            break;
        case VARIANT8_UI8:
            pvar8->flt = pvar8->ui8;
            break;
        case VARIANT8_UI16:
            pvar8->flt = pvar8->ui16;
            break;
        case VARIANT8_UI32:
            pvar8->flt = pvar8->ui32;
            break;
        default:
            *pvar8 = variant8_error(VARIANT8_ERR_UNSCON, 0, 0);
            break;
        }
        if (pvar8->type != VARIANT8_ERROR) {
            pvar8->type = VARIANT8_FLT;
        }
    } else if (pvar8->type == VARIANT8_FLT) {
        switch (type) {
        case VARIANT8_I8:
            if ((pvar8->flt >= INT8_MIN) && (pvar8->flt <= INT8_MAX)) {
                pvar8->i8 = (int8_t)pvar8->flt;
            } else {
                *pvar8 = variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
            }
            break;
        case VARIANT8_I16:
            if ((pvar8->flt >= INT16_MIN) && (pvar8->flt <= INT16_MAX)) {
                pvar8->i16 = (int16_t)pvar8->flt;
            } else {
                *pvar8 = variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
            }
            break;
        case VARIANT8_I32:
            if ((pvar8->flt >= INT32_MIN) && (pvar8->flt <= INT32_MAX)) {
                pvar8->i32 = (int16_t)pvar8->flt;
            } else {
                *pvar8 = variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
            }
            break;
        case VARIANT8_UI8:
            if ((pvar8->flt >= 0) && (pvar8->flt <= UINT8_MAX)) {
                pvar8->ui8 = (uint8_t)pvar8->flt;
            } else {
                *pvar8 = variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
            }
            break;
        case VARIANT8_UI16:
            if ((pvar8->flt >= 0) && (pvar8->flt <= UINT16_MAX)) {
                pvar8->ui16 = (uint16_t)pvar8->flt;
            } else {
                *pvar8 = variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
            }
            break;
        case VARIANT8_UI32:
            if ((pvar8->flt >= 0) && (pvar8->flt <= UINT32_MAX)) {
                pvar8->ui32 = (uint32_t)pvar8->flt;
            } else {
                *pvar8 = variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
            }
            break;
        default:
            *pvar8 = variant8_error(VARIANT8_ERR_UNSCON, 0, 0);
            break;
        }
        if (pvar8->type != VARIANT8_ERROR) {
            pvar8->type = type;
        }
    } else if (variant8_is_integer(pvar8)) {
        // TODO: integer conversion with range checking
        *pvar8 = variant8_error(VARIANT8_ERR_UNSCON, 0, 0);
    }
    if (pvar8->type == VARIANT8_ERROR) {
        ret = 0;
    }
    variant8_done(&var8);
    return ret;
}
#endif
variant8_t variant8_empty(void) {
    _variant8_t v = _VARIANT8_EMPTY();
    return *pack(&v);
}
variant8_t variant8_i8(int8_t i8) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_I8, 0, 0, .i8 = i8);
    return *pack(&v);
}
variant8_t variant8_bool(bool b) {
    return variant8_i8(int8_t(b));
}
variant8_t variant8_ui8(uint8_t ui8) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_UI8, 0, 0, .ui8 = ui8);
    return *pack(&v);
}
variant8_t variant8_i16(int16_t i16) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_I16, 0, 0, .i16 = i16);
    return *pack(&v);
}
variant8_t variant8_ui16(uint16_t ui16) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_UI16, 0, 0, .ui16 = ui16);
    return *pack(&v);
}
variant8_t variant8_i32(int32_t i32) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_I32, 0, 0, .i32 = i32);
    return *pack(&v);
}
variant8_t variant8_ui32(uint32_t ui32) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_UI32, 0, 0, .ui32 = ui32);
    return *pack(&v);
}
variant8_t variant8_flt(float flt) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_FLT, 0, 0, .flt = flt);
    return *pack(&v);
}

variant8_t variant8_pchar(char *pch, uint16_t count, int init) {
    if (init) {
        return variant8_init(VARIANT8_PCHAR, count ? count : strlen(pch) + 1, (void *)pch);
    } else {
        _variant8_t v = _VARIANT8_TYPE(VARIANT8_PCHAR, 0, .size = (uint16_t)(count ? count : strlen(pch) + 1), .pch = pch);
        return *pack(&v);
    }
}

variant8_t variant8_user(uint32_t usr32, uint16_t usr16, uint8_t usr8) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_USER, usr8, .usr16 = usr16, .usr32 = usr32);
    return *pack(&v);
}

variant8_t variant8_error(uint32_t err32, uint16_t err16, uint8_t err8) {
    _variant8_t v = _VARIANT8_TYPE(VARIANT8_ERROR, err8, .err16 = err16, .err32 = err32);
    return *pack(&v);
}

void variant8_set_usr8(variant8_t *v, uint8_t usr) {
    unpack(v)->usr8 = usr;
}

void variant8_set_type(variant8_t *v, uint8_t type) {
    if (type & VARIANT8_PTR) {
        type |= VARIANT8_PTR_OWNER;
    }
    unpack(v)->type = type;
}

uint8_t variant8_get_type(variant8_t v) { return unpack(&v)->type & (~VARIANT8_PTR_OWNER); }

// returns variant8_t usr8
uint8_t variant8_get_usr8(variant8_t v) { return unpack(&v)->usr8; }

// returns variant8_t usr16
uint16_t variant8_get_usr16(variant8_t v) { return unpack(&v)->usr16; }

// returns variant8_t flt
float variant8_get_flt(variant8_t v) { return unpack(&v)->flt; }

// returns variant8_t pch
char *variant8_get_pch(variant8_t v) {
    return (unpack(&v)->type & (~VARIANT8_PTR_OWNER)) == VARIANT8_PCHAR ? unpack(&v)->pch : NULL;
}

// returns variant8_t ui8 from the array
uint8_t variant8_get_uia(variant8_t v, uint8_t index) { return index < 4 ? unpack(&v)->ui8a[index] : UINT8_MAX; }

// returns variant8_t ui32
uint32_t variant8_get_ui32(variant8_t v) { return unpack(&v)->ui32; }

// returns variant8_t i32
int32_t variant8_get_i32(variant8_t v) { return unpack(&v)->i32; }

// returns variant8_t ui16
uint16_t variant8_get_ui16(variant8_t v) { return unpack(&v)->ui16; }

// returns variant8_t i16
int16_t variant8_get_i16(variant8_t v) { return unpack(&v)->i16; }

// returns variant8_t ui8
uint8_t variant8_get_ui8(variant8_t v) { return unpack(&v)->ui8; }

// returns variant8_t i8
int8_t variant8_get_i8(variant8_t v) { return unpack(&v)->i8; }

// returns variant8_t bool
bool variant8_get_bool(variant8_t v) { return (bool)variant8_get_i8(v); }

#ifdef CLEAN_UNUSED
variant8_t variant8_pui8(uint8_t *pui8, uint16_t count, int init) {
    if (init) {
        return variant8_init(VARIANT8_PUI8, count, (void *)pui8);
    }
    return _VARIANT8_TYPE(VARIANT8_PUI8, 0, .size = (uint16_t)(count * sizeof(uint8_t)), .pui8 = pui8);
}

variant8_t variant8_pui16(uint16_t *pui16, uint16_t count, int init) {
    if (init) {
        return variant8_init(VARIANT8_PUI16, count, (void *)pui16);
    }
    return _VARIANT8_TYPE(VARIANT8_PUI16, 0, .size = (uint16_t)(count * sizeof(uint16_t)), .pui16 = pui16);
}

variant8_t variant8_pui32(uint32_t *pui32, uint16_t count, int init) {
    if (init) {
        return variant8_init(VARIANT8_PUI32, count, (void *)pui32);
    }
    return _VARIANT8_TYPE(VARIANT8_PUI32, 0, .size = (uint16_t)(count * sizeof(uint32_t)), .pui32 = pui32);
}

variant8_t variant8_pflt(float *pflt, uint16_t count, int init) {
    if (init) {
        return variant8_init(VARIANT8_PFLT, count, (void *)pflt);
    }
    return _VARIANT8_TYPE(VARIANT8_PFLT, 0, .size = (uint16_t)(count * sizeof(float)), .pflt = pflt);
}
#endif
uint16_t variant8_type_size(uint8_t type) {
    switch (type) {
    case VARIANT8_I8:
    case VARIANT8_UI8:
        return sizeof(uint8_t);
    case VARIANT8_CHAR:
        return sizeof(char);
    case VARIANT8_I16:
    case VARIANT8_UI16:
        return sizeof(uint16_t);
    case VARIANT8_I32:
    case VARIANT8_UI32:
        return sizeof(uint32_t);
    case VARIANT8_FLT:
        return sizeof(float);
    case VARIANT8_USER:
        return (sizeof(variant8_t) - sizeof(uint8_t));
    }
    return 0;
}

uint16_t variant8_data_size(variant8_t *pvar8) {
    if (pvar8) {
        _variant8_t v = *unpack(pvar8);
        if (v.type & VARIANT8_PTR) {
            return v.size;
        } else {
            return variant8_type_size(v.type);
        }
    }
    return 0;
}

void *variant8_data_ptr(variant8_t *pvar8) {
    if (pvar8) {
        _variant8_t *v = unpack(pvar8);
        if (v->type & VARIANT8_PTR) {
            return v->ptr;
        } else {
            switch (v->type) {
            case VARIANT8_I8:
                return &(v->i8);
            case VARIANT8_UI8:
                return &(v->ui8);
            case VARIANT8_CHAR:
                return &(v->ch);
            case VARIANT8_I16:
                return &(v->i16);
            case VARIANT8_UI16:
                return &(v->ui16);
            case VARIANT8_I32:
                return &(v->i32);
            case VARIANT8_UI32:
                return &(v->ui32);
            case VARIANT8_FLT:
                return &(v->flt);
            case VARIANT8_USER:
                return &(v->usr8);
            }
        }
    }
    return 0;
}

#ifdef CLEAN_UNUSED
const char *variant8_typename(uint8_t type) {
    static const char *_typename[] = { " EMPTY", "PI8", "PUI8", "PI16", "PUI16", "PI32", "PUI32", "PFLT", "PCHAR" };
    if (type <= VARIANT8_CHAR) {
        return _typename[type] + 1;
    }
    if ((type >= VARIANT8_PI8) && (type <= VARIANT8_PCHAR)) {
        return _typename[type & ~VARIANT8_PTR];
    }
    switch (type) {
    case VARIANT8_USER:
        return "USER";
    case VARIANT8_ERROR:
        return "ERROR";
    }
    return "???";
}
#endif
int variant8_snprintf(char *str, unsigned int size, const char *fmt, variant8_t *pvar8) {
    int n = 0;
    _variant8_t *pv = unpack(pvar8);
    switch (pv->type) {
    case VARIANT8_EMPTY:
        if (size) {
            *str = 0;
        }
        break;
    case VARIANT8_I8:
        n = snprintf(str, size, fmt ? fmt : "%i", (int)pv->i8);
        break;
    case VARIANT8_UI8:
        n = snprintf(str, size, fmt ? fmt : "%u", (unsigned int)pv->ui8);
        break;
    case VARIANT8_I16:
        n = snprintf(str, size, fmt ? fmt : "%i", (int)pv->i16);
        break;
    case VARIANT8_UI16:
        n = snprintf(str, size, fmt ? fmt : "%u", (unsigned int)pv->ui16);
        break;
    case VARIANT8_I32:
        n = snprintf(str, size, fmt ? fmt : "%i", (int)pv->i32);
        break;
    case VARIANT8_UI32:
        n = snprintf(str, size, fmt ? fmt : "%u", (unsigned int)pv->ui32);
        break;
    case VARIANT8_FLT:
        n = snprintf(str, size, fmt ? fmt : "%f", (double)pv->flt);
        break;
    case VARIANT8_CHAR:
        n = snprintf(str, size, fmt ? fmt : "%c", pv->ch);
        break;
    case VARIANT8_USER:
        n = snprintf(str, size, fmt ? fmt : "%u %u %u", (unsigned int)pv->usr32, (unsigned int)pv->usr16, (unsigned int)pv->usr8);
        break;
    case VARIANT8_PCHAR:
        n = snprintf(str, size, fmt ? fmt : "%s", pv->pch);
        break;
    }
    return n;
}

#ifdef CLEAN_UNUSED
enum {
    VARIANT8_TO_STR_MAX_BUFF = 32
};

char *
variant8_to_str(variant8_t *pvar8, const char *fmt) {
    char buff[VARIANT8_TO_STR_MAX_BUFF];

    /// Output buffer size, including the terminating \0
    int out_buff_size = variant8_snprintf(buff, sizeof(buff), fmt, pvar8) + 1;
    if (out_buff_size <= 1) {
        return nullptr;
    }

    char *str = (char *)variant8_malloc(out_buff_size);
    if (!str) {
        return nullptr;
    }

    // If the string fit inside buff, we can copy it and we don't need to call snprintf again
    if (n < sizeof(buff)) {
        memcpy(str, buff, out_buff_size);
    } else {
        variant8_snprintf(str, out_buff_size, fmt, pvar8);
    }

    return str;
}
#endif

variant8_t variant8_from_str(uint8_t type, char *str) {
    switch (type) {
    case VARIANT8_EMPTY:
        return variant8_empty();
    case VARIANT8_I8:
    case VARIANT8_I16:
    case VARIANT8_I32: {
        int i;
        if (sscanf(str, "%i", &i)) {
            switch (type) {
            case VARIANT8_I8:
                if ((i >= INT8_MIN) && (i <= INT8_MAX)) {
                    return variant8_i8(i);
                } else {
                    return variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
                }
            case VARIANT8_I16:
                if ((i >= INT16_MIN) && (i <= INT16_MAX)) {
                    return variant8_i16(i);
                } else {
                    return variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
                }
            case VARIANT8_I32:
                return variant8_i32(i);
            }
        }
        break;
    }
    case VARIANT8_UI8:
    case VARIANT8_UI16:
    case VARIANT8_UI32: {
        unsigned ui;
        if (sscanf(str, "%u", &ui)) {
            switch (type) {
            case VARIANT8_UI8:
                if (ui <= UINT8_MAX) {
                    return variant8_ui8(ui);
                } else {
                    return variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
                }
                break;
            case VARIANT8_UI16:
                if (ui <= UINT16_MAX) {
                    return variant8_ui16(ui);
                } else {
                    return variant8_error(VARIANT8_ERR_OOFRNG, 0, 0);
                }
            case VARIANT8_UI32:
                return variant8_ui32(ui);
            }
        }
    }
        [[fallthrough]];
    case VARIANT8_FLT: {
        float f;
        if (sscanf(str, "%f", &f)) {
            return variant8_flt(f);
        }
        [[fallthrough]];
    }
    case VARIANT8_CHAR: {
        char c;
        if (sscanf(str, "%c", &(c))) {
            _variant8_t v = _VARIANT8_TYPE(VARIANT8_CHAR, 0, 0, .ch = c);
            return *pack(&v);
        }
        [[fallthrough]];
    }
    case VARIANT8_USER: {
        uint32_t usr32;
        uint16_t usr16;
        uint16_t usr8; // wider than necessary due to missing SCNu8/%hhu on newlib-nano
        int n = sscanf(str, "%" SCNu32 " %" SCNu16 " %" SCNu16, &usr32, &usr16, &usr8);
        if (n == 3) {
            return variant8_user(usr32, usr16, usr8);
        }
        [[fallthrough]];
    }
    case VARIANT8_PCHAR:
        return variant8_pchar(str, 0, 1);
    }
    return variant8_error(VARIANT8_ERR_UNSCON, 0, 0);
}

#ifdef VARIANT8_DBG_MALLOC
uint32_t variant8_total_malloc_size = 0;
#endif // VARIANT8_DBG_MALLOC

static void *variant8_malloc(uint16_t size) {
    void *ptr = malloc(size);
#ifdef VARIANT8_DBG_MALLOC
    variant8_total_malloc_size += malloc_usable_size(ptr);
#endif // VARIANT8_DBG_MALLOC
    return ptr;
}

static void variant8_free(void *ptr) {
    if (ptr) {
#ifdef VARIANT8_DBG_MALLOC
        variant8_total_malloc_size -= malloc_usable_size(ptr);
#endif // VARIANT8_DBG_MALLOC
        free(ptr);
    }
}

#ifdef CLEAN_UNUSED
void *variant8_realloc(void *ptr, uint16_t size) {
    #ifdef VARIANT8_DBG_MALLOC
    uint32_t old_size = 0;
    uint32_t new_size = 0;
    if (ptr) {
        old_size = malloc_usable_size(ptr);
    }
    #endif // VARIANT8_DBG_MALLOC
    ptr = realloc(ptr, size);
    #ifdef VARIANT8_DBG_MALLOC
    if (ptr) {
        new_size = malloc_usable_size(ptr);
    }
    variant8_total_malloc_size += (new_size - old_size);
    #endif // VARIANT8_DBG_MALLOC
    return ptr;
}
#endif
} // extern "C"

// supported conversions for variant8_change_type
// from              to               param     description
// VARIANT8_EMPTY -> VARIANT8_PCHAR   fmt
// VARIANT8_I8    -> VARIANT8_PCHAR   fmt
// VARIANT8_UI8   -> VARIANT8_PCHAR   fmt
// VARIANT8_I16   -> VARIANT8_PCHAR   fmt
// VARIANT8_UI16  -> VARIANT8_PCHAR   fmt
// VARIANT8_I32   -> VARIANT8_PCHAR   fmt
// VARIANT8_UI32  -> VARIANT8_PCHAR   fmt
// VARIANT8_FLT   -> VARIANT8_PCHAR   fmt
// VARIANT8_CHAR  -> VARIANT8_PCHAR   fmt
// VARIANT8_USER  -> VARIANT8_PCHAR   fmt
//
// VARIANT8_PCHAR -> VARIANT8_EMPTY   fmt
// VARIANT8_PCHAR -> VARIANT8_I8      fmt
// VARIANT8_PCHAR -> VARIANT8_UI8     fmt
// VARIANT8_PCHAR -> VARIANT8_I16     fmt
// VARIANT8_PCHAR -> VARIANT8_UI16    fmt
// VARIANT8_PCHAR -> VARIANT8_I32     fmt
// VARIANT8_PCHAR -> VARIANT8_UI32    fmt
// VARIANT8_PCHAR -> VARIANT8_FLT     fmt
// VARIANT8_PCHAR -> VARIANT8_CHAR    fmt
// VARIANT8_PCHAR -> VARIANT8_USER    fmt
//
#ifdef CLEAN_UNUSED
class cvariant8 {
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

    cvariant8(uint64_t val);

    ~cvariant8();

public: // public functions
    cvariant8 copy();
    uint64_t pack() const;

    // cvariant8 &attach(variant8_t var8);
    // variant8_t detach();
    #ifdef CLEAN_UNUSED
    cvariant8 &change_type(uint8_t new_type);
    #endif

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
    int32_t get_valid_int() const; // helper for extractors, works ony on integer values
public: // extractors
    // clang-format off
    operator int8_t()       const { return (is_integer())           ?   int8_t(get_valid_int()) : ((data_.type == VARIANT8_FLT) ?   int8_t(data_.flt)           : 0); }
    operator uint8_t()      const { return (is_integer())           ?  uint8_t(get_valid_int()) : ((data_.type == VARIANT8_FLT) ?  uint8_t(data_.flt)           : 0); }
    operator int16_t()      const { return (is_integer())           ?  int16_t(get_valid_int()) : ((data_.type == VARIANT8_FLT) ?  int16_t(data_.flt)           : 0); }
    operator uint16_t()     const { return (is_integer())           ? uint16_t(get_valid_int()) : ((data_.type == VARIANT8_FLT) ? uint16_t(data_.flt)           : 0); }
    operator int32_t()      const { return (is_integer())           ?  int32_t(get_valid_int()) : ((data_.type == VARIANT8_FLT) ?  int32_t(data_.flt)           : 0); }
    operator uint32_t()     const { return (is_integer())           ? uint32_t(get_valid_int()) : ((data_.type == VARIANT8_FLT) ? uint32_t(data_.flt)           : 0); }
    operator float()        const { return (data_.type == VARIANT8_FLT)   ? data_.flt                       : ((is_integer())         ? float(get_valid_int())  : 0); }
    operator const char *() const { return (data_.type == VARIANT8_PCHAR) ? data_.pch : 0; }
    // clang-format on
private:
    enum class operator_x { plus,
        minus,
        multiplies,
        divides }; // float has no modulus
    // T should be int types or float - better pass by value
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
        default: // avoid warning
            return lhs + rhs;
        }
    }
    // used by assignment operators like +=
    cvariant8 &assignment_operator_x(const cvariant8 &rhs, operator_x op);

public: // arithmetic operators
    cvariant8 &operator+=(const cvariant8 &rhs) {
        return assignment_operator_x(rhs, operator_x::plus);
    }
    cvariant8 &operator-=(const cvariant8 &rhs) {
        return assignment_operator_x(rhs, operator_x::minus);
    }
    cvariant8 &operator*=(const cvariant8 &rhs) {
        return assignment_operator_x(rhs, operator_x::multiplies);
    }
    cvariant8 &operator/=(const cvariant8 &rhs) {
        return assignment_operator_x(rhs, operator_x::divides);
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

private:
    variant8_t data_;
};
#endif

#ifdef CLEAN_UNUSED

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
    #ifdef CLEAN_UNUSED
    cvariant8 &change_type(uint8_t new_type);
    #endif

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
    int32_t get_valid_int() const; // helper for extractors, works ony on integer values
public: // extractors
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
        divides }; // float has no modulus
    // T should be int types or float - better pass by value
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
        default: // avoid warning
            return lhs + rhs;
        }
    }
    // used by assignment operators like +=
    cvariant8 &assignment_operator_x(const cvariant8 &rhs, operator_x op);

public: // arithmetic operators
    cvariant8 &operator+=(const cvariant8 &rhs) {
        return assignment_operator_x(rhs, operator_x::plus);
    }
    cvariant8 &operator-=(const cvariant8 &rhs) {
        return assignment_operator_x(rhs, operator_x::minus);
    }
    cvariant8 &operator*=(const cvariant8 &rhs) {
        return assignment_operator_x(rhs, operator_x::multiplies);
    }
    cvariant8 &operator/=(const cvariant8 &rhs) {
        return assignment_operator_x(rhs, operator_x::divides);
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

cvariant8::cvariant8() {
    data_ = variant8_empty();
}

cvariant8::cvariant8(const cvariant8 &var8) {
    data_ = variant8_copy(&var8.data_);
}

    #if __cplusplus >= 201103L
cvariant8::cvariant8(cvariant8 &&var8) {
    data_ = var8.data_;
    *((&var8.data_)) = variant8_empty();
}
    #endif

cvariant8::cvariant8(int8_t val) {
    data_ = variant8_i8(val);
}

cvariant8::cvariant8(uint8_t val) {
    data_ = variant8_ui8(val);
}

cvariant8::cvariant8(int16_t val) {
    data_ = variant8_i16(val);
}

cvariant8::cvariant8(uint16_t val) {
    data_ = variant8_ui16(val);
}

cvariant8::cvariant8(int32_t val) {
    data_ = variant8_i32(val);
}

cvariant8::cvariant8(uint32_t val) {
    data_ = variant8_ui32(val);
}

cvariant8::cvariant8(float val) {
    data_ = variant8_flt(val);
}

cvariant8::cvariant8(const char *val) {
    data_ = variant8_pchar((char *)val, 0, 1);
}
cvariant8::~cvariant8() {
    variant8_done(&data_);
}

cvariant8 cvariant8::copy() {
    cvariant8 var8 = *this;
    return var8;
}

packed_variant8_t cvariant8::pack() const {
    packed_variant8_t r;
    memcpy(&r, &data_, sizeof(packed_variant8_t));
    return r;
}

std::uint16_t cvariant8::size() const {
    return variant8_type_size(data_.type);
}

// cvariant8 &cvariant8::attach(variant8_t var8) {
//     variant8_done(this);
//     *((variant8_t *)this) = var8;
//     return *this;
// }

// variant8_t cvariant8::detach() {
//     variant8_t var8 = *this;
//     variant8_done(this);
//     return var8;
// }
cvariant8 &cvariant8::change_type(uint8_t new_type) {
    variant8_change_type(this, new_type);
    return *this;
}
bool cvariant8::is_empty() const { return (data_.type == VARIANT8_EMPTY) ? true : false; }

bool cvariant8::is_error() const { return (data_.type == VARIANT8_ERROR) ? true : false; }

bool cvariant8::is_signed() const { return variant8_is_signed(&data_) ? true : false; }

bool cvariant8::is_unsigned() const { return variant8_is_unsigned(&data_) ? true : false; }

bool cvariant8::is_integer() const { return variant8_is_integer(&data_) ? true : false; }

bool cvariant8::is_number() const { return variant8_is_number(&data_) ? true : false; }

cvariant8 &cvariant8::operator=(const cvariant8 &var8) {
    variant8_done(&data_);
    data_ = variant8_copy(&var8.data_);
    return *this;
}

    #if __cplusplus >= 201103L
cvariant8 &cvariant8::operator=(cvariant8 &&var8) {
    variant8_done(&data_);
    data_ = variant8_copy(&var8.data_);
    *((&var8.data_)) = variant8_empty();
    return *this;
}
    #endif

cvariant8 &cvariant8::operator=(int8_t val) {
    variant8_done(&data_);
    data_ = variant8_i8(val);
    return *this;
}

cvariant8 &cvariant8::operator=(uint8_t val) {
    variant8_done(&data_);
    data_ = variant8_ui8(val);
    return *this;
}

cvariant8 &cvariant8::operator=(int16_t val) {
    variant8_done(&data_);
    data_ = variant8_i16(val);
    return *this;
}

cvariant8 &cvariant8::operator=(uint16_t val) {
    variant8_done(&data_);
    data_ = variant8_ui16(val);
    return *this;
}

cvariant8 &cvariant8::operator=(int32_t val) {
    variant8_done(&data_);
    data_ = variant8_i32(val);
    return *this;
}

cvariant8 &cvariant8::operator=(uint32_t val) {
    variant8_done(&data_);
    data_ = variant8_ui32(val);
    return *this;
}

cvariant8 &cvariant8::operator=(float val) {
    variant8_done(&data_);
    data_ = variant8_flt(val);
    return *this;
}

cvariant8 &cvariant8::operator=(const char *val) {
    variant8_done(&data_);
    data_ = variant8_pchar((char *)val, 0, 1);
    return *this;
}

// helper for extractors
// works ony on integer values
int32_t cvariant8::get_valid_int() const {
    switch (data_.type) {
    case VARIANT8_I8:
        return data_.i8;
    case VARIANT8_UI8:
        return data_.ui8;
    case VARIANT8_I16:
        return data_.i16;
    case VARIANT8_UI16:
        return data_.ui16;
    case VARIANT8_I32:
        return data_.i32;
    case VARIANT8_UI32:
        return data_.ui32;
    default: // wrong type
        return 0;
    }
}

// used by assignment operators like +=
cvariant8 &cvariant8::assignment_operator_x(const cvariant8 &rhs, cvariant8::operator_x op) {
    if (data_.type == rhs.data_.type) {
        switch (data_.type) {
        case VARIANT8_I8:
            data_.i8 = calc(data_.i8, rhs.data_.i8, op);
            break;
        case VARIANT8_UI8:
            data_.ui8 = calc(data_.ui8, rhs.data_.ui8, op);
            break;
        case VARIANT8_I16:
            data_.i16 = calc(data_.i16, rhs.data_.i16, op);
            break;
        case VARIANT8_UI16:
            data_.ui16 = calc(data_.ui16, rhs.data_.ui16, op);
            break;
        case VARIANT8_I32:
            data_.i32 = calc(data_.i32, rhs.data_.i32, op);
            break;
        case VARIANT8_UI32:
            data_.ui32 = calc(data_.ui32, rhs.data_.ui32, op);
            break;
        case VARIANT8_FLT:
            data_.flt = calc(data_.flt, rhs.data_.flt, op);
            break;
        case VARIANT8_EMPTY: // empty is fine
            break;
        default:
            // unsupported types set error
            data_.type = VARIANT8_ERROR;
            break;
        }
    } else {
        data_.type = VARIANT8_ERROR;
    }
    return *this;
}
#endif
