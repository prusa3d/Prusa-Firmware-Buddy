// guitypes.hpp
#pragma once

#include "guitypes.h"
#include "resource.h"
#include "general_response.hpp"
#include <optional>

namespace png {
/**
 * @brief this struct handles PNG resources
 * all pngs share single file and have offset
 * this shred file os opened at first acces and never closed
 *
 * If this struct is used for some temporary file, close of that file must be called manually (we need trivial destructor)
 * There is a child structure ResourceSingleFile to handle it automatically
 */
struct Resource {
    FILE *file = nullptr;       // default file
    const char *name = nullptr; // name is optional, external file might not need it
    size_t offset = 0;          // 0 == no offset, all "normal" png files containing only one png has no offset
    size_t size = 0;            // 0 == calculate at run time
    uint16_t w = 0;             // 0 == calculate at run time
    uint16_t h = 0;             // 0 == calculate at run time

    /**
     * @brief ctor for multiple pngs contained in single file
     */
    constexpr Resource(const char *name, size_t offset, size_t size, uint16_t w, uint16_t h)
        : file(nullptr)
        , name(name)
        , offset(offset)
        , size(size)
        , w(w)
        , h(h) {}

    Resource(const char *name);

    FILE *Get() const;

    static void EnableDefaultFile() { enabled = true; }

private:
    static bool enabled; // wait after bootstrap !!!
};

/**
 * @brief Child of Resource with non trivial destructor
 * designed to open file in ctor and close it in dtor
 */
struct ResourceSingleFile : public Resource {
    ResourceSingleFile(const char *name);
    ~ResourceSingleFile();
};

} // namespace png

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

    point_t operator+(const point_t &rhs) const {
        return {
            .x = static_cast<T>(x + rhs.x),
            .y = static_cast<T>(y + rhs.y)
        };
    }
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
    Response response;
    int i_val;
    struct header_t {
        layout_color layout;
    } header;
    GUIStartupProgress *pGUIStartupProgress;
};

static_assert(sizeof(event_conversion_union::point) <= sizeof(event_conversion_union::pvoid), "event_conversion_union is broken");

struct size_ui16_t {
    uint16_t w;
    uint16_t h;
    constexpr bool operator==(const size_ui16_t &rhs) const {
        return (w == rhs.w) && (h == rhs.h);
    }
    constexpr bool operator!=(const size_ui16_t &rhs) const { return !((*this) == rhs); }
};

template <class T>
struct padding_t {
    T left;
    T top;
    T right;
    T bottom;

    constexpr bool operator==(const padding_t &rhs) const {
        return (left == rhs.left) && (top == rhs.top) && (right == rhs.right) && (bottom == rhs.bottom);
    }
    constexpr bool operator!=(const padding_t &rhs) const { return !((*this) == rhs); }
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

const uint8_t *resource_ptr(ResourceId id);

uint16_t resource_size(ResourceId id);

FILE *resource_fopen(ResourceId id, const char *opentype);

font_t *resource_font(ResourceId id);
