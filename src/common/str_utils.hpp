#pragma once

#include <string>
#include <array>
#include <cstdint>

static const constexpr char CHAR_SPACE = ' ';
static const constexpr char CHAR_NBSP = '\xA0'; /// Non Breaking Space
#define NBSP "\xA0"                             /// Non Breaking Space
static const constexpr char CHAR_NL = '\n';     /// New Line
#define NL "\n"                                 /// New Line
static const constexpr char EOS = '\0';         /// End Of String

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
    const uint32_t w = 12;
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
/// @details For testing purpose only
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

////////////////////////////////////////////////////////////////////////////////
///
/// caching word buffer
///
using word_buffer = std::array<uint32_t, 32>;

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

    size_t size() const {
        return p_word_buffer_->size();
    };

private:
    word_buffer *p_word_buffer_;
};

////////////////////////////////////////////////////////////////////////////////
///
/// Stream reader without breaking the lines
///
struct no_wrap {
    using value_type = uint32_t;

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

    text_wrapper(uint32_t width, font_type font)
        : width_(width)     ///< width of the space for the text in pixels
        , index_(-1)        ///< current position in buffer where single word is
        , current_width_(0) ///< width used already
        , word_length_(0)   ///< number characters of current word + trailing white character
        , font_(font) {};

    template <typename source>
    value_type character(source &s) {
        if (index_ < 0) {
            const uint32_t w = buffering(s); ///< current word's width in pixels
            index_ = 0;
            if ((w + current_width_) > width_) {
                /// this word will not fit to this line => break the line
                current_width_ = w;
                return static_cast<value_type>(CHAR_NL);
            }
            current_width_ = (w + current_width_ == width_) ? 0 : current_width_ + w;
        }

        const value_type c = buffer_[index_];
        buffer_[index_] = 0;
        if (index_ < static_cast<int32_t>(word_length_)) {
            index_++;
            return c;
        }
        index_ = -1;
        if (current_width_ == 0) {
            return c == static_cast<value_type>(EOS)
                ? c
                : static_cast<value_type>(CHAR_NL);
        }
        if (c == static_cast<value_type>(CHAR_SPACE)) {
            current_width_ += width::value(font_);
        } else if (c == static_cast<value_type>(CHAR_NL))
            current_width_ -= current_width_;
        return c;
    }

private:
    /// Copies current word to the buffer
    /// \returns word's width in pixels
    template <typename source>
    uint32_t buffering(source &s) {
        uint32_t i = 0;
        uint32_t word_width = 0;
        value_type c = 0;

        //read max size() characters continuosly
        //in our case word_buffer = std::array<uint32_t, 32>;
        while (i < buffer_.size()) {
            c = s.getUtf8Char();
            buffer_[i] = c;
            if (c == static_cast<value_type>(CHAR_NL)
                || c == static_cast<value_type>(CHAR_SPACE)
                || c == static_cast<value_type>(EOS))
                break;
            word_width += width::value(font_);
            if (c == static_cast<value_type>(CHAR_NBSP)) {
                buffer_[i] = static_cast<value_type>(CHAR_SPACE);
            }
            ++i;
        }
        word_length_ = i;
        return word_width;
    }

    memory_buffer buffer_;
    uint32_t width_;
    int32_t index_;
    uint32_t current_width_;
    uint32_t word_length_;
    font_type font_;
};
