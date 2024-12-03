// window_text.cpp
#include "window_text.hpp"
#include "gui.hpp"
#include "ScreenHandler.hpp"

void window_text_t::SetText(const string_view_utf8 &txt) {
    if (text.is_same_ref(txt)) {
        return; // prevent invalidation if texts are the same
    }

    text = txt;
    Invalidate();
}

window_text_t::window_text_t(window_t *parent, Rect16 rect, is_multiline multiline, is_closed_on_click_t close, const string_view_utf8 &txt)
    : IWindowText(parent, rect, close)
    , text(txt) {
    flags.multiline = bool(multiline);
}

namespace {

void do_draw(Rect16 rect, const string_view_utf8 &text, Font font, Color parent_background, Color clr_text_background, Color clr_text_foreground, padding_ui8_t padding, text_flags text_flags, uint8_t rounding_rad, uint8_t rounding_flag, bool has_round_corners) {
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
    const bool invert_text_back_colors = IsFocused() && !flags.color_scheme_background && !flags.color_scheme_foreground;

    do_draw(GetRect(), text, get_font(),
        GetParent() ? GetParent()->GetBackColor() : GetBackColor(),
        invert_text_back_colors ? GetTextColor() : GetBackColor(),
        invert_text_back_colors ? GetBackColor() : GetTextColor(),
        padding, { GetAlignment(), is_multiline(flags.multiline) },
        GuiDefaults::MenuItemCornerRadius, MIC_ALL_CORNERS, flags.has_round_corners);
}

/*****************************************************************************/
// window_text_button_t
WindowButton::WindowButton(window_t *parent, Rect16 rect, ButtonCallback cb, const string_view_utf8 &txt)
    : window_text_t(parent, rect, is_multiline::no, is_closed_on_click_t::no, txt)
    , callback(cb) {
    Enable();
}

void WindowButton::set_icon(const img::Resource *set) {
    if (icon_ == set) {
        return;
    }

    icon_ = set;
    if (set) {
        text = {};
    }
    Invalidate();
}

void WindowButton::SetText(const string_view_utf8 &txt) {
    set_icon(nullptr);
    window_text_t::SetText(txt);
}

void WindowButton::unconditionalDraw() {
    if (icon_) {
        window_aligned_t::unconditionalDraw(); // background
        window_icon_t::unconditional_draw(this, icon_);

    } else {
        window_text_t::unconditionalDraw();
    }
}

void WindowButton::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::CLICK:
    case GUI_event_t::TOUCH_CLICK:
        callback(*this);
        break;

    default:
        window_text_t::windowEvent(sender, event, param);
        break;
    }
}

WindowBlinkingText::WindowBlinkingText(window_t *parent, Rect16 rect, const string_view_utf8 &txt, uint16_t blink_step)
    : window_text_t(parent, rect, is_multiline::no, is_closed_on_click_t::no, txt)
    , blink_step(blink_step)
    , blink_enable(false) {
    SetPadding({ 0, 0, 0, 0 });
}

void WindowBlinkingText::unconditionalDraw() {
    // blink_enable handled in event (better invalidation)
    Color backup_clr = GetTextColor();
    if (blink_phase) {
        SetTextColor(color_blink);
    }

    window_text_t::unconditionalDraw();

    if (blink_phase) {
        SetTextColor(backup_clr);
    }
}

void WindowBlinkingText::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    const uint8_t prev_blink_phase = blink_phase;
    if (blink_enable && blink_step) {
        blink_phase = (gui::GetTick() / uint32_t(blink_step)) & 0b1;
    } else {
        blink_phase = 0;
    }

    if (blink_phase != prev_blink_phase) {
        Invalidate();
    }

    window_text_t::windowEvent(sender, event, param);
}
