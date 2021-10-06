// window_text_stub.cpp
#include "window_text.hpp"

void window_text_t::SetText(string_view_utf8 txt) {
    text = txt;
    Invalidate();
}

void window_text_t::SetTextColor(color_t clr) {
    color_text = clr;
    Invalidate();
}

void window_text_t::SetPadding(padding_ui8_t padd) {
    padding = padd;
    Invalidate();
}

window_text_t::window_text_t(window_t *parent, Rect16 rect, is_multiline multiline, is_closed_on_click_t close, string_view_utf8 txt)
    : AddSuperWindow<window_aligned_t>(parent, rect, win_type_t::normal, close)
    , color_text(COLOR_BLACK)
    , font(nullptr)
    , text(txt)
    , padding({ 2, 2, 2, 2 }) {
    flags.multiline = bool(multiline);
}

void window_text_t::unconditionalDraw() {
}
