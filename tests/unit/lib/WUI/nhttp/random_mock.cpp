#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

bool rand_u_secure(uint32_t *out) {
    *out = 0xaaaaaaaa;
    return true;
}

uint32_t rand_u() {
    return 0xaaaaaaaa;
}

uint32_t rand_u_sw() {
    return 0xaaaaaaaa;
}

#ifdef __cplusplus
}
#endif
