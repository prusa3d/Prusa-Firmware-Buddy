// guitypes.hpp
#pragma once

#include "guitypes.h"
#include <optional>

enum class EFooter { Off,
    On };

template <class T>
struct point_t {
    T x;
    T y;
    constexpr bool operator==(const point_t &rhs) const {
        return (x == rhs.x) && (y == rhs.y);
    }
    constexpr bool operator!=(const point_t &rhs) const { return !((*this) == rhs); }
};

using point_i16_t = point_t<int16_t>;
using point_ui16_t = point_t<uint16_t>;

enum class layout_color : uint8_t { leave_it,
    black,
    red };

struct GUIStartupProgress {
    unsigned percent_done;
    std::optional<const char *> bootstrap_description;
};

union event_conversion_union {
    void *pvoid;
    point_ui16_t point;
    struct header_t {
        layout_color layout;
    } header;
    GUIStartupProgress *pGUIStartupProgress;
};

static_assert(sizeof(event_conversion_union::point) <= sizeof(event_conversion_union::pvoid), "event_conversion_union is broken");

struct size_ui16_t {
    uint16_t w;
    uint16_t h;
    constexpr bool operator==(const size_ui16_t &rhs) {
        return (w == rhs.w) && (h == rhs.h);
    }
    constexpr bool operator!=(const size_ui16_t &rhs) { return !((*this) == rhs); }
};

template <class T>
struct padding_t {
    T left;
    T top;
    T right;
    T bottom;

    constexpr bool operator==(const padding_t &rhs) {
        return (left == rhs.left) && (top == rhs.top) && (right == rhs.right) && (bottom == rhs.bottom);
    }
    constexpr bool operator!=(const padding_t &rhs) { return !((*this) == rhs); }
};

using padding_ui8_t = padding_t<uint8_t>;
using padding_ui16_t = padding_t<uint16_t>;

struct bitmap_t {
    uint16_t w;  //bitmap width [pixels]
    uint16_t h;  //bitmap height [pixels]
    uint8_t bpp; //bits per pixel
    uint8_t bpr; //bytes per row
    void *ppx;   //pixel data pointer
};

inline point_i16_t point_i16(int16_t x, int16_t y) {
    point_i16_t point = { x, y };
    return point;
}

inline point_ui16_t point_ui16(uint16_t x, uint16_t y) {
    point_ui16_t point = { x, y };
    return point;
}

inline size_ui16_t size_ui16(uint16_t w, uint16_t h) {
    size_ui16_t size = { w, h };
    return size;
}

inline padding_ui8_t padding_ui8(uint8_t l, uint8_t t, uint8_t r, uint8_t b) {
    padding_ui8_t padding = { l, t, r, b };
    return padding;
}

point_ui16_t icon_meas(const uint8_t *pi);
size_ui16_t icon_size(const uint8_t *pi);

const uint8_t *resource_ptr(uint16_t id);

uint16_t resource_size(uint16_t id);

FILE *resource_fopen(uint16_t id, const char *opentype);

font_t *resource_font(uint16_t id);
