// guitypes.hpp
#pragma once

#include "guitypes.h"

typedef uint32_t color_t;

struct point_i16_t {
    int16_t x;
    int16_t y;
};

struct point_ui16_t {
    uint16_t x;
    uint16_t y;
};

struct size_ui16_t {
    uint16_t w;
    uint16_t h;
};

struct rect_ui16_t {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct padding_ui8_t {
    uint8_t left;
    uint8_t top;
    uint8_t right;
    uint8_t bottom;
};

struct bitmap_t {
    uint16_t w;  //bitmap width [pixels]
    uint16_t h;  //bitmap height [pixels]
    uint8_t bpp; //bits per pixel
    uint8_t bpr; //bytes per row
    void *ppx;   //pixel data pointer
};

inline point_ui16_t point_ui16(uint16_t x, uint16_t y) {
    point_ui16_t point = { x, y };
    return point;
}

inline size_ui16_t size_ui16(uint16_t w, uint16_t h) {
    size_ui16_t size = { w, h };
    return size;
}

inline rect_ui16_t rect_ui16(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    rect_ui16_t rect = { x, y, w, h };
    return rect;
}

inline padding_ui8_t padding_ui8(uint8_t l, uint8_t t, uint8_t r, uint8_t b) {
    padding_ui8_t padding = { l, t, r, b };
    return padding;
}

inline int point_in_rect_ui16(point_ui16_t pt, rect_ui16_t rc) {
    return ((pt.x >= rc.x) && (pt.x < (rc.x + rc.w)) && (pt.y >= rc.y) && (pt.y < (rc.y + rc.h))) ? 1 : 0;
}

inline int rect_in_rect_ui16(rect_ui16_t rc, rect_ui16_t rc1) {
    return ((rc.x >= rc1.x) && ((rc.x + rc.w) <= (rc1.x + rc1.w)) && (rc.y >= rc1.y) && ((rc.y + rc.h) <= (rc1.y + rc1.h))) ? 1 : 0;
}

inline int rect_empty_ui16(rect_ui16_t rc) {
    return ((rc.w == 0) || (rc.h == 0)) ? 1 : 0;
}

rect_ui16_t rect_intersect_ui16(rect_ui16_t rc1, rect_ui16_t rc2);

rect_ui16_t rect_ui16_add_padding_ui8(rect_ui16_t rc, padding_ui8_t pad);

rect_ui16_t rect_ui16_sub_padding_ui8(rect_ui16_t rc, padding_ui8_t pad);

rect_ui16_t rect_align_ui16(rect_ui16_t rc, rect_ui16_t rc1, uint8_t align);

point_ui16_t icon_meas(const uint8_t *pi);
size_ui16_t icon_size(const uint8_t *pi);

const uint8_t *resource_ptr(uint16_t id);

uint16_t resource_size(uint16_t id);

FILE *resource_fopen(uint16_t id, const char *opentype);

font_t *resource_font(uint16_t id);
