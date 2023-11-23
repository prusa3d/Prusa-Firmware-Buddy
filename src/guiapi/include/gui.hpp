// gui.hpp
#pragma once

#include "guiconfig.h"
#include "guitypes.hpp"
#include "gui_timer.h"
#include "display_helper.h"
#include "display.h"
#include "GuiDefaults.hpp"

typedef void(gui_loop_cb_t)(void);

extern void gui_run(void);

extern void gui_init(void);

extern void gui_redraw(void);

#ifdef GUI_USE_RTOS
    #include "cmsis_os.h"

extern osThreadId gui_task_handle;

#endif // GUI_USE_RTOS

#ifdef GUI_WINDOW_SUPPORT
    #include "window.hpp"
    #include "window_frame.hpp"
    #include "window_text.hpp"
    #include "window_roll_text.hpp"
    #include "window_numb.hpp"
    #include "window_icon.hpp"
    #include "window_term.hpp"
    #include "window_msgbox.hpp"
    #include "window_progress.hpp"
    #include "window_qr.hpp"
    #include "circle_buffer.hpp"

extern uint8_t gui_get_nesting(void);

extern void gui_loop_cb();

extern void gui_loop(void);
extern void gui_error_run(void);

extern void gui_bare_loop(void);

// meant to be use as MsgCircleBuffer().push_back(txt);
inline constexpr size_t MSG_STACK_SIZE = 8 + 1; // status message stack size
inline constexpr size_t MSG_MAX_LENGTH = 63; // status message max length
using MsgBuff_t = CircleStringBuffer<MSG_STACK_SIZE, MSG_MAX_LENGTH>;

MsgBuff_t &MsgCircleBuffer();
void MsgCircleBuffer_cb(const char *txt);

#endif // GUI_WINDOW_SUPPORT
