#include "str_utils.h"
#include <string.h>

static const char *pcustom_set = "";
static const char *pwithdraw_set = "";
static int hyphen_distance = HYPHEN_DENY;

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
size_t strdel(char *str, const size_t &n) {
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

/// Inserts \param ins into \param str \param times times
/// \returns number of inserted characters
size_t strins(char *str, const char *ins, size_t times, bool before_flag) {
    // TODO check capacity of the \param str
    if (str == nullptr || ins == nullptr)
        return 0;

    size_t count = strlen(str);
    const size_t n = strlen(ins);
    const size_t inserted = n * times;

    if (before_flag)
        count++;

    if (0 == strshift(str, inserted))
        return 0;

    size_t i;
    for (size_t r = 0; r < times; r++)
        for (i = 0; i < n; i++, str++)
            *str = *(ins + i);

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
void set_hyphen_distance(int dist) {
    pinstance->hyphen_distance = dist;
}

/// help function (parametr setter)
void set_defaults(void) {
    pinstance->pcustom_set = "";
    pinstance->pwithdraw_set = "";
    pinstance->hyphen_distance = HYPHEN_DENY;
}

void set_custom_set(const char *pstr) {
    pcustom_set = pstr;
}

void set_withdraw_set(const char *pstr) {
    pwithdraw_set = pstr;
}

void set_hyphen_distance(int dist) {
    hyphen_distance = dist;
}

void set_defaults(void) {
    pcustom_set = "";
    pwithdraw_set = "";
    hyphen_distance = HYPHEN_DENY;
}

size_t str2plain(char *pstr, bool withdraw_flag) {
    const char *pset = "";

    if (withdraw_flag)
        pset = pwithdraw_set;
    return (str2plain(pstr, pset));
}

/// converts string to plain text
size_t str2plain(char *pstr, const char *withdraw_set, const char *substitute_set, char substitute_char) {
    size_t counter = 0;
    bool flag;

    while (*pstr != EOS) {
        if ((flag = (*pstr == CHAR_HSPACE)) || (strchr(substitute_set, *pstr) != NULL)) {
            if (flag)
                *pstr = CHAR_SPACE;
            else
                *pstr = substitute_char;
            counter++;
        } else if ((*pstr == CHAR_HYPHEN) || (strchr(withdraw_set, *pstr) != NULL)) {
            pstr -= strdel(pstr); // pointer correction needed
            counter++;
        }
        pstr++;
    }
    return counter;
}

/// converts string to multi-line text
size_t str2multiline(char *pstr, size_t line_width) {
    size_t actual_width = 1;
    char *last_delimiter_position = nullptr; // initialization only due to compiler-warning
    delimiter_t delimiter_type = delimiter_t::NONE;
    size_t lines_count = 1;

    while (*pstr != EOS) {
        switch (*pstr) {
        case CHAR_SPACE:
            if (delimiter_type == delimiter_t::HYPHEN) {
                pstr -= strdel(last_delimiter_position); // pointer correction needed
                actual_width--;
            }
            last_delimiter_position = pstr;
            delimiter_type = delimiter_t::SPACE;
            break;
        case CHAR_HSPACE:
            *pstr = CHAR_SPACE;
            break;
        case CHAR_NL:
            if (delimiter_type == delimiter_t::HYPHEN) {
                pstr -= strdel(last_delimiter_position); // pointer correction needed
            }
            delimiter_type = delimiter_t::NONE;
            actual_width = 0;
            lines_count++;
            break;
        case CHAR_HYPHEN:
            if ((pinstance->hyphen_distance == HYPHEN_DENY) || (((delimiter_type == delimiter_t::SPACE) || (delimiter_type == delimiter_t::CUSTOM)) && ((pstr - last_delimiter_position) < pinstance->hyphen_distance))) {
                pstr -= strdel(pstr); // pointer correction needed
                actual_width--;
                break;
            }
            if (delimiter_type == delimiter_t::HYPHEN) {
                pstr -= strdel(last_delimiter_position); // pointer correction needed
                actual_width--;
            }
            last_delimiter_position = pstr;
            delimiter_type = delimiter_t::HYPHEN;
            break;
        default:
            if (strchr(pinstance->pcustom_set, *pstr) != NULL) {
                if (delimiter_type == delimiter_t::HYPHEN) {
                    pstr -= strdel(last_delimiter_position); // pointer correction needed
                    actual_width--;
                }
                last_delimiter_position = pstr;
                delimiter_type = delimiter_t::CUSTOM;
            } else if (strchr(pinstance->pwithdraw_set, *pstr) != NULL) {
                pstr -= strdel(pstr); // pointer correction needed
                actual_width--;
            }
            break;
        }
        if ((line_width != LINE_WIDTH_UNLIMITED) && (actual_width > line_width)) {
            switch (delimiter_type) {
            case delimiter_t::NONE:
                strins(pstr - 1, QT_NL);
                actual_width = 0;
                break;
            case delimiter_t::SPACE:
                *last_delimiter_position = CHAR_NL;
                actual_width = pstr - last_delimiter_position;
                delimiter_type = delimiter_t::NONE;
                break;
            case delimiter_t::HYPHEN:
                *last_delimiter_position = CHAR_MINUS;
                actual_width = pstr - last_delimiter_position;  // !before! "pstr" correction
                pstr += strins(last_delimiter_position, QT_NL); // pointer correction needed / !after! "actual_width" (re)calculation
                delimiter_type = delimiter_t::NONE;
                break;
            case delimiter_t::CUSTOM:
                actual_width = pstr - last_delimiter_position;  // !before! "pstr" correction
                pstr += strins(last_delimiter_position, QT_NL); // pointer correction needed / !after! "actual_width" (re)calculation
                delimiter_type = delimiter_t::NONE;
                break;
            }
            lines_count++;
        }
        pstr++;
        actual_width++;
    }
    return (lines_count);
}
