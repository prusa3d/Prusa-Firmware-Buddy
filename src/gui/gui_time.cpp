/// @file
#include "gui_time.hpp"

#include <common/timing.h>

static uint32_t current_tick = 0;

void gui::TickLoop() {
    current_tick = ticks_ms();
}

uint32_t gui::GetTick() {
    return current_tick;
}
