// window_text.cpp
#include "window_text.hpp"
#include "gui.hpp"
#include "ScreenHandler.hpp"

void window_text_t::SetText(string_view_utf8 txt) {
    if (text.is_same_ref(txt)) {
        return; // prevent invalidation if texts are the same
    }

    text = txt;
    Invalidate();
}

window_text_t::window_text_t(window_t *parent, Rect16 rect, is_multiline multiline, is_closed_on_click_t close, string_view_utf8 txt)
    : AddSuperWindow<IWindowText>(parent, rect, close)
    , text(txt) {
    flags.multiline = bool(multiline);
}

namespace {

void do_draw(Rect16 rect, string_view_utf8 text, const font_t *font, color_t parent_background, color_t clr_text_background, color_t clr_text_foreground, padding_ui8_t padding, text_flags text_flags, uint8_t rounding_rad, uint8_t rounding_flag, bool has_round_corners) {
    if (has_round_corners) {
        render_rounded_rect(rect, parent_background, clr_text_background, rounding_rad, rounding_flag);

        rect = Rect16::Width_t { static_cast<uint16_t>(rect.Width() - rounding_rad * 2) };
        rect = Rect16::X_t { static_cast<int16_t>(rect.Left() + rounding_rad) };
    }

    render_text_align(rect, text, font,
        clr_text_background,
        clr_text_foreground,
        padding, text_flags);
}
} // namespace

void window_text_t::unconditionalDraw() {
    if (flags.color_scheme_background || flags.color_scheme_foreground) {
        do_draw(GetRect(), text, get_font(),
            GetParent() ? GetParent()->GetBackColor() : GetBackColor(),
            GetBackColor(),
            GetTextColor(),
            padding, { GetAlignment(), is_multiline(flags.multiline) },
            GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS, flags.has_round_corners);
    } else {
        do_draw(GetRect(), text, get_font(),
            GetParent() ? GetParent()->GetBackColor() : GetBackColor(),
            IsFocused() ? GetTextColor() : GetBackColor(),
            IsFocused() ? GetBackColor() : GetTextColor(),
            padding, { GetAlignment(), is_multiline(flags.multiline) },
            GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS, flags.has_round_corners);
    }
}

/*****************************************************************************/
// window_text_button_t
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
