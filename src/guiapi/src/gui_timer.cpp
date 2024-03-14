// gui_timer.cpp

#include "gui_timer.h"
#include <string.h>
#include "ScreenHandler.hpp"
#include "text_roll.hpp"
#include <algorithm>
static const constexpr uint8_t GUI_MAX_TIMERS = 6;

enum {
    GUI_TIMER_NONE,
    GUI_TIMER_1SHT,
    GUI_TIMER_PERI,
};

struct gui_timer_t {
    uint32_t start;
    uint32_t delay : 24;
    union {
        uint8_t flags : 8;
        struct
        {
            uint8_t f_reserved0 : 5;
            uint8_t f_timer : 3;
        };
    };
    window_t *pWin;
};

gui_timer_t gui_timers[GUI_MAX_TIMERS];
int8_t gui_timer_count = -1;
int8_t gui_menu_timeout_id = -1;

int8_t gui_timer_new(window_t *pWin, uint8_t timer, uint32_t ms) {
    window_t *window;
    uint32_t tick = gui::GetTick();
    int8_t id = -1;
    if (gui_timer_count < GUI_MAX_TIMERS) {
        id = 0;
        if (gui_timer_count == -1) // not initialized
        {
            gui_timer_count = 0;
            memset(gui_timers, 0, GUI_MAX_TIMERS * sizeof(gui_timer_t));
        } else { // find free id
            while ((id < GUI_MAX_TIMERS) && (gui_timers[id].f_timer != GUI_TIMER_NONE)) {
                id++;
            }
        }
        if (id < GUI_MAX_TIMERS) // id is valid
        {
            gui_timers[id].start = tick;
            gui_timers[id].delay = ms;
            gui_timers[id].f_timer = timer;
            gui_timers[id].pWin = pWin;
            gui_timer_count++; // increment count
            if ((window = Screens::Access()->Get()) != 0) {
                window->SetHasTimer(); // set timer flag
            }
        } else {
            id = -1;
        }
    }
    return id;
}

int8_t gui_timer_create_oneshot(window_t *pWin, uint32_t ms) {
    return gui_timer_new(pWin, GUI_TIMER_1SHT, ms);
}

int8_t gui_timer_create_periodical(window_t *pWin, uint32_t ms) {
    return gui_timer_new(pWin, GUI_TIMER_PERI, ms);
}

void gui_timer_delete(int8_t id) {
    if ((id >= 0) && (id < GUI_MAX_TIMERS) && (gui_timers[id].f_timer != GUI_TIMER_NONE)) {
        gui_timers[id].start = 0;
        gui_timers[id].delay = 0;
        gui_timers[id].f_timer = GUI_TIMER_NONE;
        gui_timers[id].pWin = nullptr;
        gui_timer_count--; // decrement count
    }
}

void gui_timers_delete_by_window(window_t *pWin) {
    int8_t id;
    for (id = 0; id < GUI_MAX_TIMERS; id++) {
        if (gui_timers[id].pWin == pWin) {
            gui_timer_delete(id);
        }
    }
}

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
    uint32_t delay;
    uint32_t diff;
    uint32_t diff_min = 0xffffffff;
    uint8_t f_timer;
    uint8_t count = 0;
    int8_t id;

    diff_min = fire_text_roll_event(tick, diff_min);

    for (id = 0; (id < GUI_MAX_TIMERS); id++) {
        if ((f_timer = gui_timers[id].f_timer) != GUI_TIMER_NONE) {
            if ((delay = gui_timers[id].delay) > 0) {
                diff = tick - gui_timers[id].start;
                if (delay <= diff) {
                    switch (gui_timers[id].f_timer) {
                    case GUI_TIMER_1SHT:
                        Screens::Access()->ScreenEvent(gui_timers[id].pWin, GUI_event_t::TIMER, (void *)(int)id);
                        gui_timers[id].delay = 0;
                        break;
                    case GUI_TIMER_PERI:
                        Screens::Access()->ScreenEvent(gui_timers[id].pWin, GUI_event_t::TIMER, (void *)(int)id);
                        gui_timers[id].start += delay;
                        break;
                    }
                } else if (diff_min < diff) {
                    diff_min = diff;
                }
            }
            if (++count >= gui_timer_count) {
                break;
            }
        }
    }
    return diff_min;
}

void gui_timer_reset(int8_t id) {

    if ((id >= 0) && (id < GUI_MAX_TIMERS) && (gui_timers[id].f_timer != GUI_TIMER_NONE)) {
        gui_timers[id].start = gui::GetTick();
    }
}

int8_t gui_timer_expired(int8_t id) {

    if ((id >= 0) && (id < GUI_MAX_TIMERS) && (gui_timers[id].f_timer != GUI_TIMER_NONE)) {
        return gui_timers[id].delay == 0 ? 1 : 0;
    }

    return -1;
}

int8_t gui_get_menu_timeout_id(void) {
    return gui_menu_timeout_id;
}
