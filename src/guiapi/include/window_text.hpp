// window_text.hpp

#pragma once

#include "i_window_text.hpp"
#include "font_flags.hpp" // is_multiline
#include "../../lang/string_view_utf8.hpp"

class window_text_t : public AddSuperWindow<IWindowText> {
public:
    string_view_utf8 text;

    string_view_utf8 GetText() const { return text; }
    virtual void SetText(string_view_utf8 txt);

    window_text_t(window_t *parent, Rect16 rect, is_multiline multiline, is_closed_on_click_t close = is_closed_on_click_t::no, string_view_utf8 txt = string_view_utf8::MakeNULLSTR());

protected:
    virtual void unconditionalDraw() override;
};

struct window_text_button_t : public AddSuperWindow<window_text_t> {
    ButtonCallback callback;

    window_text_button_t(window_t *parent, Rect16 rect, ButtonCallback cb, string_view_utf8 txt = string_view_utf8::MakeNULLSTR()); // default action is close screen

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

class WindowBlinkingText : public AddSuperWindow<window_text_t> {
    color_t color_blink;
    uint16_t blink_step;
    bool blink_enable;

public:
    void SetBlinkColor(color_t clr) {
        color_blink = clr;
        color_blink == GetTextColor() ? DisableBlink() : EnableBlink();
    }
    constexpr color_t GetBlinkColor() const { return color_blink; }
    constexpr void EnableBlink() { blink_enable = true; }
    constexpr void DisableBlink() { blink_enable = false; }

    WindowBlinkingText(window_t *parent, Rect16 rect, string_view_utf8 txt = string_view_utf8::MakeNULLSTR(), uint16_t blink_step = 500);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
