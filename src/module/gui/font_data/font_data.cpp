/// @file
#include <font_data/font_data.hpp>

#include <algorithm>
#include <bsod/bsod.h>
#include <cstdint>
#include <span>

namespace font_data {

struct FontData {
    /// The characters the bitmap holds, sorted, in the order they are stored in
    std::span<const uint16_t> charset;
    const uint8_t *bitmap; ///< 4 bits per pixel, characters in the order of the charset
};

namespace {
    using CharacterIterator = std::span<const uint16_t>::iterator;
    CharacterIterator find_character(CharacterIterator first, CharacterIterator last, uint32_t character) {
        const auto i = std::lower_bound(first, last, character);
        return i != last && *i == character ? i : last;
    }

    uint32_t char_position(const FontData &data, uint32_t character) {
        const auto first = data.charset.begin();
        const auto last = data.charset.end();

        auto i = find_character(first, last, character);
        if (i == last) {
            i = find_character(first, last, '?');
        }
        if (i == last) {
            return 0;
        }

        return static_cast<uint32_t>(std::distance(first, i));
    }
} // namespace

// The character sets and the fonts, built by font.py out of the translations and the
// source pngs
#include "fonts.gen"

const uint8_t *Font::character_bitmap(uint32_t character) const {
    debug_assert(data);
    debug_assert(data->bitmap);

    const uint32_t bytes_per_character = (w * h + 1) >> 1;
    return data->bitmap + char_position(*data, character) * bytes_per_character;
}

bool Font::contains(uint32_t character) const {
    debug_assert(data);

    const auto first = data->charset.begin();
    const auto last = data->charset.end();
    return find_character(first, last, character) != last;
}

} // namespace font_data
