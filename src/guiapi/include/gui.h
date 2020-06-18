// gui.h
#ifndef _GUI_H
#define _GUI_H

#include "guiconfig.h"
#include "guitypes.h"
#include "gui_timer.h"
//#include "display.h"
#include "display_helper.h"

#ifdef GUI_JOGWHEEL_SUPPORT
    #include "jogwheel.h"
#endif //GUI_JOGWHEEL_SUPPORT

#ifdef GUI_WINDOW_SUPPORT
    #include "window.h"
    #include "window_frame.h"
    #include "window_text.h"
    #include "window_roll_text.h"
    #include "window_numb.h"
    #include "window_icon.h"
    #include "window_list.h"
    #include "window_spin.h"
    #include "window_term.h"
    #include "window_msgbox.h"
    #include "window_progress.h"
    #include "window_qr.h"
    #include "screen.h"
#endif //GUI_WINDOW_SUPPORT

#ifdef GUI_USE_RTOS
    #include "cmsis_os.h"
#endif //GUI_USE_RTOS

typedef void(gui_loop_cb_t)(void);

#ifdef __cplusplus
    #include "display.h"
extern "C" {
#endif //__cplusplus

#ifdef GUI_USE_RTOS
extern osThreadId gui_task_handle;
#endif //GUI_USE_RTOS

extern gui_defaults_t gui_defaults;

extern gui_loop_cb_t *gui_loop_cb;

extern int8_t menu_timeout_enabled;

extern void *gui_malloc(unsigned int size);

extern void gui_free(void *ptr);

extern void gui_init(void);

extern void gui_invalidate(void);

extern void gui_redraw(void);

#ifdef GUI_WINDOW_SUPPORT

extern uint8_t gui_get_nesting(void);
extern void gui_loop(void);

extern void gui_reset_menu_timer();

extern int gui_msgbox_ex(const char *title, const char *text, uint16_t flags, rect_ui16_t rect, uint16_t id_icon, const char **buttons);

extern int gui_msgbox(const char *text, uint16_t flags);

extern int gui_msgbox_prompt(const char *text, uint16_t flags);

#endif //GUI_WINDOW_SUPPORT

#ifdef GUI_JOGWHEEL_SUPPORT

extern void gui_reset_jogwheel(void);

#endif //GUI_JOGWHEEL_SUPPORT

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_GUI_H
