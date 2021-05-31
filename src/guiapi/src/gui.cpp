// gui.cpp
#include "display.h"
#include "gui.hpp"
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "ScreenHandler.hpp"
#include "IDialog.hpp"
#include "Jogwheel.hpp"
#include "ScreenShot.hpp"
#include "gui_media_events.hpp"

static const constexpr uint16_t GUI_FLG_INVALID = 0x0001;

static bool gui_invalid = false;

#ifdef GUI_USE_RTOS
osThreadId gui_task_handle = 0;
#endif //GUI_USE_RTOS

font_t *GuiDefaults::Font = nullptr;
font_t *GuiDefaults::FontBig = nullptr;
font_t *GuiDefaults::FontMenuItems = nullptr;
font_t *GuiDefaults::FontMenuSpecial = nullptr;

constexpr padding_ui8_t GuiDefaults::Padding;
constexpr Rect16 GuiDefaults::RectHeader;
constexpr Rect16 GuiDefaults::RectScreenBody;
constexpr Rect16 GuiDefaults::RectScreen;
constexpr Rect16 GuiDefaults::RectScreenNoFoot;
constexpr Rect16 GuiDefaults::RectScreenNoHeader;
constexpr Rect16 GuiDefaults::RectFooter;

gui_loop_cb_t *gui_loop_cb = nullptr;
uint32_t gui_loop_tick = 0;

void gui_init(void) {
    display::Init();
    gui_task_handle = osThreadGetId();
}

void gui_redraw(void) {
    if (gui_invalid) {
        Screens::Access()->Draw();
        gui_invalid = false;
    }
}

//at least one window is invalid
void gui_invalidate(void) {
    gui_invalid = true;
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
    BtnState_t btn_ev;
    bool is_btn = jogwheel.ConsumeButtonEvent(btn_ev);
    int32_t encoder_diff = jogwheel.ConsumeEncoderDiff();

    if (encoder_diff != 0 || is_btn) {
        if (gui_loop_cb)
            gui_loop_cb();

        window_t::EventEncoder(encoder_diff);

        if (is_btn) {
            window_t::EventJogwheel(btn_ev);
        }
    }
    #endif //GUI_JOGWHEEL_SUPPORT

    MediaState_t media_state = MediaState_t::unknown;
    if (GuiMediaEventsHandler::ConsumeSent(media_state)) {
        switch (media_state) {
        case MediaState_t::inserted:
        case MediaState_t::removed:
        case MediaState_t::error:
            Screens::Access()->ScreenEvent(nullptr, GUI_event_t::MEDIA, (void *)int(media_state));
            break;
        default:
            break;
        }
    }

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
        Screens::Access()->ScreenEvent(nullptr, GUI_event_t::LOOP, 0);
    }
    --guiloop_nesting;
}

#endif //GUI_WINDOW_SUPPORT
