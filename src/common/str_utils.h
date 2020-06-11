#pragma once

#include <inttypes.h>
#include <string.h>

constexpr char CHAR_SPACE = ' ';
constexpr char NBSP = '\xA0'; /// Non Breaking Space
constexpr char NL = '\n';     /// New Line
constexpr char EOS = '\0';    /// end of string

#define LINE_WIDTH_UNLIMITED 0

enum delimiter_t {
    NONE,
    SPACE,
    HYPHEN,
    CUSTOM
};

#pragma pack(push, 1)
typedef enum {
    ML_MODE_NONE,
    ML_MODE_WORDB,
    ML_MODE_EXT
} ml_mode_t;
#pragma pack(pop)

#ifdef __cplusplus

typedef struct {
    const char *pcustom_set = "";
    const char *pwithdraw_set = "";
    int hyphen_distance = 0;
} ml_instance_t;

extern "C" void set_instance(ml_instance_t *pinst);
void set_self_instance(void);

size_t strdel(char *str, const size_t &n = 1);
size_t strins(char *str, const char *ins, size_t times = 1);

void set_custom_set(const char *pstr);
void set_withdraw_set(const char *pstr);
void set_hyphen_distance(int dist);
void set_defaults(void);

extern "C" size_t str2multiline(char *str, const size_t line_width);

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
