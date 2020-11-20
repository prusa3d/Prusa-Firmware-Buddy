// window_text.hpp

#pragma once

#include "window.hpp"

enum class is_multiline : bool { no,
    yes };

struct window_text_t : public AddSuperWindow<window_aligned_t> {
    color_t color_text;
    font_t *font;
    string_view_utf8 text;
    padding_ui8_t padding;

    string_view_utf8 GetText() const { return text; }
    virtual void SetText(string_view_utf8 txt);
    void SetTextColor(color_t clr);

    color_t GetTextColor() const { return color_text; }
    void SetPadding(padding_ui8_t padd);

    window_text_t(window_t *parent, Rect16 rect, is_multiline multiline, is_closed_on_click_t close = is_closed_on_click_t::no, string_view_utf8 txt = string_view_utf8::MakeNULLSTR());

protected:
    virtual void unconditionalDraw() override;
};

struct window_text_button_t : public AddSuperWindow<window_text_t> {
    ButtonCallback callback;

    window_text_button_t(window_t *parent, Rect16 rect, ButtonCallback cb, string_view_utf8 txt = string_view_utf8::MakeNULLSTR()); //default action is close screen

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
