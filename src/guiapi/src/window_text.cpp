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

void window_text_t::SetAlignment(uint8_t alignm) {
    alignment = alignm;
    Invalidate();
}

window_text_t::window_text_t(window_t *parent, Rect16 rect, is_closed_on_click_t close, string_view_utf8 txt)
    : window_t(parent, rect, is_dialog_t::no, close)
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , text(txt)
    , padding(GuiDefaults::Padding)
    , alignment(GuiDefaults::Alignment) {
}

void window_text_t::unconditionalDraw() {
    render_text_align(rect, text, font,
        (IsFocused()) ? color_text : color_back,
        (IsFocused()) ? color_back : color_text,
        padding, alignment);
}

/*****************************************************************************/
//window_text_button_t
window_text_button_t::window_text_button_t(window_t *parent, Rect16 rect, ButtonCallback cb, string_view_utf8 txt)
    : window_text_t(parent, rect, is_closed_on_click_t::no, txt)
    , callback(cb) {
    Enable();
}

void window_text_button_t::windowEvent(window_t *sender, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK) {
        callback();
    } else {
        window_text_t::windowEvent(sender, event, param);
    }
}
