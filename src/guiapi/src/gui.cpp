// gui.c
#include "display.h"
#include "gui.hpp"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "sound.hpp"
#include "ScreenHandler.hpp"
#include "IDialog.hpp"

#define GUI_FLG_INVALID 0x0001

uint16_t gui_flags = 0;

#ifdef GUI_JOGWHEEL_SUPPORT
int gui_jogwheel_encoder = 0;
int gui_jogwheel_button_down = 0;
#endif //GUI_JOGWHEEL_SUPPORT

#ifdef GUI_USE_RTOS
osThreadId gui_task_handle = 0;
#endif //GUI_USE_RTOS

font_t *GuiDefaults::Font = 0;
font_t *GuiDefaults::FontBig = 0;

constexpr padding_ui8_t GuiDefaults::Padding;
constexpr Rect16 GuiDefaults::RectHeader;
constexpr Rect16 GuiDefaults::RectScreenBody;
constexpr Rect16 GuiDefaults::RectScreenBodyNoFoot;
constexpr Rect16 GuiDefaults::RectScreen;
constexpr Rect16 GuiDefaults::RectFooter;

gui_loop_cb_t *gui_loop_cb = 0;
uint32_t gui_loop_tick = 0;

void gui_init(void) {
    display::Init();
#ifdef GUI_JOGWHEEL_SUPPORT
    jogwheel_init();
    gui_reset_jogwheel();
#endif //GUI_JOGWHEEL_SUPPORT
    gui_task_handle = osThreadGetId();
}

void gui_redraw(void) {
    if (gui_flags & GUI_FLG_INVALID) {
        Screens::Access()->Draw();
        gui_flags &= ~GUI_FLG_INVALID;
    }
}

//at least one window is invalid
void gui_invalidate(void) {
    gui_flags |= GUI_FLG_INVALID;
#ifdef GUI_USE_RTOS
    osSignalSet(gui_task_handle, GUI_SIG_REDRAW);
#endif //GUI_USE_RTOS
}

#define GUI_DELAY_MIN  1
#define GUI_DELAY_MAX  10
#define GUI_DELAY_LOOP 100

#ifdef GUI_WINDOW_SUPPORT

static uint8_t guiloop_nesting = 0;
uint8_t gui_get_nesting(void) { return guiloop_nesting; }

void gui_loop(void) {
    ++guiloop_nesting;
    uint32_t delay;
    uint32_t tick;
    #ifdef GUI_JOGWHEEL_SUPPORT

    // Encoder sound moved from guimain to gui loop to control encoder sounds in
    // every gui screens. Previous method wasn't everywhere.
    if ((jogwheel_changed & 1) && jogwheel_button_down) { //button changed and pressed
        Sound_Play(eSOUND_TYPE_ButtonEcho);
    } else if (jogwheel_changed & 2) { // encoder changed
        Sound_Play(eSOUND_TYPE_EncoderMove);
    }

    if (jogwheel_changed) {
        if (gui_loop_cb)
            gui_loop_cb();
        jogwheel_changed = 0;
        window_t *capturedWin = window_t::GetCapturedWindow();
        if ((jogwheel_encoder != gui_jogwheel_encoder)) {
            int dif = jogwheel_encoder - gui_jogwheel_encoder;
            if (capturedWin) {
                if (dif > 0)
                    capturedWin->WindowEvent(capturedWin, WINDOW_EVENT_ENC_UP, (void *)dif);
                else if (dif < 0)
                    capturedWin->WindowEvent(capturedWin, WINDOW_EVENT_ENC_DN, (void *)-dif);
            }
            gui_jogwheel_encoder = jogwheel_encoder;
            gui_reset_menu_timer();
        }
        if (!jogwheel_button_down ^ !gui_jogwheel_button_down) {
            if (capturedWin) {
                if (gui_jogwheel_button_down)
                    capturedWin->WindowEvent(capturedWin, WINDOW_EVENT_BTN_UP, 0);
                else
                    capturedWin->WindowEvent(capturedWin, WINDOW_EVENT_BTN_DN, 0);
            }
            gui_jogwheel_button_down = jogwheel_button_down;
            gui_reset_menu_timer();
        }
    }

    #endif //GUI_JOGWHEEL_SUPPORT
    delay = gui_timers_cycle();
    if (delay < GUI_DELAY_MIN)
        delay = GUI_DELAY_MIN;
    if (delay > GUI_DELAY_MAX)
        delay = GUI_DELAY_MAX;
    #ifdef GUI_USE_RTOS
    osEvent evt = osSignalWait(GUI_SIG_REDRAW, delay);
    if ((evt.status == osEventSignal) && (evt.value.signals & GUI_SIG_REDRAW))
    #endif //GUI_USE_RTOS

        gui_redraw();
    tick = HAL_GetTick();
    if ((tick - gui_loop_tick) >= GUI_DELAY_LOOP) {
        if (gui_loop_cb)
            gui_loop_cb();
        gui_loop_tick = tick;
        Screens::Access()->ScreenEvent(0, WINDOW_EVENT_LOOP, 0);
    }
    --guiloop_nesting;

    // -- reset menu timer when we're in dialog
    if (guiloop_nesting > 0) {
        gui_reset_menu_timer();
    }
}

void gui_reset_menu_timer() {
    if (menu_timeout_enabled) {
        if (gui_get_menu_timeout_id() >= 0) {
            gui_timer_reset(gui_get_menu_timeout_id());
        } else {
            //gui_timer_create_timeout((uint32_t)MENU_TIMEOUT_MS, (int16_t)-1);
        }
    }
}

#endif //GUI_WINDOW_SUPPORT

#ifdef GUI_JOGWHEEL_SUPPORT
void gui_reset_jogwheel(void) {
    gui_jogwheel_encoder = jogwheel_encoder;
    gui_jogwheel_button_down = jogwheel_button_down;
}
#endif //GUI_JOGWHEEL_SUPPORT
