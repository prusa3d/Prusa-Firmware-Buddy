// window_text.hpp

#pragma once

#include "i_window_text.hpp"
#include "font_flags.hpp" // is_multiline
#include "../../lang/string_view_utf8.hpp"

class window_text_t : public AddSuperWindow<IWindowText> {

public:
    window_text_t() = default;
    window_text_t(window_t *parent, Rect16 rect, is_multiline multiline, is_closed_on_click_t close = is_closed_on_click_t::no, const string_view_utf8 &txt = string_view_utf8::MakeNULLSTR());

public:
    string_view_utf8 GetText() const { return text; }
    virtual void SetText(string_view_utf8 txt);

    void set_is_multiline(bool set) {
        flags.multiline = set;
    }

protected:
    virtual void unconditionalDraw() override;

protected:
    string_view_utf8 text;
};

class WindowButton : public window_text_t {

public:
    WindowButton() = default;
    WindowButton(window_t *parent, Rect16 rect, ButtonCallback cb, const string_view_utf8 &txt = string_view_utf8::MakeNULLSTR()); // default action is close screen

public:
    ButtonCallback callback;

public:
    const img::Resource *icon() const {
        return icon_;
    }
    void set_icon(const img::Resource *set);

    virtual void SetText(string_view_utf8 txt) override;

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

    void unconditionalDraw() override;

protected:
    /// If set, icon is rendered instead of text
    const img::Resource *icon_ = nullptr;
};

using window_text_button_t = WindowButton;

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

    WindowBlinkingText(window_t *parent, Rect16 rect, const string_view_utf8 &txt = string_view_utf8::MakeNULLSTR(), uint16_t blink_step = 500);

protected:
    virtual void unconditionalDraw() override;
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
