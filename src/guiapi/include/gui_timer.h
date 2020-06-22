//gui_timer.h
#ifndef _GUI_TIMER_H
#define _GUI_TIMER_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int8_t gui_timer_create_periodical(uint32_t ms, int16_t win_id);

extern int8_t gui_timer_create_oneshot(uint32_t ms, int16_t win_id);

extern int8_t gui_timer_create_timeout(uint32_t ms, int16_t win_id);
extern int8_t gui_timer_create_txtroll(uint32_t ms, int16_t win_id);

extern void gui_timer_delete(int8_t id);

extern void gui_timers_delete_by_window_id(int16_t win_id);

extern uint32_t gui_timers_cycle(void);

extern void gui_timer_reset(int8_t id);

extern void gui_timer_change_txtroll_peri_delay(uint32_t ms, int16_t win_id);

extern void gui_timer_restart_txtroll(int16_t win_id);

extern int8_t gui_timer_expired(int8_t id);

extern int8_t gui_get_menu_timeout_id(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_GUI_TIMER_H
