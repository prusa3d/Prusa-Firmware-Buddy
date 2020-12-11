// screen_watchdog.cpp

#include "screen_watchdog.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include "sound.hpp"

screen_watchdog_data_t::screen_watchdog_data_t()
    : AddSuperWindow<screen_reset_error_data_t>()
    , text(this, Rect16(10, 70, 220, 24), is_multiline::yes)
    , exit(this, Rect16(0, 110, 240, 24), is_multiline::no, is_closed_on_click_t::yes) {

    ClrOnSerialClose();
    ClrMenuTimeoutClose();
    SetBackColor(COLOR_RED);

    Sound_Stop();
    Sound_Play(eSOUND_TYPE::CriticalAlert);

    text.font = resource_font(IDR_FNT_BIG);
    static const char wdgr[] = "WATCHDOG RESET";
    text.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wdgr));
    text.SetAlignment(ALIGN_CENTER);

    exit.font = resource_font(IDR_FNT_NORMAL);
    static const char ptc[] = "press to continue...";
    exit.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ptc));
    exit.SetAlignment(ALIGN_CENTER);
}

void screen_watchdog_data_t::draw() {
    super::draw();
    ScreenHardFault();
}
