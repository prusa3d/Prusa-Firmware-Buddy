/*
 * bsod.h
 *
 *  Created on: 2019-10-01
 *      Author: Radek Vana
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// following does not work with macro parameters
// #define QUOTE_X(t)#t
// #define QUOTE(t)QUOTE_X(t)
// #define bsod(format, ...) _bsod(QUOTE(FILE:\n %s\nLINE: %d\n) format, __FILE__ , __LINE__ , ##__VA_ARGS__)

#define bsod(fmt, ...) _bsod(fmt, __FILE__, __LINE__, ##__VA_ARGS__)

// no file name
#define bsod_nofn(fmt, ...) _bsod(fmt, 0, __LINE__, ##__VA_ARGS__)
// no line number
#define bsod_noln(fmt, ...) _bsod(fmt, __FILE__, -1, ##__VA_ARGS__)
// no file name, no line number
#define bsod_nofn_noln(fmt, ...) _bsod(fmt, 0, -1, ##__VA_ARGS__)

__attribute__((noreturn)) void _bsod(const char *fmt, const char *file_name, int line_number, ...); // with file name and line number

/** Fatal error that causes redscreen
 * @param error - error message, that will be displayed as error description (MAX length 107 chars)
 * @param module - module affected by error will be displayed as error title (MAX length 20 chars)
 */
__attribute__((noreturn)) void fatal_error(const char *error, const char *module);

#ifdef __cplusplus
}
#endif //__cplusplus

/**
 * @brief Mark when BSOD would be shown and allow dumping a new BSOD.
 */
void bsod_mark_shown();
