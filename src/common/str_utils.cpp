#include "str_utils.hpp"
#include <string.h>

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
    if (str == nullptr)
        return 0;

    size_t size = strlenT(str);
    if (n >= size) {
        str[0] = '\0';
        return size;
    }

    size = size - n + 1; // copy \0 as well
    for (size_t i = 0; i < size; ++i, ++str)
        *str = *(str + n);
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
    if (str == nullptr)
        return str_err::nullptr_err;
    if (n == 0)
        return 0;

    const size_t size = strlenT(str);
    if (size + n >= max_size) /// too much to add
        return str_err::small_buffer;

    /// copy text, start from the last character including '\0'
    for (size_t i = size + n; i >= n; --i) {
        str[i] = str[i - n];
    }

    if (default_char == '\0')
        return n;

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
    if (str == nullptr || ins == nullptr)
        return str_err::nullptr_err;

    const size_t ins_size = strlenT(ins);
    const size_t inserted = ins_size * times;
    if (inserted <= 0)
        return 0;

    /// shift the end
    const int shifted = strshiftT(str, max_size, inserted, T(0));
    if (shifted <= 0)
        return shifted;

    /// insert text in the newly created space
    size_t i;
    for (size_t t = 0; t < times; ++t)
        for (i = 0; i < ins_size; ++i, ++str)
            *str = ins[i];

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
    if (str == nullptr || line_width == 0)
        return str_err::nullptr_err;
    if (*str == EOS)
        return 1;

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
            //last_delimiter = i;
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
                if (inserted < 0)
                    return str_err::small_buffer;
            }
            ++lines;
            current_length = 0;
            last_delimiter = -1;
            last_NBSP = -1;
        }

        if (str[i] == EOS)
            break;
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
