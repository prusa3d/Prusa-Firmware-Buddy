#include "str_utils.hpp"

#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <cinttypes>

static word_buffer ram_word_buffer;

ram_buffer::ram_buffer() {
    p_word_buffer_ = &ram_word_buffer;
};

template <typename T>
size_t strlenT(const T *s) {
    size_t i;
    for (i = 0; s[i] != '\0'; i++)
        ;
    return i;
}

/// Deletes \param n characters from beginning of the \param str
/// \returns number of deleted characters
template <typename T>
size_t strdelT(T *str, const size_t n) {
    if (str == nullptr) {
        return 0;
    }

    size_t size = strlenT(str);
    if (n >= size) {
        str[0] = '\0';
        return size;
    }

    size = size - n + 1; // copy \0 as well
    for (size_t i = 0; i < size; ++i, ++str) {
        *str = *(str + n);
    }
    return n;
}

size_t strdel(char *str, const size_t n) {
    return strdelT(str, n);
}

size_t strdelUnicode(uint32_t *str, const size_t n) {
    return strdelT(str, n);
}

/// Shifts text in \param str by \param n characters
/// \param default_char is inserted if new undefined space appears
/// if \param default_char is 0 then nothing is inserted but the resulting
/// string could be shorter than expected
/// \returns number of characters shifted or negative number in case of error
template <typename T>
int strshiftT(T *str, size_t max_size, const size_t n, const T default_char) {
    if (str == nullptr) {
        return str_err::nullptr_err;
    }
    if (n == 0) {
        return 0;
    }

    const size_t size = strlenT(str);
    if (size + n >= max_size) { /// too much to add
        return str_err::small_buffer;
    }

    /// copy text, start from the last character including '\0'
    for (size_t i = size + n; i >= n; --i) {
        str[i] = str[i - n];
    }

    if (default_char == '\0') {
        return n;
    }

    /// fill the space between old and new text
    for (size_t i = size; i < n; ++i) {
        str[i] = default_char;
    }
    return n;
}

int strshift(char *str, size_t max_size, const size_t n, const char default_char) {
    return strshiftT(str, max_size, n, default_char);
}

int strshiftUnicode(uint32_t *str, size_t max_size, const size_t n, const uint32_t default_char) {
    return strshiftT(str, max_size, n, default_char);
}

/// Inserts \param ins at the beginning of \param str \param times times
/// \returns number of inserted characters or negative number in case of error
template <typename T>
int strinsT(T *str, size_t max_size, const T *const ins, size_t times) {
    if (str == nullptr || ins == nullptr) {
        return str_err::nullptr_err;
    }

    const size_t ins_size = strlenT(ins);
    const size_t inserted = ins_size * times;
    if (inserted <= 0) {
        return 0;
    }

    /// shift the end
    const int shifted = strshiftT(str, max_size, inserted, T(0));
    if (shifted <= 0) {
        return shifted;
    }

    /// insert text in the newly created space
    size_t i;
    for (size_t t = 0; t < times; ++t) {
        for (i = 0; i < ins_size; ++i, ++str) {
            *str = ins[i];
        }
    }

    return inserted;
}

int strins(char *str, size_t max_size, const char *const ins, size_t times) {
    return strinsT(str, max_size, ins, times);
}

int strinsUnicode(uint32_t *str, size_t max_size, const uint32_t *const ins, size_t times) {
    return strinsT(str, max_size, ins, times);
}

/// Replaces breakable spaces into line breaks in \param str
/// to ensure that no line is longer than \param line_width.
/// If \param line_width is too short,
/// the text will be broken in the middle of the word.
/// Existing line breaks are not removed.
/// \returns final number of lines or negative number in case of error
template <typename T>
int str2multilineT(T *str, size_t max_size, size_t line_width, const T *nl) {
    if (str == nullptr || line_width == 0) {
        return str_err::nullptr_err;
    }
    if (*str == EOS) {
        return 1;
    }

    int last_delimiter = -1;
    int last_NBSP = -1;
    size_t lines = 1;
    size_t current_length = 0;
    size_t i = 0;

    while (1) {
        /// analyze character
        switch (str[i]) {
        case (T)CHAR_SPACE:
            last_delimiter = i;
            ++current_length;
            break;
        case (T)CHAR_NBSP:
            str[i] = ' ';
            last_NBSP = i;
            // last_delimiter = i;
            ++current_length;
            break;
        case (T)CHAR_NL:
            ++lines;
            last_delimiter = -1;
            last_NBSP = -1;
            current_length = 0;
            break;
        default:
            ++current_length;
            break;
        }

        ++i;

        if (current_length > line_width) { /// if the length is too big, break the line
            if (last_delimiter >= 0) {
                /// break at space
                str[last_delimiter] = CHAR_NL;
                i = last_delimiter + 1;
            } else if (last_NBSP >= 0) {
                /// break at nonbreaking space - better than break a word
                str[last_NBSP] = CHAR_NL;
                i = last_NBSP + 1;
            } else {
                /// no break point available - break a word instead
                const int inserted = strinsT(str + i - 1, max_size - i + 1, nl, 1);
                if (inserted < 0) {
                    return str_err::small_buffer;
                }
            }
            ++lines;
            current_length = 0;
            last_delimiter = -1;
            last_NBSP = -1;
        }

        if (str[i] == EOS) {
            break;
        }
    }
    return lines;
}

