#include <algorithm>
#include "gui_time.hpp"
#include "timing.h"
#include "sw_timer.hpp"

#ifndef UNITTESTS
    #include "metric.h"
#endif

static uint32_t current_tick = 0;
static uint32_t current_loop_counter = 0;

static uint32_t loop_start_tick = 0;
static uint32_t worst_duration = 0;

static constexpr uint32_t REPORT_DELAY = 100; // Record highest gui loop duration every 100ms
static Sw_Timer report_timer(REPORT_DELAY);

void gui::TickLoop() {
    current_tick = ticks_ms();
    current_loop_counter += 1;
}

void gui::StartLoop() {
    gui::TickLoop();
    loop_start_tick = current_tick;
}

#ifndef UNITTESTS
METRIC_DEF(gui_loop_duration, "gui_loop_dur", METRIC_VALUE_INTEGER, 100, METRIC_DISABLED);
#endif

void gui::EndLoop() {
    worst_duration = std::max(worst_duration, ticks_ms() - loop_start_tick);
    if (report_timer.RestartIfIsOver(ticks_ms())) {
#ifndef UNITTESTS
        metric_record_integer(&gui_loop_duration, worst_duration);
#endif
        worst_duration = 0;
    }
}

uint32_t gui::GetLoopCounter() {
    return current_loop_counter;
}

uint32_t gui::GetTick() {
    return current_tick;
}

uint32_t gui::GetTick_ForceActualization() {
    gui::StartLoop();
    return gui::GetTick();
};
