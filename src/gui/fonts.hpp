/**
 * @file fonts.hpp
 */

#pragma once
#include <stdint.h>
#include <guiconfig/guiconfig.h>
#include "font_character_sets.hpp"

enum class Font : uint8_t {
    small = 0,
    normal,
    big,
    special,
#ifdef USE_ILI9488
    large,
#endif
};

struct font_t {
    uint8_t w; // char width [pixels]
    uint8_t h; // char height [pixels]
    uint8_t bpr; // bytes per row
    const void *pcs; // charset data pointer
    char asc_min; // min ascii code (first character)
    FontCharacterSet charset; // character set (see README_FONT)
};

font_t *resource_font(Font id);

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
consteval font_size_t resource_font_size(Font id) {
    switch (id) {
#ifdef USE_ST7789
    case Font::small:
        return { 7, 13 };
    case Font::normal:
    case Font::big: // Big font removed to save flash
        return { 11, 18 };
    case Font::special:
        return { 9, 16 };
#endif /*USE_ST7789*/

#ifdef USE_ILI9488
    case Font::small:
        return { 9, 16 };
    case Font::normal:
        return { 11, 19 };
    case Font::big:
        return { 13, 22 };
    case Font::special:
        return { 9, 16 };
    case Font::large:
        return { 30, 53 };
#endif /*USE_ILI9488*/

    default:
        return { 0, 0 };
    }
}

inline consteval auto width(Font font) {
    return resource_font_size(font).w;
}

inline consteval auto height(Font font) {
    return resource_font_size(font).h;
}
