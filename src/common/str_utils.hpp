#pragma once

#include <string>
#include <array>
#include <cstdint>

constexpr char CHAR_SPACE = ' ';
constexpr char CHAR_NBSP = '\xA0'; /// Non Breaking Space
#define NBSP "\xA0"                /// Non Breaking Space
constexpr char CHAR_NL = '\n';     /// New Line
#define NL "\n"                    /// New Line
constexpr char EOS = '\0';         /// End Of String

enum str_err {
    nullptr_err = -1,
    small_buffer = -2
};
size_t strdel(char *str, const size_t n = 1);
int strins(char *str, size_t max_size, const char *const ins, size_t times = 1);
int strshift(char *str, size_t max_size, const size_t n = 1, const char default_char = ' ');
int str2multiline(char *str, size_t max_size, const size_t line_width);

int strshiftUnicode(uint32_t *str, size_t max_size, const size_t n = 1, const uint32_t default_char = ' ');
int strinsUnicode(uint32_t *str, size_t max_size, const uint32_t *const ins, size_t times = 1);
int str2multilineUnicode(uint32_t *str, size_t max_size, const size_t line_width);

////////////////////////////////////////////////////////////////////////////////
///
/// Emulate font with the constant character width
///
struct monospace {
    const std::uint16_t w = 12;
};

////////////////////////////////////////////////////////////////////////////////
///
/// Determine the width of the character given by a specific font
///
struct width {
    template <class U>
    static constexpr size_t value(U const &c) { return c->w; };
};

////////////////////////////////////////////////////////////////////////////////
///
/// Memory storage
///
/// @details For testing purpose
///
struct memory_source {
    using value_type = char;

    memory_source(std::string const &s)
        : index_(0) {
        buffer_.fill(0);
        std::copy(s.begin(), s.end(), buffer_.data());
    }

    value_type getUtf8Char() const {
        value_type c;
        if (index_ >= buffer_.size())
            return EOS;
        c = buffer_[index_++];
        if (c == EOS)
            index_ = buffer_.size();
        return c;
    }

    value_type peek() const { return buffer_[index_]; }

private:
    std::array<value_type, 512> buffer_;
    mutable size_t index_;
};

using word_buffer = std::array<std::uint32_t, 32>;

////////////////////////////////////////////////////////////////////////////////
///
/// Global ram word storage
///
/// @details Max length of the word is 31 characters in UTF-8 encoding
///
struct ram_buffer {
    using value_type = word_buffer::value_type;

    ram_buffer();

    value_type &operator[](size_t index) {
        return (*p_word_buffer_)[index];
    };

private:
    word_buffer *p_word_buffer_;
};

////////////////////////////////////////////////////////////////////////////////
///
/// Stream reader without breaking the lines
///
struct no_wrap {
    using value_type = std::uint32_t;

    template <typename source>
    value_type character(source &s) {
        return s.getUtf8Char();
    }
};

////////////////////////////////////////////////////////////////////////////////
///
/// Template class which modify input stream of the UTF-8 characters and
/// appending the new lines (\n) in case that the word cannot be drawn into the
/// specific rectangle
///
/// @details Based on memory policy and font type modify the input stream such a way
/// that storing the word to the buffer and determine if the word can be fit into the
/// specific rectangle. In case that the word potentially overflow the rectangle it
/// breaks the line by return the \n character followed by the word. Number of characters
/// in the line is calculated as a sum of character's width already in the line.
///
/// @tparam memory_buffer Buffer storage policy
/// @tparam font_type Font
///
template <
    typename memory_buffer,
    typename font_type>
struct text_wrapper {
    using value_type = typename memory_buffer::value_type;

    text_wrapper(std::uint16_t width, font_type font)
        : width_(width)
        , index_(-1)
        , current_width_(0)
        , word_length_(0)
        , font_(font) {};

    template <typename source>
    value_type character(source &s) {
        if (index_ < 0) {
            std::uint16_t w = buffering(s);
            if ((w + current_width_) > width_) {
                current_width_ = (w + width::value(font_));
                return static_cast<value_type>(CHAR_NL);
            } else if ((w + current_width_) == width_) {
                current_width_ = 0;
            } else {
                current_width_ += (w + width::value(font_));
            }
        }

        if (index_ < static_cast<std::int8_t>(word_length_)) {
            value_type c = buffer_[index_];
            buffer_[index_] ^= buffer_[index_];
            ++index_;
            return c;
        } else {
            index_ = -1;
            return current_width_ == 0
                ? static_cast<value_type>(CHAR_NL)
                : static_cast<value_type>(CHAR_SPACE);
        }
    }

private:
    template <typename source>
    std::uint16_t buffering(source &s) {
        std::uint8_t i = 0;
        std::uint16_t word_width = 0;
        value_type c = 0;
        while ((c = s.getUtf8Char()) != static_cast<value_type>(CHAR_SPACE)) {
            word_width += width::value(font_);
            if (c == static_cast<value_type>(CHAR_NBSP)) {
                buffer_[i++] = static_cast<value_type>(CHAR_SPACE);
            } else if (c == static_cast<value_type>(CHAR_NL)) {
                buffer_[i++] = c;
                current_width_ = 0;
                break;
            } else if (c == static_cast<value_type>(EOS)) {
                buffer_[i++] = c;
                word_width -= width::value(font_);
                break;
            } else {
                buffer_[i++] = c;
            }
        }
        index_ = 0;
        word_length_ = i;
        return word_width;
    }

    memory_buffer buffer_;
    std::uint16_t width_;
    std::int8_t index_;
    std::uint16_t current_width_;
    std::uint8_t word_length_;
    font_type font_;
};
