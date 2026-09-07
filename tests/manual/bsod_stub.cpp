// Minimal _bsod for standalone manual test binaries (no Catch2 here):
// print the message and abort.
#include <bsod/bsod.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    va_list args;
    va_start(args, line_number);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, " (%s:%d)\n", file_name, line_number);
    abort();
}
