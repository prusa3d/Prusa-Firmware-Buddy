// guitypes.hpp
#pragma once

#include "marlin_server_types/general_response.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>

#include <utils/color.hpp>

// Menu item corners - background rounded corners
enum {
    MIC_TOP_RIGHT = 1 << 0,
    MIC_TOP_LEFT = 1 << 1,
    MIC_BOT_RIGHT = 1 << 2,
    MIC_BOT_LEFT = 1 << 3,
    MIC_ALL_CORNERS = (MIC_TOP_RIGHT | MIC_TOP_LEFT | MIC_BOT_RIGHT | MIC_BOT_LEFT),
    MIC_ALT_CL_TOP_RIGHT = 1 << 4,
    MIC_ALT_CL_TOP_LEFT = 1 << 5,
    MIC_ALT_CL_BOT_RIGHT = 1 << 6,
    MIC_ALT_CL_BOT_LEFT = 1 << 7,
};

namespace img {

struct Resource {
    FILE *file; ///< Open file handle, nullptr to use default get_resource_file()
    size_t offset; ///< Offset in file [byte]
    size_t size; ///< Size of resource (informative) [byte]
    uint16_t w; ///< Width (informative) [pixel]
    uint16_t h; ///< Height (informative) [pixel]
    constexpr Resource(size_t offset, size_t size, uint16_t w, uint16_t h)
        : file(nullptr)
        , offset(offset)
        , size(size)
        , w(w)
        , h(h) {}
    constexpr Resource(FILE *file)
        : file(file)
        , offset(0)
        , size(0)
        , w(0)
        , h(0) {}
};

/**
 * @brief Child of Resource with non trivial destructor
 * designed to open file in ctor and close it in dtor
 */
struct ResourceSingleFile : public Resource {
    ResourceSingleFile(const char *name);
    ~ResourceSingleFile();
};

} // namespace img

enum class EFooter { Off,
    On };

template <class T>
struct point_t {
    T x;
    T y;

    template <typename U>
    static point_t from_point(const point_t<U> &o) {
        return point_t {
            .x = static_cast<T>(o.x),
            .y = static_cast<T>(o.y),
        };
    }

    constexpr bool operator==(const point_t &rhs) const {
        return (x == rhs.x) && (y == rhs.y);
    }
    constexpr bool operator!=(const point_t &rhs) const { return !((*this) == rhs); }

    point_t operator+(const point_t &rhs) const {
        return {
            .x = static_cast<T>(x + rhs.x),
            .y = static_cast<T>(y + rhs.y)
        };
    }
    point_t operator-(const point_t &rhs) const {
        return {
            .x = static_cast<T>(x - rhs.x),
            .y = static_cast<T>(y - rhs.y)
        };
    }
};

using point_i16_t = point_t<int16_t>;
using point_ui16_t = point_t<uint16_t>;

enum class layout_color : uint8_t { leave_it,
    black,
    red,
    blue,
};

struct GUIStartupProgress {
    unsigned percent_done;
    std::optional<const char *> bootstrap_description;
};

union event_conversion_union {
    void *pvoid;
    point_ui16_t point;
    Response response;
    int i_val;
    struct header_t {
        layout_color layout;
    } header;
    GUIStartupProgress *pGUIStartupProgress;
};

static_assert(sizeof(event_conversion_union::point) <= sizeof(event_conversion_union::pvoid), "event_conversion_union is broken");

struct size_ui16_t {
    uint16_t w = 0;
    uint16_t h = 0;

    constexpr bool operator==(const size_ui16_t &rhs) const {
        return (w == rhs.w) && (h == rhs.h);
    }
    constexpr bool operator!=(const size_ui16_t &rhs) const { return !((*this) == rhs); }
};

template <class T>
struct padding_t {
    T left = 0;
    T top = 0;
    T right = 0;
    T bottom = 0;

    constexpr bool operator==(const padding_t &rhs) const {
        return (left == rhs.left) && (top == rhs.top) && (right == rhs.right) && (bottom == rhs.bottom);
    }
    constexpr bool operator!=(const padding_t &rhs) const { return !((*this) == rhs); }
};

using padding_ui8_t = padding_t<uint8_t>;
using padding_ui16_t = padding_t<uint16_t>;

struct bitmap_t {
    uint16_t w; // bitmap width [pixels]
    uint16_t h; // bitmap height [pixels]
    uint8_t bpp; // bits per pixel
    uint8_t bpr; // bytes per row
    void *ppx; // pixel data pointer
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
