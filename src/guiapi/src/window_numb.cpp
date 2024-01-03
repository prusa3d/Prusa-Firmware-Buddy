// window_numb.cpp
#include "window_numb.hpp"
#include "time_helper.hpp"
#include "gui.hpp"
#include <ctime>

// @@TODO Beware - keep this big enough as long as the SetFormat is being abused to print
// long utf8 text messages in selftest_cool.cpp (and probably in other places too)
static const constexpr uint8_t WINDOW_NUMB_MAX_TEXT = 30;

static_assert(sizeof(uint32_t) == sizeof(float), "size of uint32 does not match float");

void window_numb_t::unconditionalDraw() {
    color_t clr_back = GetBackColor();
    color_t clr_text = GetTextColor();
    // TODO remove this if statement (its body too)
    if (!(flags.color_scheme_background || flags.color_scheme_foreground)) {
        clr_back = (IsFocused()) ? GetTextColor() : GetBackColor();
        clr_text = (IsFocused()) ? GetBackColor() : GetTextColor();
        if (IsShadowed()) {
            clr_text = COLOR_GRAY;
        }
    }

    if (IsCaptured()) { // capture color could be part of color scheme, but currently it is used only here
        clr_text = COLOR_ORANGE;
    }

    char text[WINDOW_NUMB_MAX_TEXT];
    switch (printAs) {
    case printType::asInt32:
        snprintf(text, WINDOW_NUMB_MAX_TEXT, format, (int32_t)(value));
        break;
    case printType::asFloat:
        snprintf(text, WINDOW_NUMB_MAX_TEXT, format, (double)value);
        break;
    case printType::asUint32:
        snprintf(text, WINDOW_NUMB_MAX_TEXT, format, (uint32_t)value);
        break;
    case printType::asTime:
        PrintTime(text);
        break;
    }

    render_text_align(GetRect(),
        // this MakeRAM is safe - render_text finishes its work and the local string text[] is then no longer needed
        string_view_utf8::MakeRAM((const uint8_t *)text),
        get_font(),
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

window_numb_t::window_numb_t(window_t *parent, Rect16 rect, float value, const char *frmt, Font font)
    : AddSuperWindow<IWindowText>(parent, rect)
    , value(value)
    , format(frmt == nullptr ? "%.0f" : frmt) {
    PrintAsFloat();
    set_font(font);
}

void window_numb_t::PrintAsFloat() {
    printAs = printType::asFloat;
}

void window_numb_t::PrintAsInt32() {
    printAs = printType::asInt32;
}

void window_numb_t::PrintAsUint32() {
    printAs = printType::asUint32;
}
bool window_numb_t::IsPrintingAsInt() const {
    return printAs == printType::asInt32;
}
void window_numb_t::PrintAsTime() {
    printAs = printType::asTime;
}

void window_numb_t::PrintTime(char *buffer) {
    format_duration(std::span<char> { buffer, WINDOW_NUMB_MAX_TEXT }, value);
}
