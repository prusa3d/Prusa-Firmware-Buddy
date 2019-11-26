#include "safety_timer.h"

static millis_t safety_timer_interval = 0; // zero if disabled
static millis_t last_reset = 0;

void safety_timer_set_interval(millis_t ms) {
  safety_timer_interval = ms;
  safety_timer_reset();
}

bool safety_timer_is_expired() {
  return safety_timer_interval && ELAPSED(millis(), last_reset + safety_timer_interval);
}

void safety_timer_reset() {
  last_reset = millis();
}
