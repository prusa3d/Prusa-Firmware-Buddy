// screen_hardfault.cpp

#include "screen_hardfault.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include "sound.hpp"

screen_hardfault_data_t::screen_hardfault_data_t()
    : window_frame_t()
    , text(this, Rect16(10, 70, 220, 24), is_multiline::yes)
    , exit(this, Rect16(0, 110, 240, 24), is_multiline::no, is_closed_on_click_t::yes) {

    ClrMenuTimeoutClose();
    ClrOnSerialClose();
    SetBackColor(COLOR_RED);

    text.font = resource_font(IDR_FNT_BIG);
    static const char wdgr[] = "HARDFAULT RESET";
    text.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wdgr));
    text.SetAlignment(ALIGN_CENTER);

    exit.font = resource_font(IDR_FNT_NORMAL);
    static const char ptc[] = "press to continue...";
    exit.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ptc));
    exit.SetAlignment(ALIGN_CENTER);
}

void screen_hardfault_data_t::draw() {
    window_frame_t::draw();
    ScreenHardFault();
    Sound_Play(eSOUND_TYPE::CriticalAlert);
}

void screen_hardfault_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::CLICK:
    case GUI_event_t::ENC_DN:
    case GUI_event_t::ENC_UP:
    case GUI_event_t::CAPT_0:
    case GUI_event_t::CAPT_1:
        Sound_Stop();
    default:
        break;
    }
}
