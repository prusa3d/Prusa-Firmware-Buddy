#include "str_utils.h"
#include <string.h>

/// Deletes \param n characters from beginning of the \param str
/// \returns number of deleted characters
size_t strdel(char *str, const size_t n) {
    if (str == nullptr)
        return 0;

    size_t size = strlen(str);
    if (n >= size) {
        str[0] = '\0';
        return size;
    }

    size = size - n + 1; // copy \0 as well
    for (size_t i = 0; i < size; ++i, ++str)
        *str = *(str + n);
    return n;
}

/// Shifts text in \param str by \param n characters
/// \param default_char is inserted if new undefined space appears
/// if \param default_char is 0 then nothing is inserted but the resulting
/// string could be shorter than expected
/// \returns number of characters shifted
size_t strshift(char *str, const size_t n, const char default_char) {
    // TODO check capacity of the \param str
    if (str == nullptr || n == 0)
        return 0;

    const size_t size = strlen(str);
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

/// Inserts \param ins at the beginning of \param str \param times times
/// \returns number of inserted characters
size_t strins(char *str, const char *const ins, size_t times) {
    if (str == nullptr || ins == nullptr)
        return 0;

    const size_t ins_size = strlen(ins);
    const size_t inserted = ins_size * times;

    /// shift the end
    if (0 == strshift(str, inserted, 0))
        return 0;

    /// insert text in the newly created space
    size_t i;
    for (size_t t = 0; t < times; ++t)
        for (i = 0; i < ins_size; ++i, ++str)
            *str = ins[i];

    return inserted;
}

/// Replaces breakable spaces into line breaks in \param str
/// to ensure that no line is longer than \param line_width.
/// If \param line_width is too short,
/// the text will be broken in the middle of the word.
/// Existing line breaks are not removed.
/// \returns number of lines or 0 if no change was done
size_t str2multiline(char *str, const size_t line_width) {
    if (str == nullptr || *str == EOS || line_width == 0)
        return 0;

    char *last_delimiter = nullptr;
    size_t lines = 1;
    size_t current_length = 0;

    /// analyze character
    while (1) {
        switch (*str) {
        case CHAR_SPACE:
            last_delimiter = str;
            break;
        case CHAR_NL:
            ++lines;
            last_delimiter = nullptr;
            current_length = 0;
            break;
        }

        ++str;
        ++current_length;

        if (current_length > line_width) { /// if the length is too big, break the line
            if (last_delimiter == nullptr) {
                /// no break point available - break a word
                strins(str - 1, NL);
                ++str;
                current_length = 1;
            } else {
                /// break at space
                *(last_delimiter) = CHAR_NL;
                current_length = str - last_delimiter - 1; // -1 because the space is replaced
                last_delimiter = nullptr;
            }
            ++lines;
        }
        if (*str == EOS)
            break;
    }
    return lines;
}
