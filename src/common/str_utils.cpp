#include "str_utils.hpp"

#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <string_view_utf8.hpp>
#include <bsod/bsod.h>

RectTextLayout::RectTextLayout(StringReaderUtf8 &reader, uint16_t max_cols, uint16_t max_rows, is_multiline multiline) {
    if (max_cols == 0 || max_rows == 0) {
        overflow = (reader.getUtf8Char() != 0);
        return;
    }

    if (multiline == is_multiline::no) {
        max_rows = 1;
    }

    std::optional<int> split_index = std::nullopt;
    unichar c = 0;
    int chars_this_line = 0;

    while ((c = reader.getUtf8Char()) != 0) {
        switch (c) {
        case '\n': // new line

            set_current_line_characters(chars_this_line);
            skip_char[current_line] = true;
            if (!new_line(max_rows)) {
                return;
            }

            chars_this_line = 0;
            split_index = std::nullopt;
            break;

        case 0x3002: // Japanese dot
        case 0x3001: // Japanese comma
        case ' ': // remember space position

            // erasing start space is not handled here
            // Whenever we enter new line (except the first one), we always skip 1 char ('\n' || ' ') from the stream
            // If there are more whitespace characters, its clearly a choice

            split_index = chars_this_line + 1;
            skip_char[current_line] = (c == ' ');
            [[fallthrough]];

        default:
            chars_this_line++;

            if (chars_this_line > max_cols) {
                if (!split_index || current_line == max_rows - 1) { // Do not wrap singleline texts
                    split_index = max_cols;
                    overflow = true;
                    skip_char[current_line] = false;
                }

                // It does not count newline char and space before wrapped word
                // Wrapping cuts overflown word and put it on the next line
                // If word is too long for a line, it will split the word on max_cols

                // count chars in next line
                chars_this_line -= *split_index;
                set_current_line_characters(*split_index - (skip_char[current_line] ? 1 : 0));
                split_index = std::nullopt;

                if (!new_line(max_rows)) {
                    return;
                }
            }
        }
    }

    skip_char[current_line] = false;
    set_current_line_characters(chars_this_line);
    return;
}

void RectTextLayout::set_current_line_characters(uint8_t char_cnt) {
    data[current_line] = char_cnt;
    longest_char_cnt = std::max(longest_char_cnt, char_cnt);
}

uint8_t RectTextLayout::get_current_line_characters() const {
    return get_line_characters(current_line);
}

uint8_t RectTextLayout::get_line_count() const {
    return current_line == 0 && get_current_line_characters() == 0 ? 0 : current_line + 1;
}

bool RectTextLayout::new_line(uint8_t max_rows) {
    if (current_line >= MaxLines || current_line + 1 >= max_rows) {
        overflow = true;
        return false;
    }
    current_line++;
    return true;
}

template <typename T, auto strtox_func>
from_chars_light_result from_chars_light_common(const char *first, const char *last, T &value, int base) {
    std::array<char, sizeof(value) * 8 + 2> buffer; // buffer where we'll copy the number with ending zero, size reserved is to fit base 2 number + sign, plus ending zero.

    size_t len = std::distance(first, last);
    if (len > buffer.size() - 1) {
        // buffer is too small, or won't fit ending zero
        return { first, std::errc::value_too_large };
    }
    std::copy(first, last, buffer.begin());
    buffer[len] = '\0'; // make sure there is ending zero, to avoid strtoi reading beyond the buffer
    char *out_end_buf = nullptr;
    errno = 0;
    auto value_res = strtox_func(buffer.data(), &out_end_buf, base);

    const char *out_end = first + (out_end_buf - buffer.begin());
    if (errno != 0) {
        return { out_end, std::errc::result_out_of_range };
    }
    if (out_end == first) {
        return { out_end, std::errc::invalid_argument };
    }
    if (value_res < std::numeric_limits<T>::min() || value_res > std::numeric_limits<T>::max()) {
        return { out_end, std::errc::result_out_of_range };
    }
    value = value_res;

    return { out_end, std::errc {} };
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, unsigned long long &value, int base) {
    static_assert(sizeof(unsigned long long) >= sizeof(value));
    return from_chars_light_common<unsigned long long, std::strtoull>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, unsigned long &value, int base) {
    static_assert(sizeof(unsigned long) >= sizeof(value));
    return from_chars_light_common<unsigned long, std::strtoul>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, unsigned int &value, int base) {
    static_assert(sizeof(unsigned long) >= sizeof(value));
    return from_chars_light_common<unsigned int, std::strtoul>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, unsigned short &value, int base) {
    static_assert(sizeof(unsigned long) >= sizeof(value));
    return from_chars_light_common<unsigned short, std::strtoul>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, unsigned char &value, int base) {
    static_assert(sizeof(unsigned long) >= sizeof(value));
    return from_chars_light_common<unsigned char, std::strtoul>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, long long &value, int base) {
    static_assert(sizeof(long long) >= sizeof(value));
    return from_chars_light_common<long long, std::strtoll>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, long &value, int base) {
    static_assert(sizeof(long) >= sizeof(value));
    return from_chars_light_common<long, std::strtol>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, int &value, int base) {
    static_assert(sizeof(long) >= sizeof(value));
    return from_chars_light_common<int, std::strtol>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, short &value, int base) {
    static_assert(sizeof(long) >= sizeof(value));
    return from_chars_light_common<short, std::strtol>(first, last, value, base);
}

template <>
from_chars_light_result from_chars_light(const char *first, const char *last, signed char &value, int base) {
    static_assert(sizeof(long) >= sizeof(value));
    return from_chars_light_common<signed char, std::strtol>(first, last, value, base);
}

from_chars_light_result from_chars_light(const char *first, const char *last, float &value) {
    std::array<char, 32> buffer; // buffer where we'll copy the number with ending zero

    size_t len = std::distance(first, last);
    if (len >= buffer.size() - 1) {
        return { first, std::errc::value_too_large };
    }
    std::copy(first, last, buffer.begin());
    buffer[len] = '\0'; // make sure there is ending zero, to avoid strtoi reading beyond the buffer
    char *out_end_buf = nullptr;
    value = std::strtof(buffer.data(), &out_end_buf);

    const char *out_end = first + (out_end_buf - buffer.begin());

    if (out_end == first) {
        return { first, std::errc::invalid_argument };
    }
    return { out_end, std::errc {} };
}

uint16_t RectTextLayout::compute_line_count(const string_view_utf8 &text, uint16_t max_cols) {
    StringReaderUtf8 reader { text };
    const RectTextLayout layout { reader, max_cols, MaxLines, is_multiline::yes };
    debug_assert(!layout.has_text_overflown());
    return layout.get_line_count();
}
