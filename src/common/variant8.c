// variant8.c

#include "variant8.h"

variant8_t variant8_(uint8_t type) {
    variant8_t var = { type, 0, 0, { 0 } };
    return var;
}

variant8_t variant8_empty(void) { return variant8_(VARIANT8_EMPTY); }

variant8_t variant8_i8(int8_t i8) {
    variant8_t var = variant8_(VARIANT8_I8);
    var.i8 = i8;
    return var;
}

variant8_t variant8_ui8(uint8_t ui8) {
    variant8_t var = variant8_(VARIANT8_UI8);
    var.ui8 = ui8;
    return var;
}

variant8_t variant8_i16(int16_t i16) {
    variant8_t var = variant8_(VARIANT8_I16);
    var.i16 = i16;
    return var;
}

variant8_t variant8_ui16(uint16_t ui16) {
    variant8_t var = variant8_(VARIANT8_UI16);
    var.ui16 = ui16;
    return var;
}

variant8_t variant8_i32(int32_t i32) {
    variant8_t var = variant8_(VARIANT8_I32);
    var.i32 = i32;
    return var;
}

variant8_t variant8_ui32(uint32_t ui32) {
    variant8_t var = variant8_(VARIANT8_UI32);
    var.ui32 = ui32;
    return var;
}

variant8_t variant8_flt(float flt) {
    variant8_t var = variant8_(VARIANT8_FLT);
    var.flt = flt;
    return var;
}

variant8_t variant8_user(uint32_t usr32) {
    variant8_t var = variant8_(VARIANT8_USER);
    var.usr32 = usr32;
    return var;
}
