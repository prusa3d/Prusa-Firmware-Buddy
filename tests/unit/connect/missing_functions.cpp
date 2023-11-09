#include "common/filepath_operation.h"
#include "common/version.h"
#include <marlin_events.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

void print_begin(const char *, marlin_server::PreviewSkipIfAble) {}

bool f_gcode_get_next_comment_assignment(FILE *, char *, int, char *, int) {
    return false;
}

bool random32bit(uint32_t *output) {
    *output = random();
    return true;
}
