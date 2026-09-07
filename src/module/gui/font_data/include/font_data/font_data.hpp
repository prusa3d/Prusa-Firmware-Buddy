/// @file
#pragma once

#include <cstdint>

namespace font_data {

/// Character set and bitmap of a font.
///
/// Opaque - how a font is laid out is decided by this library, nothing outside can look
/// inside. A font just points to the data it was built with.
struct FontData;

struct FontSize {
    uint8_t w; ///< Character width [pixels]
    uint8_t h; ///< Character height [pixels]

    constexpr bool operator==(const FontSize &) const = default;
};

/// Monospace bitmap font, 4 bits per pixel, characters stored in the order of the
/// character set
///
/// The size is part of the declaration, so it is available in constant expressions even
/// though the data is not.
struct Font {
    uint8_t w; ///< Character width [pixels]
    uint8_t h; ///< Character height [pixels]
    const FontData *data;

    constexpr FontSize size() const { return { w, h }; }

    /// Bitmap of the character, 4 bits per pixel, `w * h` pixels, first row first.
    ///
    /// Characters the font does not contain are replaced by '?' - this really happens,
    /// for example with non-utf8 characters on filesystems. A font containing no '?'
    /// either falls back to the first character of its bitmap.
    const uint8_t *character_bitmap(uint32_t character) const;

    /// Whether the font draws the character itself, rather than falling back to '?'
    bool contains(uint32_t character) const;
};

/// The fonts, grouped by the character set they were built for - see README.md
extern const FontData regular_9x16_full_data;
inline constexpr Font regular_9x16_full { 9, 16, &regular_9x16_full_data };
extern const FontData bold_11x19_full_data;
inline constexpr Font bold_11x19_full { 11, 19, &bold_11x19_full_data };
extern const FontData bold_13x22_full_data;
inline constexpr Font bold_13x22_full { 13, 22, &bold_13x22_full_data };

extern const FontData bold_30x53_digits_data;
inline constexpr Font bold_30x53_digits { 30, 53, &bold_30x53_digits_data };

extern const FontData regular_7x13_latin_and_accents_data;
inline constexpr Font regular_7x13_latin_and_accents { 7, 13, &regular_7x13_latin_and_accents_data };
extern const FontData regular_9x16_latin_and_accents_data;
inline constexpr Font regular_9x16_latin_and_accents { 9, 16, &regular_9x16_latin_and_accents_data };
extern const FontData regular_11x18_latin_and_accents_data;
inline constexpr Font regular_11x18_latin_and_accents { 11, 18, &regular_11x18_latin_and_accents_data };

extern const FontData regular_7x13_latin_and_katakana_data;
inline constexpr Font regular_7x13_latin_and_katakana { 7, 13, &regular_7x13_latin_and_katakana_data };
extern const FontData regular_9x16_latin_and_katakana_data;
inline constexpr Font regular_9x16_latin_and_katakana { 9, 16, &regular_9x16_latin_and_katakana_data };
extern const FontData regular_11x18_latin_and_katakana_data;
inline constexpr Font regular_11x18_latin_and_katakana { 11, 18, &regular_11x18_latin_and_katakana_data };

extern const FontData regular_7x13_latin_and_cyrillic_data;
inline constexpr Font regular_7x13_latin_and_cyrillic { 7, 13, &regular_7x13_latin_and_cyrillic_data };
extern const FontData regular_9x16_latin_and_cyrillic_data;
inline constexpr Font regular_9x16_latin_and_cyrillic { 9, 16, &regular_9x16_latin_and_cyrillic_data };
extern const FontData regular_11x18_latin_and_cyrillic_data;
inline constexpr Font regular_11x18_latin_and_cyrillic { 11, 18, &regular_11x18_latin_and_cyrillic_data };

} // namespace font_data
