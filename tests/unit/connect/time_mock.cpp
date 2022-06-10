#include "time_mock.h"

namespace {

uint32_t mock_time = 0;

}

void advance_time(uint32_t by) {
    mock_time += by;
}

extern "C" {

uint32_t ticks_ms() {
    return mock_time;
}
}
