#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <common/version.h>

extern "C" {

size_t strlcat(char *dst, const char *src, size_t size) {
    /*
     * Note: this is not _completely_ correct. Specifically, if dst is longer
     * than size, it does bad things. Good enough for test purposes.
     */
    const size_t start = strlen(dst);
    strncat(dst + start, src, size - 1 - start);
    return start + strlen(src);
}

uint32_t ticks_ms() {
    return 0;
}

uint32_t ticks_s() {
    return 0;
}

void mbedtls_platform_zeroize(void *b, size_t size) {
    memset(b, 0, size);
}
}

bool random32bit(uint32_t *output) {
    *output = random();
    return true;
}

const char project_version_full[] = "1.0.0";
