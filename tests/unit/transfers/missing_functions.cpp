#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {

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
