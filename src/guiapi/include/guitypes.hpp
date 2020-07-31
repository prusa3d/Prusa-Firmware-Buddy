// guitypes.hpp
#pragma once

#include "guitypes.h"

struct padding_ui8_t {
    uint8_t left;
    uint8_t top;
    uint8_t right;
    uint8_t bottom;
};

rect_ui16_t rect_ui16_add_padding_ui8(rect_ui16_t rc, padding_ui8_t pad);

rect_ui16_t rect_ui16_sub_padding_ui8(rect_ui16_t rc, padding_ui8_t pad);
