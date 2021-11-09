// screen_test_term.cpp

#include "screen_test_term.hpp"
#include "config.h"
#include "window_progress.hpp"
#include "ScreenHandler.hpp"
#include "ScreenHandler.hpp"

screen_test_term_data_t::screen_test_term_data_t()
    : AddSuperWindow<screen_t>()
    , text(this, Rect16(10, 0, 220, 22), is_multiline::no)
    , term(this, { 10, 28 }, &term_buff) {
    SetBackColor(COLOR_GRAY);

    static const char tst[] = "Test";
    text.SetText(string_view_utf8::MakeCPUFLASH((const uint8_t *)tst));
}

void screen_test_term_data_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    int winid = -1;
    if (event == GUI_event_t::BTN_DN) {
        Screens::Access()->Close();
    }
    if (event != GUI_event_t::LOOP) {
        term.Printf("%010d w:%d e:%d\n", gui::GetTick(), winid, (int)event);
    } else {
        SuperWindowEvent(sender, event, param);
    }
}
