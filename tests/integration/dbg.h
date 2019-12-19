#pragma once

#include <cstdarg>
#include <cstdio>

inline void _dbg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}
