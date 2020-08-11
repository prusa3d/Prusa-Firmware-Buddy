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

struct monospace {
    const std::uint16_t w = 12;
};

struct width {
    template <class U>
    static constexpr size_t value(U const &c) { return c->w; };
};

struct memory_source {
    using value_type = char;

    memory_source(std::string const &s)
        : index_(0) {
        buffer_.fill(0);
        std::copy(s.begin(), s.end(), buffer_.data());
    }

    value_type get() const {
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
        while ((c = s.get()) != static_cast<value_type>(CHAR_SPACE)) {
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
