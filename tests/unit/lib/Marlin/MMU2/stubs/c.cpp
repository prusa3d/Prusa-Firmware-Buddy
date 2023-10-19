#include "stub_interfaces.h"

// stubbed external interfaces to play with
extern "C" {

static uint32_t ms = 0;

static uint32_t timerCountdown = 0;

void SetTimeoutCountdown(uint32_t tc) {
    timerCountdown = tc;
}

uint32_t millis(void) {
    if (timerCountdown) {
        --timerCountdown;
        ++ms;
    }
    return ms;
}

// a simple way of playing with timeouts
void SetMillis(uint32_t m) { ms = m; }

void IncMillis(uint32_t diff /*= 1*/) {
    ms += diff;
}

} // extern "C"
