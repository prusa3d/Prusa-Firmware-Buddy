// gui.hpp
#pragma once

#include "guiconfig.h"
#include "guitypes.hpp"
#include "gui_timer.h"
#include "display_helper.h"
#include "display.h"

typedef void(gui_loop_cb_t)(void);

extern gui_defaults_t gui_defaults;

extern gui_loop_cb_t *gui_loop_cb;

extern int8_t menu_timeout_enabled;

extern void gui_run(void);

extern void *gui_malloc(unsigned int size);

extern void gui_free(void *ptr);

extern void gui_init(void);

extern void gui_invalidate(void);

extern void gui_redraw(void);

#ifdef GUI_USE_RTOS
    #include "cmsis_os.h"

extern osThreadId gui_task_handle;

#endif //GUI_USE_RTOS

#ifdef GUI_JOGWHEEL_SUPPORT
    #include "jogwheel.h"

extern void gui_reset_jogwheel(void);

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

extern uint8_t gui_get_nesting(void);

extern void gui_loop(void);

extern void gui_reset_menu_timer();

extern int gui_msgbox_ex(string_view_utf8 title, string_view_utf8 text, uint16_t flags, rect_ui16_t rect, uint16_t id_icon, const char **buttons);

extern int gui_msgbox(string_view_utf8 text, uint16_t flags);

extern int gui_msgbox_prompt(string_view_utf8 text, uint16_t flags);

// close currently displayed msgbox, returns 1 if msgbox successfully closed or 0 if no msgbox currently displayed
// message box loop will stop and gui_msgbox function returns MSGBOX_RES_CLOSED
extern int gui_msgbox_close(void);

#endif //GUI_WINDOW_SUPPORT
