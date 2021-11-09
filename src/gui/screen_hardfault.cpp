// screen_hardfault.cpp

#include "screen_hardfault.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include "sound.hpp"

screen_hardfault_data_t::screen_hardfault_data_t()
    : AddSuperWindow<screen_reset_error_data_t>()
    , text(this, Rect16(10, 70, 220, 24), is_multiline::yes)
    , exit(this, Rect16(0, 110, 240, 24), is_multiline::no, is_closed_on_click_t::yes) {

    ClrMenuTimeoutClose();
    ClrOnSerialClose();
    SetBackColor(COLOR_RED);

    text.font = resource_font(IDR_FNT_BIG);
    static const char wdgr[] = "HARDFAULT RESET";
    text.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wdgr));
    text.SetAlignment(Align_t::Center());

    exit.font = resource_font(IDR_FNT_NORMAL);
    static const char ptc[] = "press to continue...";
    exit.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ptc));
    exit.SetAlignment(Align_t::Center());
}

void screen_hardfault_data_t::draw() {
    super::draw();
    ScreenHardFault();
}

/*
//TODO check if above works, if not, use this
void screen_hardfault_data_t::draw() {
    screen_t::draw();
    ScreenHardFault();
    start_sound();
}
*/
