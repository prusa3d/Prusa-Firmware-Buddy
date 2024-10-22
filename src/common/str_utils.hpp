#pragma once

#include <charconv>
#include <string>
#include <string.h>
#include <array>
#include <cstdint>
#include <algorithm>
#include <assert.h>
#include "../lang/string_view_utf8.hpp"

inline constexpr char CHAR_SPACE = ' ';
inline constexpr char CHAR_NBSP = '\xA0'; /// Non Breaking Space
#define NBSP "\xA0" /// Non Breaking Space
inline constexpr char CHAR_NL = '\n'; /// New Line
#define NL "\n" /// New Line
inline constexpr char EOS = '\0'; /// End Of String

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

/// A const char* that is guaranteed to have unlimited lifetime (thanks to the consteval constructor)
struct ConstexprString {
    consteval ConstexprString() = default;
    consteval ConstexprString(const ConstexprString &) = default;
    consteval ConstexprString(const char *str)
        : str_(str) {}

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

    consteval inline operator const char *() const {
        return str;
    }
    consteval inline operator ConstexprString() const {
        return ConstexprString(str);
    }
};

template <typename T, T... chars>
constexpr TemplateString<chars...> operator""_tstr() { return {}; }

////////////////////////////////////////////////////////////////////////////////
///
/// test c-string equality - for multiple tests against same string
///
class CStrEqual {
    const char *buff_;
    size_t sz_;

public:
    CStrEqual(const char *buff, size_t sz)
        : buff_(buff)
        , sz_(sz) {}
    inline bool operator()(const char *str, size_t sz = std::string::npos) {
        return !strncmp(buff_, str, std::min(sz_, sz));
    }
};

////////////////////////////////////////////////////////////////////////////////
///
/// Emulate font with the constant character width == 12
///
struct monospace {
    static constexpr uint32_t w = 12;
};

////////////////////////////////////////////////////////////////////////////////
///
/// Emulate font with the constant character width == 1
///
struct font_emulation_w1 {
    static constexpr size_t w = 1;
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
        if (index_ >= buffer_.size()) {
            return EOS;
        }
        c = buffer_[index_++];
        if (c == EOS) {
            index_ = buffer_.size();
        }
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
/// numbers of characters in lines
class RectTextLayout {
public:
    static constexpr size_t MaxLines = 31;
    static constexpr uint8_t MaxCharInLine = 255; // uint8_t
    using Data_t = std::array<uint8_t, MaxLines + 1>; // last elem stores current line
private:
    Data_t data;
    uint8_t currentLine() const { return data[MaxLines]; }

public:
    RectTextLayout() {
        data.fill(0);
    }

    uint8_t LineCharacters(uint8_t line) const {
        return data[line];
    }

    uint8_t CurrentLineCharacters() const {
        return LineCharacters(currentLine());
    }

    uint8_t GetLineCount() const {
        return CurrentLineCharacters() == 0 ? currentLine() : currentLine() + 1;
    }

    // increment number of lines
    bool NewLine() {
        if (currentLine() >= MaxLines) {
            return false;
        }
        data[MaxLines] += 1;
        return true;
    }

    bool IncrementNumOfCharsUpTo(uint8_t max_val) {
        if (currentLine() >= MaxLines) {
            return false;
        }
        if (CurrentLineCharacters() >= max_val) {
            return false;
        }
        data[currentLine()] += 1;
        return true;
    }
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

    text_wrapper(uint32_t wrap_width, font_type font)
        : wrap_width_(wrap_width)
        , font_(font) {};

