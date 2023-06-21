// gui_timer.h
#pragma once

#include <inttypes.h>
#include "window.hpp"

extern int8_t gui_timer_create_periodical(window_t *pWin, uint32_t ms);

extern int8_t gui_timer_create_oneshot(window_t *pWin, uint32_t ms);

extern void gui_timer_delete(int8_t id);

extern void gui_timers_delete_by_window(window_t *pWin);

extern uint32_t gui_timers_cycle(void);

extern void gui_timer_reset(int8_t id);

extern int8_t gui_timer_expired(int8_t id);

extern int8_t gui_get_menu_timeout_id(void);