int str2multiline(char *str, size_t max_size, size_t line_width) {
    return str2multilineT(str, max_size, line_width, NL);
}

int str2multilineUnicode(uint32_t *str, size_t max_size, size_t line_width) {
    static const uint32_t nl[2] = { 0xa, 0 };
    return str2multilineT(str, max_size, line_width, nl);
}

// StringBuilder
// ---------------------------------------------
void StringBuilder::init(char *buffer, size_t buffer_size) {
    buffer_start_ = buffer;
    current_pos_ = buffer;
    buffer_end_ = buffer + buffer_size;

    // Make the resulting string valid from the go
    *current_pos_ = '\0';
}

StringBuilder &StringBuilder::append_char(char ch) {
    if (char *ptr = alloc_chars(1)) {
        *ptr = ch;
    }

    return *this;
}

StringBuilder &StringBuilder::append_string(const char *str) {
    if (is_problem()) {
        return *this;
    }

    // Accomodate for terminating null
    char *buffer_pre_end = buffer_end_ - 1;
    char *buffer_pos = current_pos_;

    while (true) {
        // At the end of the appended string -> success
        if (*str == '\0') {
            current_pos_ = buffer_pos;
            break;
        }

        // Check if we're not at the end of the buffer
        if (buffer_pos >= buffer_pre_end) {
            is_ok_ = false;
            break;
        }

        *buffer_pos++ = *str++;
    }

    // Ensure the string is valid by appending nullterm
    *current_pos_ = '\0';
    return *this;
}

StringBuilder &StringBuilder::append_std_string_view(const std::string_view &view) {
    if (const auto buf = alloc_chars(view.size())) {
        view.copy(buf, view.size());
    }

    return *this;
}

StringBuilder &StringBuilder::append_string_view(const string_view_utf8 &str) {
    StringReaderUtf8 reader(str);

    while (true) {
        if (is_problem()) {
            return *this;
        }

        char b = reader.getbyte();
        if (b == '\0') {
            return *this;
        }

        append_char(b);
    }

    return *this;
}

StringBuilder &StringBuilder::append_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    append_vprintf(fmt, args);
    va_end(args);
    return *this;
}

StringBuilder &StringBuilder::append_vprintf(const char *fmt, va_list args) {
    if (is_problem()) {
        return *this;
    }

    const int available_bytes = int(buffer_end_ - current_pos_);
    const int ret = vsnprintf(current_pos_, available_bytes, fmt, args);

    // >= because we need to account fo rhte terminating \0
    if (ret < 0 || ret >= available_bytes) {
        *current_pos_ = '\0';
        is_ok_ = false;
        return *this;
    }

    current_pos_ += ret;
    return *this;
}

// This stupid utility function saves 3kB of FLASH...
// ...unless somebody else uses pow() somewhere, then it can be removed again.
static constexpr uint32_t dumb_pow_10(uint8_t exponent) {
    uint32_t result = 1;
    for (uint8_t i = 0; i < exponent; ++i) {
        result *= 10;
    }
    return result;
}
static_assert(dumb_pow_10(0) == 1);
static_assert(dumb_pow_10(1) == 10);
static_assert(dumb_pow_10(2) == 100);

StringBuilder &StringBuilder::append_float(double val, const AppendFloatConfig &config) {
    if (isnan(val)) {
        append_string("NaN");
        return *this;
    }

    const bool is_negative = val < 0;
    uint32_t precision_mult = dumb_pow_10(config.max_decimal_places);
    uint64_t accum = static_cast<uint64_t>(round(abs(val) * precision_mult));

    if (accum == 0) {
        append_char('0');
        return *this;
    }

    if (is_negative) {
        append_char('-');
    }

    // Print integral part
    const auto integral_part = accum / precision_mult;
    accum %= precision_mult;

    if (integral_part > 0 || is_negative || !config.skip_zero_before_dot) {
        append_printf("%" PRIu64, integral_part);
    }

    // Print decimal part
    for (int i = 0; accum > 0 || (config.all_decimal_places && i < config.max_decimal_places); i++) {
        if (i == 0) {
            // Decimal point
            append_char('.');
        }

        precision_mult /= 10;
        append_char('0' + (accum / precision_mult % 10));
        accum %= precision_mult;
    }

    return *this;
}

char *StringBuilder::alloc_chars(size_t cnt) {
    if (is_problem()) {
        return nullptr;
    }

    const size_t available_bytes = int(buffer_end_ - current_pos_);

    // >= because we need to account fo rhte terminating \0
    if (cnt >= available_bytes) {
        is_ok_ = false;
        return nullptr;
    }

    current_pos_ += cnt;
    *current_pos_ = '\0';
    return current_pos_ - cnt;
}