    /// Yields next character of the string
    template <typename source>
    value_type character(source &s) {
        // Nothing in the buffer -> load next word
        if (buffer_pos_ >= buffer_count_) {
            if (buffer_next_word(s)) {
                return static_cast<value_type>(CHAR_NL);
            }
        }

        const value_type c = buffer_[buffer_pos_++];

        // We're not at the end of the word -> simply return the character
        if (buffer_pos_ < buffer_count_) {
            return c;
        }

        // Mark that we should read next word in the next character call
        buffer_count_ = 0;

        if (c == static_cast<value_type>(CHAR_SPACE)) {
            // Increase current width by the space size
            // (because the word_width is not increased trailing space in buffer_next_word)
            current_line_width_ += width::value(font_);

            // Should the space be trailing at the end of the line, omit it and go straight to newline
            if (buffer_next_word(s)) {
                return static_cast<value_type>(CHAR_NL);
            }

        } else if (c == static_cast<value_type>(CHAR_NL)) {
            current_line_width_ = 0;
        }

        return c;
    }

private:
    /// Loads next word (potentially including a trailing whitespace) into the buffer.
    /// Updates buffer_count_, current_line_width_. Resets buffer_pos_.
    /// If the loaded word wouldn't fit on the current line, resets current_line_width_ and returns TRUE
    /// to indicate a new line should be started before yielding the current buffer contents.
    template <typename source>
    bool buffer_next_word(source &s) {
        uint32_t i = 0;
        uint32_t buffer_width = 0;
        value_type c = 0;

        const bool was_long_word_mode = long_word_mode_;

        // read max size() characters continuosly
        // in our case word_buffer = std::array<uint32_t, 32>;
        while (true) {
            if (i >= buffer_.size()) {
                long_word_mode_ = true;
                break;
            }

            c = s.getUtf8Char();
            buffer_[i++] = c;

            if (c == static_cast<value_type>(CHAR_NL)
                || c == static_cast<value_type>(CHAR_SPACE)
                || c == static_cast<value_type>(EOS)) {
                long_word_mode_ = false;
                break;
            }

            buffer_width += width::value(font_);

            if (c == static_cast<value_type>(CHAR_NBSP)) {
                buffer_[i - 1] = static_cast<value_type>(CHAR_SPACE);
            }
        }

        buffer_count_ = i;
        buffer_pos_ = 0;

        // If the current loaded word doesn't fit to the current line (and is not the first thing in the line) -> break the line
        if ((buffer_width + current_line_width_) > wrap_width_ && current_line_width_ != 0 && !was_long_word_mode) {
            current_line_width_ = buffer_width;
            return true;
        } else {
            current_line_width_ += buffer_width;
            return false;
        }
    }

private:
    const uint32_t wrap_width_; ///< width of the space for the text in pixels
    const font_type font_;

private:
    memory_buffer buffer_;
    uint32_t buffer_pos_ = 0; ///< current position in buffer where single word is
    uint32_t buffer_count_ = 0; ///< number of characters in the buffer (current word + trailing white character, if the word is shorter than the buffer)

private:
    uint32_t current_line_width_ = 0; ///< pixels already used in the current line
    bool long_word_mode_ = false; ///< Set to true for consecutive buffer loads if the word is longer than buffer size
};

/// Class that allows safely building strings in char buffers.
/// The class ensures that the string is always validly null terminated. Last character in the buffer is always reserved for \0.
/// Use this class instead of snprintf, because it does all the checks.
/// The class does NOT take the OWNERSHIP in the buffer in any way, the buffer has to exist for the whole existence of StringBuilder (but see ArrayStringBuilder)
/// The builder always considers terminating \0, so the actual available size for string is buffer_size - 1
class StringBuilder {

public:
    StringBuilder() = default;

    StringBuilder(std::span<char> span) {
        init(span.data(), span.size());
    }

    /// See StringBuilder::init
    static StringBuilder from_ptr(char *buffer, size_t buffer_size) {
        StringBuilder result;
        result.init(buffer, buffer_size);
        return result;
    }

public:
    /// Initializes string builder on the buffer.
    void init(char *buffer, size_t buffer_size);

public:
    /// Returns false if there is no space left to write further text or if any of the called functions failed
    inline bool is_ok() const {
        return is_ok_;
    }

    /// See is_ok
    inline bool is_problem() const {
        return !is_ok_;
    }

    /// Returns number of characters in the buffer (NOT counting the terminating \0 which is always there)
    inline size_t char_count() const {
        return current_pos_ - buffer_start_;
    }

    /// Returns number of bytes used of the buffer (COUNTING the terminating \0 which is always there)
    inline size_t byte_count() const {
        return char_count() + 1;
    }

    /// Returns pointer to the first character in the builder buffer
    inline const char *begin() const {
        return buffer_start_;
    }

    /// Returns pointer behind the last character in the builder buffer (should always point to \0)
    inline const char *end() const {
        return current_pos_;
    }

public:
    StringBuilder &append_char(char ch);

    StringBuilder &append_string(const char *str);

    StringBuilder &append_std_string_view(const std::string_view &view);

    StringBuilder &append_string_view(const string_view_utf8 &str);

    /// Appends text to the builder, using vsnprintf under the hood.
    StringBuilder &__attribute__((format(__printf__, 2, 3)))
    append_printf(const char *fmt, ...);

    /// Appends text to the builder, using vsnprintf under the hood.
    StringBuilder &append_vprintf(const char *fmt, va_list args);

    struct AppendFloatConfig {
        /// Maximum decimal places to print
        uint8_t max_decimal_places = 3;

        /// Always use all max_decimal_places
        bool all_decimal_places : 1 = false;

        /// 0.xxx -> .xxx
        bool skip_zero_before_dot : 1 = false;
    };

    /// Appends a float value
    StringBuilder &append_float(double val, const AppendFloatConfig &config);

public:
    /// Allocates $cnt chars at the end of the string and returns the pointer to them.
    /// Appends \0 at the end of the allocation.
    /// Returns pointer to the allocated string or nullptr on failure
    char *alloc_chars(size_t cnt);

private:
    /// For safety reasons, string builder copying is only for static constructors
    StringBuilder(const StringBuilder &o) = default;

private:
    /// Pointer to start of the buffer
    char *buffer_start_ = nullptr;

    /// Pointer after end of the buffer (1 char after the terminating null character)
    char *buffer_end_ = nullptr;

    /// Pointer to the current writable position on the buffer (should always contain terminating null char)
    char *current_pos_ = nullptr;

    /// Error flag, can be set by for example when printf fails
    bool is_ok_ = true;
};

/// StringBuilder bundled together with an array
template <size_t array_size_>
class ArrayStringBuilder : public StringBuilder {

public:
    inline ArrayStringBuilder()
        : StringBuilder(array) {}

public:
    inline const char *str() const {
        assert(is_ok());
        return array.data();
    }
    inline const char *str_nocheck() const {
        return array.data();
    }

    inline const uint8_t *str_bytes() const {
        assert(is_ok());
        return reinterpret_cast<const uint8_t *>(array.data());
    }

private:
    std::array<char, array_size_> array;
};
