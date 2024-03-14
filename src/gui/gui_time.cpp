#include <algorithm>
#include "gui_time.hpp"
#include "timing.h"
#include "metric.h"
#include "sw_timer.hpp"

static uint32_t current_tick = 0;
static uint32_t current_tick_overflows = 0;
static uint32_t current_loop_counter = 0;

static uint32_t loop_start_tick = 0;
static uint32_t worst_duration = 0;

static constexpr uint32_t REPORT_DELAY = 100; // Record highest gui loop duration every 100ms
static Sw_Timer report_timer(REPORT_DELAY);

void gui::TickLoop() {
    uint32_t now = ticks_ms();
    if (current_tick > now) { // overflow
        ++current_tick_overflows;
    }
    current_tick = now;
    current_loop_counter += 1;
}

void gui::StartLoop() {
    gui::TickLoop();
    loop_start_tick = current_tick;
}

static metric_t gui_loop_duration = METRIC("gui_loop_dur", METRIC_VALUE_INTEGER, 100, METRIC_HANDLER_DISABLE_ALL);

void gui::EndLoop() {
    worst_duration = std::max(worst_duration, ticks_ms() - loop_start_tick);
    if (report_timer.RestartIfIsOver(ticks_ms())) {
        metric_record_integer(&gui_loop_duration, worst_duration);
        worst_duration = 0;
    }
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
