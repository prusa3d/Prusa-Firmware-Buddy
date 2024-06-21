// window_text_stub.cpp
#include "window_text.hpp"

void window_text_t::SetText(const string_view_utf8 &txt) {
    text = txt;
    Invalidate();
}

window_text_t::window_text_t(window_t *parent, Rect16 rect, is_multiline multiline, is_closed_on_click_t close, const string_view_utf8 &txt)
    : IWindowText(parent, rect, close)
    , text(txt) {
    flags.multiline = bool(multiline);
}

void window_text_t::unconditionalDraw() {
}
