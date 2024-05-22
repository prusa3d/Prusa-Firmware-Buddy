// Software implementation of random.h

#include "random.h"

#include <stdlib.h>

RAND_DECL uint32_t rand_u() {
    return rand_u_sw();
}

RAND_DECL uint32_t rand_u_sw() {
    return uint32_t(std::rand());
}

// Do not implement this function - we cannot provide secure rand with just SW implementation
// RAND_DECL bool rand_u_secure(uint32_t *out) {}
