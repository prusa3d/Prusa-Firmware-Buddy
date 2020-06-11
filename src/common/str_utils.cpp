#include "str_utils.h"
#include <string.h>

static const char *pcustom_set = "";
static const char *pwithdraw_set = "";

static ml_instance_t self_instance;
static ml_instance_t *pinstance = &self_instance;

/// help function (context switching)
void set_instance(ml_instance_t *pinst) {
    pinstance = pinst;
}

/// help function (context switching)
void set_self_instance(void) {
    pinstance = &self_instance;
}

/// Deletes \param n characters from beginning of the \param str
/// \returns number of deleted characters
size_t strdel(char *str, const size_t &n = 1) {
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
/// \returns number of characters shifted
size_t strshift(char *str, const size_t n) {
    // TODO check capacity of the \param str
    if (str == nullptr)
        return 0;

    const size_t size = strlen(str) + 1; /// mind the trailing \0
    for (size_t i = 0; i < size; ++i)
        str[i + n] = str[i];

    return n;
}

/// Inserts \param ins at the beginning of \param str \param times times
/// \returns number of inserted characters
size_t strins(char *str, const char *ins, size_t times) {
    if (str == nullptr || ins == nullptr)
        return 0;

    const size_t ins_size = strlen(ins);
    const size_t inserted = ins_size * times;

    if (0 == strshift(str, inserted))
        return 0;

    size_t i;
    for (size_t t = 0; t < times; ++t)
        for (i = 0; i < ins_size; ++i, ++str)
            *str = ins[i];

    return inserted;
}

/// help function (parametr setter)
void set_custom_set(const char *pstr) {
    pinstance->pcustom_set = pstr;
}

/// help function (parametr setter)
void set_withdraw_set(const char *pstr) {
    pinstance->pwithdraw_set = pstr;
}

/// help function (parametr setter)
void set_defaults(void) {
    pinstance->pcustom_set = "";
    pinstance->pwithdraw_set = "";
}

void set_custom_set(const char *pstr) {
    pcustom_set = pstr;
}

void set_withdraw_set(const char *pstr) {
    pwithdraw_set = pstr;
}

void set_defaults(void) {
    pcustom_set = "";
    pwithdraw_set = "";
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
        case NL:
            ++lines;
            last_delimiter = nullptr;
            break;
        default:
        }

        ++str;
        if (*str == EOS)
            break;
        ++current_length;

        if (current_length > line_width) { /// if the length is too big, break the line
            if (last_delimiter == nullptr) {
                /// no break point available - break a word
                strins(str, &NL);
                ++str;
                if (*str == EOS)
                    break;
            } else {
                /// break at space
                *(last_delimiter) = NL;
                last_delimiter = nullptr;
            }
            ++lines;
        }
    }
    return lines;
}
