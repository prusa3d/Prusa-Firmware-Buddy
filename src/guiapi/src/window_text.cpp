// window_text.cpp
#include "window_text.hpp"
#include "gui.hpp"
#include "ScreenHandler.hpp"

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
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , text(txt)
    , padding(GuiDefaults::Padding) {
    flags.custom0 = bool(multiline);
}

void window_text_t::unconditionalDraw() {
    render_text_align(rect, text, font,
        (IsFocused()) ? color_text : color_back,
        (IsFocused()) ? color_back : color_text,
        padding, flags.custom0 ? GetAlignment() | RENDER_FLG_WORDB : GetAlignment());
}

/*****************************************************************************/
//window_text_button_t
window_text_button_t::window_text_button_t(window_t *parent, Rect16 rect, ButtonCallback cb, string_view_utf8 txt)
    : AddSuperWindow<window_text_t>(parent, rect, is_multiline::no, is_closed_on_click_t::no, txt)
    , callback(cb) {
    Enable();
}

void window_text_button_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CLICK) {
        callback();
    } else {
        SuperWindowEvent(sender, event, param);
    }
}
