/*
 * bsod.h
 *
 *  Created on: 2019-10-01
 *      Author: Radek Vana
 */

#pragma once

#include <stdint.h>
#if not defined(UNITTESTS)
    #include "error_codes.hpp"
#else
enum class ErrCode;
#endif

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define bsod(fmt, ...) _bsod(fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)

void __attribute__((noreturn, format(__printf__, 1, 4)))
_bsod(const char *fmt, const char *file_name, int line_number, ...); // with file name and line number

/** Fatal error that causes redscreen
 * @param error - error message, that will be displayed as error description (MAX length 107 chars)
 * @param module - module affected by error will be displayed as error title (MAX length 20 chars)
 */
[[noreturn]] void fatal_error(const char *error, const char *module);

#ifdef __cplusplus
}
#endif //__cplusplus

[[noreturn]] void fatal_error(const ErrCode error_code, ...);

[[noreturn]] void raise_redscreen(ErrCode error_code, const char *error, const char *module);
