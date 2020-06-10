#pragma once

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

enum class delimiter_t {
    NONE,
    SPACE,
    HYPHEN,
    CUSTOM
};

size_t strdel(char *pstr, size_t n = 1);
size_t strins(char *pstr, const char *pinstr, size_t repeater = 1, bool before_flag = false);

void set_custom_set(const char *pstr);
void set_withdraw_set(const char *pstr);
void set_hyphen_distance(int dist);
void set_defaults(void);

size_t str2plain(char *pstr, const char *withdraw_set, const char *substitute_set = "", char substitute_char = CHAR_SPACE);
size_t str2plain(char *pstr, bool withdraw_flag = false);

size_t str2multiline(char *pstr, size_t line_width = LINE_WIDTH_UNLIMITED);

#include <inttypes.h>
#include <string.h>

#define CHAR_SPACE  ' '
#define CHAR_HSPACE '\xA0' // ~ <NonBreakingSpace>
#define CHAR_LF     '\n'   // ~ <LineFeed>
#define CHAR_NL     CHAR_LF
#define CHAR_HYPHEN '\xAD' // ~ <SoftHyphen>
#define CHAR_MINUS  '-'

#define QT_HSPACE "\xA0" // ~ <NonBreakingSpace>
#define QT_LF     "\n"   // ~ <LineFeed>
#define QT_NL     QT_LF
#define QT_HYPHEN "\xAD" // ~ <SoftHyphen>

#define HYPHEN_ALLWAYS       0
#define HYPHEN_DENY          -1
#define LINE_WIDTH_UNLIMITED 0

#define EOS '\x00'

#pragma pack(push, 1)
typedef enum {
    ML_MODE_NONE,
    ML_MODE_WORDB,
    ML_MODE_EXT
} ml_mode_t;
#pragma pack(pop)

#ifdef __cplusplus

typedef struct
{
    const char *pcustom_set = "";
    const char *pwithdraw_set = "";
    int hyphen_distance = HYPHEN_DENY;
} ml_instance_t;

extern "C" void set_instance(ml_instance_t *pinst);
void set_self_instance(void);

size_t strdel(char *str, const size_t &n = 1);
size_t strins(char *pstr, const char *pinstr, size_t repeater = 1, bool before_flag = false);

void set_custom_set(const char *pstr);
void set_withdraw_set(const char *pstr);
void set_hyphen_distance(int dist);
void set_defaults(void);

size_t str2plain(char *pstr, const char *withdraw_set, const char *substitute_set = "", char substitute_char = CHAR_SPACE);
size_t str2plain(char *pstr, bool withdraw_flag = false);

extern "C" size_t str2multiline(char *pstr, size_t line_width = LINE_WIDTH_UNLIMITED);

#else

typedef struct
{
    const char *pcustom_set;
    const char *pwithdraw_set;
    int hyphen_distance;
} ml_instance_t;

void set_instance(ml_instance_t *pinst);

size_t str2multiline(char *pstr, size_t line_width);

#endif // __cplusplus
