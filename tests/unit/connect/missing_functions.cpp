#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {

size_t strlcpy(char *, const char *, size_t);

void get_LFN(char *lfn, size_t lfn_size, char *path) {
    strlcpy(lfn, basename(path), lfn_size);
}

void mbedtls_platform_zeroize(void *b, size_t size) {
    memset(b, 0, size);
}
}

bool f_gcode_get_next_comment_assignment(FILE *, char *, int, char *, int) {
    return false;
}

bool random32bit(uint32_t *output) {
    *output = random();
    return true;
}
