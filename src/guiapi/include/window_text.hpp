// window_text.hpp

#pragma once

#include "window.hpp"

struct window_class_text_t {
    window_class_t cls;
};

struct window_text_t : public window_t {
    color_t color_text;
    font_t *font;
    string_view_utf8 text;
    padding_ui8_t padding;
    uint8_t alignment;

    string_view_utf8 GetText() const { return text; }
    void SetText(string_view_utf8 txt);
    void SetTextColor(color_t clr);

    color_t GetTextColor() const { return color_text; }
    void SetPadding(padding_ui8_t padd);
    void SetAlignment(uint8_t alignm);
};

extern const window_class_text_t window_class_text;
