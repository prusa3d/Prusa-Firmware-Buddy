/**
 * @file fonts.hpp
 */

#pragma once
#include <stdint.h>
#include <printers.h>
#include <font_data/font_data.hpp>
#include <option/enable_translation_ja.h>
#include <option/enable_translation_uk.h>

enum class Font : uint8_t {
    small = 0,
    normal,
    big,
    special,
#if not PRINTER_IS_PRUSA_MINI()
    large,
#endif

    largest_available =
#if not PRINTER_IS_PRUSA_MINI()
        large,
#else
        big,
#endif
};

using font_t = font_data::Font;
using font_size_t = font_data::FontSize;

#if PRINTER_IS_PRUSA_MINI()
    #if ENABLE_TRANSLATION_JA()
inline constexpr const font_t &font_regular_7x13 = font_data::regular_7x13_latin_and_katakana;
inline constexpr const font_t &font_regular_9x16 = font_data::regular_9x16_latin_and_katakana;
inline constexpr const font_t &font_regular_11x18 = font_data::regular_11x18_latin_and_katakana;
    #elif ENABLE_TRANSLATION_UK()
inline constexpr const font_t &font_regular_7x13 = font_data::regular_7x13_latin_and_cyrillic;
inline constexpr const font_t &font_regular_9x16 = font_data::regular_9x16_latin_and_cyrillic;
inline constexpr const font_t &font_regular_11x18 = font_data::regular_11x18_latin_and_cyrillic;
    #else
inline constexpr const font_t &font_regular_7x13 = font_data::regular_7x13_latin_and_accents;
inline constexpr const font_t &font_regular_9x16 = font_data::regular_9x16_latin_and_accents;
inline constexpr const font_t &font_regular_11x18 = font_data::regular_11x18_latin_and_accents;
    #endif
#else
inline constexpr const font_t &font_regular_9x16 = font_data::regular_9x16_full;
inline constexpr const font_t &font_bold_11x19 = font_data::bold_11x19_full;
inline constexpr const font_t &font_bold_13x22 = font_data::bold_13x22_full;
inline constexpr const font_t &font_bold_30x53 = font_data::bold_30x53_digits;
#endif

const font_t *resource_font(Font id);

/**
 * @brief Get font size in pixels.
 * This can be used in constant expresisons.
 */
consteval font_size_t resource_font_size(Font id) {
    switch (id) {
#if PRINTER_IS_PRUSA_MINI()
    case Font::small:
        return font_regular_7x13.size();
    case Font::normal:
    case Font::big: // Big font removed to save flash
        return font_regular_11x18.size();
    case Font::special:
        return font_regular_9x16.size();
#endif

#if not PRINTER_IS_PRUSA_MINI()
    case Font::small:
        return font_regular_9x16.size();
    case Font::normal:
        return font_bold_11x19.size();
    case Font::big:
        return font_bold_13x22.size();
    case Font::special:
        return font_regular_9x16.size();
    case Font::large:
        return font_bold_30x53.size();
#endif

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
