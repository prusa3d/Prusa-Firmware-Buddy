#include "time_mock.h"

namespace {

// in milliseconds
uint32_t mock_time = 0;

} // namespace

void advance_time_s(uint32_t by) {
    mock_time += 1000 * by;
}

void advance_time_ms(uint32_t by) {
    mock_time += by;
}

extern "C" {

uint32_t ticks_ms() {
    return mock_time;
}

uint32_t ticks_s() {
    return mock_time / 1000;
}
}

uint32_t osDelay(uint32_t millis) {
    // We simply pretend that much time happened.
    advance_time_ms(millis);
    return 0;
}
