// screen_test_term.cpp

#include "screen_test_term.hpp"
#include "config.h"
#include "window_progress.hpp"
#include "ScreenHandler.hpp"
#include "stm32f4xx_hal.h"
#include "ScreenHandler.hpp"

screen_test_term_data_t::screen_test_term_data_t()
    : window_frame_t()
    , text(this, Rect16(10, 0, 220, 22), is_multiline::no)
    , term(this, { 10, 28 }, &term_buff) {
    SetBackColor(COLOR_GRAY);

    static const char tst[] = "Test";
    text.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tst));
}

void screen_test_term_data_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    int winid = -1;
    if (event == WINDOW_EVENT_BTN_DN) {
        Screens::Access()->Close();
    }
    if (event != WINDOW_EVENT_LOOP) {
        term.Printf("%010d w:%d e:%d\n", HAL_GetTick(), winid, (int)event);
    } else {
        window_frame_t::windowEvent(sender, event, param);
    }
}
