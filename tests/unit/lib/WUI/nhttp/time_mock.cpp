#include "time_mock.h"

namespace {

uint32_t mock_time = 0;

}

void set_time(uint32_t time) {
    mock_time = time;
}

extern "C" {

uint32_t ticks_s() {
    return mock_time;
}

uint32_t ticks_ms() {
    return 1000 * mock_time;
}
}
