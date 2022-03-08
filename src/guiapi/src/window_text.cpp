// window_text.cpp
#include "window_text.hpp"
#include "gui.hpp"
#include "ScreenHandler.hpp"

void window_text_t::SetText(string_view_utf8 txt) {
    if (text == txt)
        return; // prevent invalidation if texts are the same
    text = txt;
    Invalidate();
}

window_text_t::window_text_t(window_t *parent, Rect16 rect, is_multiline multiline, is_closed_on_click_t close, string_view_utf8 txt)
    : AddSuperWindow<IWindowText>(parent, rect, close)
    , text(txt) {
    flags.multiline = bool(multiline);
}

void window_text_t::unconditionalDraw() {
    if (flags.color_scheme_background || flags.color_scheme_foreground) {
        //TODO keep only following 3 lines in function body, remove rest
        super::unconditionalDraw();
        render_text_align(GetRect(), text, font, GetBackColor(), GetTextColor(),
            padding, { GetAlignment(), is_multiline(flags.multiline) });
    } else {
        render_text_align(GetRect(), text, font,
            (IsFocused()) ? GetTextColor() : GetBackColor(),
            (IsFocused()) ? GetBackColor() : GetTextColor(),
            padding, { GetAlignment(), is_multiline(flags.multiline) });
    }
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

WindowBlinkingText::WindowBlinkingText(window_t *parent, Rect16 rect, string_view_utf8 txt, uint16_t blink_step)
    : AddSuperWindow<window_text_t>(parent, rect, is_multiline::no, is_closed_on_click_t::no, txt)
    , blink_step(blink_step)
    , blink_enable(false) {
    SetPadding({ 0, 0, 0, 0 });
}

void WindowBlinkingText::unconditionalDraw() {
    // blink_enable handled in event (better invalidation)
    color_t backup_clr = GetTextColor();
    if (flags.blink0) {
        SetTextColor(color_blink);
    }

    super::unconditionalDraw();

    if (flags.blink0) {
        SetTextColor(backup_clr);
    }
}

void WindowBlinkingText::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (blink_enable && blink_step) {
        bool b = (gui::GetTick() / uint32_t(blink_step)) & 0x01;
        if (flags.blink0 != b) {
            flags.blink0 = b;
            Invalidate();
        }
    } else {
        if (flags.blink0) {
            flags.blink0 = false;
            Invalidate();
        }
    }

    SuperWindowEvent(sender, event, param);
}
