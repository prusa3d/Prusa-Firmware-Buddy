// screen_watchdog.cpp

#include "screen_watchdog.hpp"
#include "config.h"
#include "ScreenHandler.hpp"

screen_watchdog_data_t::screen_watchdog_data_t()
    : window_frame_t(&text0)
    , text0(this, rect_ui16(10, 70, 220, 24))
    , text1(this, rect_ui16(0, 110, 240, 24)) {
    SetBackColor(COLOR_RED);

    text0.font = resource_font(IDR_FNT_BIG);
    static const char wdgr[] = "WATCHDOG RESET";
    text0.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)wdgr));
    text0.SetAlignment(ALIGN_CENTER);

    text1.font = resource_font(IDR_FNT_NORMAL);
    static const char ptc[] = "press to continue...";
    text1.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)ptc));
    text1.SetAlignment(ALIGN_CENTER);
    text1.Enable();
    text1.SetTag(1);
}

void screen_watchdog_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK || event == WINDOW_EVENT_BTN_DN)
        Screens::Access()->Close();
}
