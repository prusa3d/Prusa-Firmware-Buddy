#include <algorithm>
#include "gui_time.hpp"
#include "timing.h"

static uint32_t current_tick = 0;
static uint32_t current_tick_overflows = 0;
static uint32_t current_loop_counter = 0;

void gui::TickLoop() {
    uint32_t now = ticks_ms();
    if (current_tick > now) { //overflow
        ++current_tick_overflows;
    }
    current_tick = now;
    current_loop_counter += 1;
}

void gui::StartLoop() {
    gui::TickLoop();
}

void gui::EndLoop() {
}

uint32_t gui::GetLoopCounter() {
    return current_loop_counter;
}

uint32_t gui::GetTick() {
    return current_tick;
}

uint64_t gui::GetTickU64() {
    uint64_t u64_tick = uint64_t(current_tick_overflows);
    u64_tick <<= 32;
    u64_tick |= uint64_t(current_tick);
    return u64_tick;
}

uint32_t gui::GetTick_IgnoreTickLoop() {
    return ticks_ms();
}

uint32_t gui::GetTick_ForceActualization() {
    gui::StartLoop();
    return gui::GetTick();
};
