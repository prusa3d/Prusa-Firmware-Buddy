#include "common/filepath_operation.h"
#include "common/version.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {

size_t strlcpy(char *, const char *, size_t);

size_t strlcat(char *dst, const char *src, size_t size) {
    /*
     * Note: this is not _completely_ correct. Specifically, if dst is longer
     * than size, it does bad things. Good enough for test purposes.
     */
    const size_t start = strlen(dst);
    strncat(dst + start, src, size - 1 - start);
    return start + strlen(src);
}

void get_LFN(char *lfn, size_t lfn_size, char *path) {
    strlcpy(lfn, basename_b(path), lfn_size);
}

void get_SFN_path(char *path) {
}

void mbedtls_platform_zeroize(void *b, size_t size) {
    memset(b, 0, size);
}

void fatal_error(const char *, const char *) {
    abort();
}

const char project_version_full[] = "1.0.0";

void print_begin(const char *, bool) {}
}

bool f_gcode_get_next_comment_assignment(FILE *, char *, int, char *, int) {
    return false;
}

bool random32bit(uint32_t *output) {
    *output = random();
    return true;
}
