#include "bsod.h"

void abort() {
    bsod("aborted");
}

int _isatty(int __attribute__((unused)) fd) {
    // TTYs are not supported
    return 0;
}
