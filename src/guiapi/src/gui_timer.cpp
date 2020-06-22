// gui_timer.c

#include "gui_timer.h"
#include <string.h>
#include "stm32f4xx_hal.h"
#include "screen.h"

#define GUI_MAX_TIMERS 6

#define GUI_TIMER_NONE    0
#define GUI_TIMER_1SHT    1
#define GUI_TIMER_PERI    2
#define GUI_MENU_TIMEOUT  3
#define GUI_TIMER_TXTROLL 4

typedef struct _gui_timer_t {
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
    int16_t win_id;
} gui_timer_t;

gui_timer_t gui_timers[GUI_MAX_TIMERS];
int8_t gui_timer_count = -1;
int8_t gui_menu_timeout_id = -1;

int8_t gui_timer_new(uint8_t timer, uint32_t ms, int16_t win_id) {
    window_t *window;
    uint32_t tick = HAL_GetTick();
    int8_t id = -1;
    if (gui_timer_count < GUI_MAX_TIMERS) {
        id = 0;
        if (gui_timer_count == -1) //not initialized
        {
            gui_timer_count = 0;
            memset(gui_timers, 0, GUI_MAX_TIMERS * sizeof(gui_timer_t));
        } else //find free id
            while ((id < GUI_MAX_TIMERS) && (gui_timers[id].f_timer != GUI_TIMER_NONE))
                id++;
        if (id < GUI_MAX_TIMERS) //id is valid
        {
            gui_timers[id].start = tick;
            gui_timers[id].delay = ms;
            gui_timers[id].f_timer = timer;
            gui_timers[id].win_id = win_id;
            gui_timer_count++; //increment count
            if ((window = window_ptr(win_id)) != 0)
                window->f_timer = 1; //set timer flag
        } else
            id = -1;
    }
    return id;
}

int8_t gui_timer_create_oneshot(uint32_t ms, int16_t win_id) {
    return gui_timer_new(GUI_TIMER_1SHT, ms, win_id);
}

int8_t gui_timer_create_periodical(uint32_t ms, int16_t win_id) {
    return gui_timer_new(GUI_TIMER_PERI, ms, win_id);
}

int8_t gui_timer_create_timeout(uint32_t ms, int16_t win_id) {
    gui_menu_timeout_id = gui_timer_new(GUI_MENU_TIMEOUT, ms, win_id);
    return gui_menu_timeout_id;
}

int8_t gui_timer_create_txtroll(uint32_t ms, int16_t win_id) {
    return gui_timer_new(GUI_TIMER_TXTROLL, ms, win_id);
}

void gui_timer_delete(int8_t id) {
    if ((id >= 0) && (id < GUI_MAX_TIMERS) && (gui_timers[id].f_timer != GUI_TIMER_NONE)) {
        gui_timers[id].start = 0;
        gui_timers[id].delay = 0;
        if (gui_timers[id].f_timer == GUI_MENU_TIMEOUT)
            gui_menu_timeout_id = -1;
        gui_timers[id].f_timer = GUI_TIMER_NONE;
        gui_timers[id].win_id = 0;
        gui_timer_count--; //decrement count
    }
}

void gui_timers_delete_by_window_id(int16_t win_id) {
    int8_t id;
    for (id = 0; id < GUI_MAX_TIMERS; id++)
        if (gui_timers[id].win_id == win_id)
            gui_timer_delete(id);
}

uint32_t gui_timers_cycle(void) {
    uint32_t tick = HAL_GetTick();
    uint32_t delay;
    uint32_t diff;
    uint32_t diff_min = 0xffffffff;
    uint8_t f_timer;
    uint8_t count = 0;
    int8_t id;
    for (id = 0; (id < GUI_MAX_TIMERS); id++)
        if ((f_timer = gui_timers[id].f_timer) != GUI_TIMER_NONE) {
            if ((delay = gui_timers[id].delay) > 0) {
                diff = tick - gui_timers[id].start;
                if (delay <= diff) {
                    switch (gui_timers[id].f_timer) {
                    case GUI_TIMER_1SHT:
                        screen_dispatch_event(window_ptr(gui_timers[id].win_id), WINDOW_EVENT_TIMER, (void *)(int)id);
                        gui_timers[id].delay = 0;
                        break;
                    case GUI_TIMER_PERI:
                        screen_dispatch_event(window_ptr(gui_timers[id].win_id), WINDOW_EVENT_TIMER, (void *)(int)id);
                        gui_timers[id].start += delay;
                        break;
                    case GUI_MENU_TIMEOUT:
                        gui_timers[id].delay = 0;
                        break;
                    case GUI_TIMER_TXTROLL:
                        screen_dispatch_event(window_ptr(gui_timers[id].win_id), WINDOW_EVENT_TIMER, (void *)(int)id);
                        gui_timers[id].start = tick;
                        break;
                    }
                } else if (diff_min < diff)
                    diff_min = diff;
            }
            if (++count >= gui_timer_count)
                break;
        }
    return diff_min;
}

void gui_timer_reset(int8_t id) {

    if ((id >= 0) && (id < GUI_MAX_TIMERS) && (gui_timers[id].f_timer != GUI_TIMER_NONE))
        gui_timers[id].start = HAL_GetTick();
}

void gui_timer_change_txtroll_peri_delay(uint32_t ms, int16_t win_id) {
    if (gui_timer_count != -1) {
        for (uint8_t id = 0; id < GUI_MAX_TIMERS; id++) {
            if (gui_timers[id].win_id == win_id) {
                gui_timers[id].delay = ms;
                break;
            }
        }
    }
}

void gui_timer_restart_txtroll(int16_t win_id) {
    if (gui_timer_count != -1) {
        for (uint8_t id = 0; id < GUI_MAX_TIMERS; id++) {
            if (gui_timers[id].win_id == win_id) {
                gui_timers[id].start = HAL_GetTick();
                break;
            }
        }
    }
}

int8_t gui_timer_expired(int8_t id) {

    if ((id >= 0) && (id < GUI_MAX_TIMERS) && (gui_timers[id].f_timer != GUI_TIMER_NONE))
        return gui_timers[id].delay == 0 ? 1 : 0;

    return -1;
}

int8_t gui_get_menu_timeout_id(void) {
    return gui_menu_timeout_id;
}
