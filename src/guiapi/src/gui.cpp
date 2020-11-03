// gui.c
#include "display.h"
#include "gui.hpp"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "sound.hpp"
#include "ScreenHandler.hpp"
#include "IDialog.hpp"
#include "Jogwheel.hpp"
#include "ScreenShot.hpp"

static const constexpr uint16_t GUI_FLG_INVALID = 0x0001;

uint16_t gui_flags = 0;

#ifdef GUI_USE_RTOS
osThreadId gui_task_handle = 0;
#endif //GUI_USE_RTOS

font_t *GuiDefaults::Font = 0;
font_t *GuiDefaults::FontBig = 0;
font_t *GuiDefaults::FontSpecial = 0;

// bool GuiDefaults::menu_timeout_enabled = true;

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

static const constexpr uint8_t GUI_DELAY_MIN = 1;
static const constexpr uint8_t GUI_DELAY_MAX = 10;
static const constexpr uint8_t GUI_DELAY_LOOP = 100;

#ifdef GUI_WINDOW_SUPPORT

static uint8_t guiloop_nesting = 0;
uint8_t gui_get_nesting(void) { return guiloop_nesting; }

void gui_loop(void) {
    ++guiloop_nesting;
    uint32_t delay;
    uint32_t tick;

    #ifdef GUI_JOGWHEEL_SUPPORT
    Jogwheel::BtnState_t btn_ev;
    bool is_btn = jogwheel.ConsumeButtonEvent(btn_ev);
    int32_t encoder_diff = jogwheel.ConsumeEncoderDiff();

    if (encoder_diff != 0 || is_btn) {
        if (gui_loop_cb)
            gui_loop_cb();
        window_t *capturedWin = window_t::GetCapturedWindow();

        if (encoder_diff != 0) {
            if (encoder_diff > 0) {
                capturedWin->WindowEvent(capturedWin, GUI_event_t::ENC_UP, (void *)encoder_diff);
            } else {
                capturedWin->WindowEvent(capturedWin, GUI_event_t::ENC_DN, (void *)-encoder_diff);
            }
        }

        if (is_btn) {
            switch (btn_ev) {
            case Jogwheel::BtnState_t::Pressed:
                capturedWin->WindowEvent(capturedWin, GUI_event_t::BTN_DN, 0);
                break;
            case Jogwheel::BtnState_t::Released:
                Sound_Play(eSOUND_TYPE::ButtonEcho);
                capturedWin->WindowEvent(capturedWin, GUI_event_t::BTN_UP, 0);
                capturedWin->WindowEvent(capturedWin, GUI_event_t::CLICK, 0);
                break;
            case Jogwheel::BtnState_t::Held:
                Sound_Play(eSOUND_TYPE::ButtonEcho);
                capturedWin->WindowEvent(capturedWin, GUI_event_t::HOLD, 0);
                break;
            }
        }

        Screens::Access()->ResetTimeout();
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
        Screens::Access()->ScreenEvent(0, GUI_event_t::LOOP, 0);
    }
    --guiloop_nesting;

    // -- reset menu timer when we're in dialog
    if (guiloop_nesting > 0) {
        Screens::Access()->ResetTimeout();
    }
}

#endif //GUI_WINDOW_SUPPORT
