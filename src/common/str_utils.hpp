#pragma once

#include <inttypes.h>
#include <string.h>

constexpr char CHAR_SPACE = ' ';
constexpr char CHAR_NBSP = '\xA0'; /// Non Breaking Space
#define NBSP "\xA0"                /// Non Breaking Space
constexpr char CHAR_NL = '\n';     /// New Line
#define NL "\n"                    /// New Line
constexpr char EOS = '\0';         /// End Of String

enum str_err {
    nullptr_err = -1,
    small_buffer = -2
};

size_t strdel(char *str, const size_t n = 1);
int strins(char *str, size_t max_size, const char *const ins, size_t times = 1);
int strshift(char *str, size_t max_size, const size_t n = 1, const char default_char = ' ');
int str2multiline(char *str, size_t max_size, const size_t line_width);

int strshiftUnicode(uint32_t *str, size_t max_size, const size_t n = 1, const uint32_t default_char = ' ');
int strinsUnicode(uint32_t *str, size_t max_size, const uint32_t *const ins, size_t times = 1);
int str2multilineUnicode(uint32_t *str, size_t max_size, const size_t line_width);
