// window_numb.cpp
#include "window_numb.hpp"
#include "gui.hpp"

#define WINDOW_NUMB_MAX_TEXT 16

void window_numb_t::unconditionalDraw() {
    color_t clr_back = (IsFocused()) ? color_text : color_back;
    color_t clr_text = (IsFocused()) ? color_back : color_text;
    if (IsCaptured())
        clr_text = COLOR_ORANGE;
    char text[WINDOW_NUMB_MAX_TEXT];
    if (IsPrintingAsInt()) {
        snprintf(text, WINDOW_NUMB_MAX_TEXT, format, (int)(value));
    } else {
        snprintf(text, WINDOW_NUMB_MAX_TEXT, format, (double)value);
    }

    render_text_align(rect,
        // this MakeRAM is safe - render_text finishes its work and the local string text[] is then no longer needed
        string_view_utf8::MakeRAM((const uint8_t *)text),
        font,
        clr_back,
        clr_text,
        padding,
        alignment);
}

void window_numb_t::SetFormat(const char *frmt) {
    format = frmt;
    Invalidate();
}

void window_numb_t::SetValue(float val) {
    setValue(val);
    Invalidate();
}

void window_numb_t::setValue(float val) {
    value = val;
}

void window_numb_t::SetFont(font_t *val) {
    font = val;
    Invalidate();
}

void window_numb_t::SetColor(color_t clr) {
    if (clr != color_text) {
        color_text = clr;
        Invalidate();
    }
}

window_numb_t::window_numb_t(window_t *parent, rect_ui16_t rect, float value)
    : window_t(parent, rect)
    , color_text(gui_defaults.color_text)
    , font(gui_defaults.font)
    , value(value)
    , format("%.0f")
    , padding(gui_defaults.padding)
    , alignment(gui_defaults.alignment) {
    PrintAsFloat();
}

void window_numb_t::PrintAsFloat() {
    f_parent_defined0 = false;
}

void window_numb_t::PrintAsInt() {
    f_parent_defined0 = true;
}

bool window_numb_t::IsPrintingAsInt() const {
    return f_parent_defined0;
}
