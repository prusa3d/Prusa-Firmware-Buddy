// screen_temperror.cpp

#include "screen_temperror.hpp"
#include "config.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include "dump.h"
#include "sound.hpp"

screen_temperror_data_t::screen_temperror_data_t()
    : window_frame_t()
    , text(this, Rect16(10, 70, 220, 24), is_multiline::yes)
    , exit(this, Rect16(0, 110, 240, 24), is_multiline::no, is_closed_on_click_t::yes) {
    SetBackColor(COLOR_RED);

    text.font = resource_font(IDR_FNT_BIG);
    static const char wdgr[] = "THERMAL ERROR";
    text.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wdgr));
    text.SetAlignment(ALIGN_CENTER);

    exit.font = resource_font(IDR_FNT_NORMAL);
    static const char ptc[] = "press to continue...";
    exit.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ptc));
    exit.SetAlignment(ALIGN_CENTER);
}

void screen_temperror_data_t::draw() {
    window_frame_t::draw();
    temp_error_code(dump_in_xflash_get_code());
    Sound_Play(eSOUND_TYPE::CriticalAlert);
}
