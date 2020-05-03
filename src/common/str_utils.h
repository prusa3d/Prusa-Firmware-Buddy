#ifndef _STR_UTILS_H
#define _STR_UTILS_H

#include <inttypes.h>
#include <string.h>

#define CHAR_SPACE  ' '
#define CHAR_HSPACE '\x1F' // ~ <UnitSeparator>
#define CHAR_LF     '\n'   // ~ <LineFeed>
#define CHAR_NL     CHAR_LF
#define CHAR_HYPHEN '\x1A' // ~ <Substitute>
#define CHAR_MINUS  '-'

#define QT_HSPACE "\x1F" // ~ <UnitSeparator>
#define QT_LF     "\n"   // ~ <LineFeed>
#define QT_NL     QT_LF
#define QT_HYPHEN "\x1A" // ~ <Substitute>

#define HYPHEN_ALLWAYS       0
#define HYPHEN_DENY          -1
#define LINE_WIDTH_UNLIMITED 0

#define EOS '\x00'

enum class delimiter_t : uint8_t { NONE,
    SPACE,
    HYPHEN,
    CUSTOM };

size_t strdel(char *pstr, size_t n = 1);
size_t strins(char *pstr, const char *pinstr, size_t repeater = 1, bool before_flag = false);

void set_custom_set(const char *pstr);
void set_withdraw_set(const char *pstr);
void set_hyphen_distance(int dist);
void set_defaults(void);

size_t str2plain(char *pstr, const char *withdraw_set, const char *substitute_set = "", char substitute_char = CHAR_SPACE);
size_t str2plain(char *pstr, bool withdraw_flag = false);

size_t str2multiline(char *pstr, size_t line_width = LINE_WIDTH_UNLIMITED);

#endif // _STR_UTILS_H
