// gui.hpp
#pragma once

#include "guiconfig.h"
#include "guitypes.h"
#include "gui_timer.h"
#include "display_helper.h"
#include "display.h"

typedef void(gui_loop_cb_t)(void);

void gui_run(void);
void *gui_malloc(unsigned int size);
void gui_free(void *ptr);
void gui_init(void);
void gui_invalidate(void);
void gui_redraw(void);

#ifdef GUI_JOGWHEEL_SUPPORT
    #include "jogwheel.h"
void gui_reset_jogwheel(void);
#endif //GUI_JOGWHEEL_SUPPORT

#ifdef GUI_WINDOW_SUPPORT
    #include "window.hpp"
    #include "window_frame.hpp"
    #include "window_text.hpp"
    #include "window_roll_text.hpp"
    #include "window_numb.hpp"
    #include "window_icon.hpp"
    #include "window_list.hpp"
    #include "window_spin.hpp"
    #include "window_term.hpp"
    #include "window_msgbox.hpp"
    #include "window_progress.hpp"
    #include "window_qr.hpp"
    #include "screen.h"
uint8_t gui_get_nesting(void);
void gui_loop(void);
void gui_reset_menu_timer();
int gui_msgbox_ex(const char *title, const char *text, uint16_t flags, rect_ui16_t rect, uint16_t id_icon, const char **buttons);
int gui_msgbox(const char *text, uint16_t flags);
int gui_msgbox_prompt(const char *text, uint16_t flags);
#endif //GUI_WINDOW_SUPPORT

extern gui_defaults_t gui_defaults;
extern gui_loop_cb_t *gui_loop_cb;
extern int8_t menu_timeout_enabled;

#ifdef GUI_USE_RTOS
    #include "cmsis_os.h"
extern osThreadId gui_task_handle;
#endif //GUI_USE_RTOS
