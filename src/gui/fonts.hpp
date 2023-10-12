/**
 * @file fonts.hpp
 */

#pragma once
#include <stdint.h>
#include "guiconfig.h"

enum ResourceId : uint8_t {
    IDR_FNT_SMALL,
    IDR_FNT_NORMAL,
    IDR_FNT_BIG,
    IDR_FNT_SPECIAL,
#ifdef USE_ILI9488
    IDR_FNT_LARGE,
#endif
};

struct font_t {
    uint8_t w; // char width [pixels]
    uint8_t h; // char height [pixels]
    uint8_t bpr; // bytes per row
    uint32_t flg; // flags
    void *pcs; // charset data pointer
    char asc_min; // min ascii code (first character)
    char asc_max; // max ascii code (last character)
};

font_t *resource_font(ResourceId id);

/**
 * @brief Font size in pixels.
 */
struct font_size_t {
    uint8_t w; ///< Char width [pixels]
    uint8_t h; ///< Char height [pixels]

    constexpr bool operator==(const font_size_t &rhs) const {
        return w == rhs.w && h == rhs.h;
    }
};

/**
 * @brief Get font size in pixels.
 * This can be used in constant expresisons.
 * @todo Refactor fonts to have public info in '.hpp' and private data in '.cpp'.
 */
consteval font_size_t resource_font_size(ResourceId id) {
    switch (id) {
#ifdef USE_ST7789
    case IDR_FNT_SMALL:
        return { 7, 13 };
    case IDR_FNT_NORMAL:
    case IDR_FNT_BIG: // Big font removed to save flash
        return { 11, 18 };
    case IDR_FNT_SPECIAL:
        return { 9, 16 };
#endif /*USE_ST7789*/

#ifdef USE_ILI9488
    case IDR_FNT_SMALL:
        return { 9, 16 };
    case IDR_FNT_NORMAL:
        return { 11, 19 };
    case IDR_FNT_BIG:
        return { 13, 22 };
    case IDR_FNT_SPECIAL:
        return { 9, 16 };
    case IDR_FNT_LARGE:
        return { 30, 53 };
#endif /*USE_ILI9488*/

    default:
        return { 0, 0 };
    }
}
