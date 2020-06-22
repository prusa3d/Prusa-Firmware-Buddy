#pragma once

#include <inttypes.h>
#include <string.h>

constexpr char CHAR_SPACE = ' ';
constexpr char CHAR_NBSP = '\xA0';  /// Non Breaking Space
#define NBSP "\xA0"                 /// Non Breaking Space
constexpr char CHAR_NL = '\n';      /// New Line
constexpr char NL[2] = { CHAR_NL }; /// New Line
constexpr char EOS = '\0';          /// end of string

#ifdef __cplusplus

size_t strdel(char *str, const size_t n = 1);
int strins(char *str, size_t max_size, const char *const ins, size_t times = 1);
int strshift(char *str, size_t max_size, const size_t n = 1, const char default_char = ' ');
extern "C" int str2multiline(char *str, size_t max_size, const size_t line_width);

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
