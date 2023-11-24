#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void mbedtls_platform_zeroize(void *b, size_t size) {
    memset(b, 0, size);
}

uint32_t rand_u() {
    // Fair roll of 6-sided die, of course
    return 42;
}
