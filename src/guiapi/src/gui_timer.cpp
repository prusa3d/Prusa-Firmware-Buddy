// gui_timer.cpp

#include "gui_timer.h"
#include <string.h>
#include "ScreenHandler.hpp"
#include "text_roll.hpp"
#include <algorithm>

uint32_t fire_text_roll_event(uint32_t tick, uint32_t diff_min) {
    static uint32_t last_tick = 0;
    if (txtroll_t::HasInstance() && ((tick - last_tick) >= txtroll_t::GetBaseTick())) {
        last_tick = tick;
        diff_min = std::min(diff_min, txtroll_t::GetBaseTick());
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::TEXT_ROLL, nullptr);
    }
    return diff_min;
}

uint32_t gui_timers_cycle(void) {
    uint32_t tick = gui::GetTick();
    uint32_t diff_min = 0xffffffff;
    diff_min = fire_text_roll_event(tick, diff_min);
    return diff_min;
}
