#include <cstdint>
#include <cstdlib>

extern "C" {

uint32_t ticks_ms() {
    return 0;
}

uint32_t ticks_s() {
    return 0;
}
}

bool random32bit(uint32_t *output) {
    *output = random();
    return true;
}
