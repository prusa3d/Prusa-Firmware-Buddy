#include "str_utils.h"
#include <string.h>

static const char *pcustom_set = "";
static const char *pwithdraw_set = "";
static int hyphen_distance = HYPHEN_DENY;

size_t strdel(char *pstr, size_t n) {
    size_t count, i;

    count = strlen(pstr);
    if (n > count)
        n = count;
    count = count - n + 1;
    for (i = 0; i < count; i++, pstr++)
        *pstr = *(pstr + n);
    return (n);
}

size_t strins(char *pstr, const char *pinstr, size_t repeater, bool before_flag) {
    size_t count, n, n_ins;
    size_t i, r;

    count = strlen(pstr);
    n = strlen(pinstr);
    n_ins = n * repeater;
    pstr += count; // ~ EOS position
    if (before_flag)
        count++;
    for (i = 0; i < count; i++, pstr--)
        *(pstr + n_ins) = *pstr;
    pstr++;
    for (r = 0; r < repeater; r++)
        for (i = 0; i < n; i++, pstr++)
            *pstr = *(pinstr + i);
    return (n_ins);
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
    return (counter);
}

size_t str2plain(char *pstr, bool withdraw_flag) {
    const char *pset = "";

    if (withdraw_flag)
        pset = pwithdraw_set;
    return (str2plain(pstr, pset));
}

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
            if ((hyphen_distance == HYPHEN_DENY) || (((delimiter_type == delimiter_t::SPACE) || (delimiter_type == delimiter_t::CUSTOM)) && ((pstr - last_delimiter_position) < hyphen_distance))) {
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
            if (strchr(pcustom_set, *pstr) != NULL) {
                if (delimiter_type == delimiter_t::HYPHEN) {
                    pstr -= strdel(last_delimiter_position); // pointer correction needed
                    actual_width--;
                }
                last_delimiter_position = pstr;
                delimiter_type = delimiter_t::CUSTOM;
            } else if (strchr(pwithdraw_set, *pstr) != NULL) {
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
