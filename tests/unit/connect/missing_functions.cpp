#include "common/filepath_operation.h"
#include "common/version.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {

void print_begin(const char *, bool) {}
}

bool f_gcode_get_next_comment_assignment(FILE *, char *, int, char *, int) {
    return false;
}

bool random32bit(uint32_t *output) {
    *output = random();
    return true;
}
