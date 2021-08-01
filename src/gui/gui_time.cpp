#include "gui_time.hpp"
#include "timing.h"

static uint32_t current_tick = 0;
static uint32_t current_tick_overflows = 0;

void gui::TickLoop() {
    uint32_t now = ticks_ms();
    if (current_tick > now) { //overflow
        ++current_tick_overflows;
    }
    current_tick = now;
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
    gui::TickLoop();
    return gui::GetTick();
};
