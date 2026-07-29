/// @file

#include <random_sw/random_sw.h>

static volatile uint32_t rng_ctx = 0x2a57ead0; ///< Chosen by fair dice roll. Guaranteed to be random.

uint32_t rand_u_sw() {
    rng_ctx = (rng_ctx * 1103515245 + 12345); // glibc LCG constants, much bad, don't use for anything important
    return rng_ctx;
}

void rand_seed_sw(uint32_t seed) {
    // Full period for any seed - the multiplier and the increment satisfy Hull-Dobell, so there is
    // no degenerate state to guard against
    rng_ctx = seed;
}
