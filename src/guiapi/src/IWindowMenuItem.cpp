#include "IWindowMenuItem.hpp"
#include "display_helper.h" //render_icon_align
#include "i18n.h"

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, expands_t expands)
    : label(label)
    , hidden(hidden)
    , enabled(enabled)
    , focused(is_focused_t::no)
    , selected(is_selected_t::no)
    , expands(expands)
    , id_icon(id_icon) {
}

void IWindowMenuItem::SetLabel(string_view_utf8 text) {
    label = text;
}

string_view_utf8 IWindowMenuItem::GetLabel() const {
    return label;
}

void IWindowMenuItem::Print(IWindowMenu &window_menu, Rect16 rect) const {
    color_t color_text = IsEnabled() ? window_menu.color_text : window_menu.color_disabled;
    color_t color_back = window_menu.color_back;
    uint8_t swap = IsEnabled() ? 0 : ROPFN_DISABLE;

    if (IsFocused()) {
        color_t swp = color_text;
        color_text = color_back;
        color_back = swp;
        swap |= ROPFN_SWAPBW;
    }

    printIcon(window_menu, rect, swap, window_menu.color_back);
    printItem(window_menu, rect, color_text, color_back, swap);
}

void IWindowMenuItem::printIcon(IWindowMenu &window_menu, Rect16 rect, uint8_t swap, color_t color_back) const {
    //do not check id
    //id == 0 wil render as black, it is needed
    render_icon_align(getIconRect(window_menu, rect), id_icon, color_back, RENDER_FLG(ALIGN_CENTER, swap));
}

void IWindowMenuItem::printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const {
    Rect16 rolling_rect = getRollingRect(window_menu, rect);
    printLabel_into_rect(rolling_rect, color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
}

void IWindowMenuItem::printLabel_into_rect(Rect16 rolling_rect, color_t color_text, color_t color_back, const font_t *font, padding_ui8_t padding, uint8_t alignment) const {
    roll.RenderTextAlign(rolling_rect, GetLabel(), font, color_back, color_text, padding, alignment);
}

void IWindowMenuItem::Click(IWindowMenu &window_menu) {
    roll.Deinit();
    window_menu.Invalidate();
    if (IsEnabled()) {
        click(window_menu);
    }
}
invalidate_t IWindowMenuItem::Roll() {
    return roll.Tick();
}

void IWindowMenuItem::SetFocus() {
    focused = is_focused_t::yes;
    //cannot call InitRollIfNeeded(window_menu, rect), rect not known (cannot add it into param)
    roll.Deinit();
}

void IWindowMenuItem::ClrFocus() {
    focused = is_focused_t::no;
    roll.Stop();
}

Rect16 IWindowMenuItem::getIconRect(IWindowMenu &window_menu, Rect16 rect) {
    return rect = Rect16::Width_t(window_menu.GetIconWidth());
}

Rect16 IWindowMenuItem::getRollingRect(IWindowMenu &window_menu, Rect16 rect) const {
    Rect16 irc = getIconRect(window_menu, rect);
    rect += Rect16::Left_t(irc.Width());
    rect -= irc.Width();
    return rect;
}

// Returns custom width Rectangle, aligned intersection on the right of the base_rect
Rect16 IWindowMenuItem::getCustomRect(IWindowMenu &window_menu, Rect16 base_rect, uint16_t custom_rect_width) {
    Rect16 custom_rect = { base_rect.Left(), base_rect.Top(), custom_rect_width, base_rect.Height() };
    custom_rect += Rect16::Left_t(base_rect.Width() - custom_rect.Width());
    return custom_rect;
}

// Reinits text rolling in case of focus/defocus/click
void IWindowMenuItem::reInitRoll(IWindowMenu &window_menu, Rect16 rect) {
    if (roll.NeedInit()) {
        roll.Init(rect, GetLabel(), window_menu.font, window_menu.padding, window_menu.GetAlignment());
    }
}
