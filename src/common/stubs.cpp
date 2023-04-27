#include "bsod.h"

void abort() {
    bsod("aborted");
}

void __assert_func(const char *file, int line, const char *func, const char *msg) {
    (void)func;
    _bsod("ASSERT %s", file, line, msg);
}

int _isatty(int __attribute__((unused)) fd) {
    // TTYs are not supported
    return 0;
}
