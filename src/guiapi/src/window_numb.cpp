// window_numb.cpp
#include "window_numb.hpp"
#include "gui.hpp"

// @@TODO Beware - keep this big enough as long as the SetFormat is being abused to print
// long utf8 text messages in selftest_cool.cpp (and probably in other places too)
static const constexpr uint8_t WINDOW_NUMB_MAX_TEXT = 30;

void window_numb_t::unconditionalDraw() {
    color_t clr_back = (IsFocused()) ? color_text : color_back;
    color_t clr_text = (IsFocused()) ? color_back : color_text;
    if (IsCaptured())
        clr_text = color_t::Orange;
    if (IsShadowed())
        clr_text = color_t::Gray;
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
        GetAlignment());
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

window_numb_t::window_numb_t(window_t *parent, Rect16 rect, float value)
    : window_aligned_t(parent, rect)
    , color_text(GuiDefaults::ColorText)
    , font(GuiDefaults::Font)
    , value(value)
    , format("%.0f")
    , padding(GuiDefaults::Padding) {
    PrintAsFloat();
}

void window_numb_t::PrintAsFloat() {
    flags.custom0 = false;
}

void window_numb_t::PrintAsInt() {
    flags.custom0 = true;
}

bool window_numb_t::IsPrintingAsInt() const {
    return flags.custom0;
}
