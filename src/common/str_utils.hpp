#pragma once

#include <charconv>
#include <string>
#include <string.h>
#include <array>
#include <cstdint>
#include <algorithm>
#include "../lang/string_view_utf8.hpp"
#include "../guiapi/include/font_flags.hpp"
#include <bitset>

/// Result of from_chars_light
struct from_chars_light_result {
    const char *ptr; // pointer to last character processed
    std::errc ec; // error code
};

/**
 * @brief from_chars_light functions are lightweight alternatives to std::from_char functions.
 * @note Problem with std::from_chars in GCC is that it uses fastfloat library, which takes about 20KB of flash space.
         from_chars_light functions aims to provide safe, but lightweight alternative. Note that this implementation surely has caveats and different behaviour
         than std implementation, but its is better than any other std functions, because it checks bounds of input array and also some overflows.
   @warning some overflows might not be detected, specifically for 64bit types, its limitation of strtoll function.
 */
template <typename T>
    requires std::integral<T>
from_chars_light_result from_chars_light(const char *first, const char *last, T &value, int base = 10); // from_chars for integers
from_chars_light_result from_chars_light(const char *first, const char *last, float &value); // from_chars for flaoat

/// A const char* that is guaranteed to have unlimited lifetime (thanks to the consteval constructor)
struct ConstexprString {
    constexpr ConstexprString() = default;
    constexpr ConstexprString(const ConstexprString &) = default;
    consteval ConstexprString(const char *str)
        : str_(str) {
        // Make sure the pointer contents is also accessible at compile time
        [[maybe_unused]] const auto ch = str ? str[0] : '\0';
    }

    /// Constructor where the contents of the pointer does not have to be constexpr
    /// !!! Unsafe, use only when you know what you're doing
    static consteval ConstexprString from_str_unsafe(const char *str) {
        ConstexprString r;
        r.str_ = str;
        return r;
    }

    constexpr operator const char *() const {
        return str_;
    }

private:
    const char *str_ = nullptr;
};

/// String that can be passed as a template parameter (use "XX"_tstr)
template <char... chars>
struct TemplateString {
    static constexpr inline const char str[] = { chars..., '\0' };

    constexpr inline operator const char *() const {
        return str;
    }
    constexpr inline operator ConstexprString() const {
        return ConstexprString(str);
    }
};

template <typename T, T... chars>
constexpr TemplateString<chars...> operator""_tstr() { return {}; }

////////////////////////////////////////////////////////////////////////////////
///
/// numbers of characters in lines
class RectTextLayout {
public:
    RectTextLayout(StringReaderUtf8 &reader, uint16_t max_cols, uint16_t max_rows, is_multiline multiline);

    uint8_t get_width_in_chars() const { return longest_char_cnt; }

    uint8_t get_height_in_chars() const { return get_line_count(); }

    bool has_text_overflown() const { return overflow; }

    uint8_t get_line_characters(uint8_t line) const { return data[line]; }

    uint8_t get_line_count() const;

    bool get_skip_char_on_line(uint8_t line) const { return skip_char[line]; }

private:
    static constexpr size_t MaxLines = 31;
    static constexpr uint8_t MaxCharInLine = 255; // uint8_t
    using Data_t = std::array<uint8_t, MaxLines>;
    std::bitset<MaxLines> skip_char = {};

    Data_t data = {};
    uint8_t current_line = 0;
    uint8_t longest_char_cnt = 0;
    bool overflow = false;

    void set_current_line_characters(uint8_t char_cnt);

    uint8_t get_current_line_characters() const;

    bool new_line(uint8_t max_rows);
};
