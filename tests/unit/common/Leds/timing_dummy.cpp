#include "timing_dummy.hpp"

uint32_t timer::time = 0;

uint32_t ticks_ms() {
    return timer::time;
}
