#include <cstdint>

extern "C" {

uint32_t ticks_ms() {
    // Mock only
    return 0;
}

uint32_t ticks_s() {
    // Mock only
    return 0;
}
}

uint32_t osDelay(uint32_t) {
    // Mock only
    return 0;
}
