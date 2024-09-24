#include <common/filepath_operation.h>
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

extern "C" {
void notify_reconfigure() {}

void netdev_get_hostname(uint32_t netdev_id, char *buffer, size_t buffer_len) {}

void *calloc_fallible(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}
}
