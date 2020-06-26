#include "IWindowMenuItem.hpp"
#include "display_helper.h" //render_icon_align
#include "../lang/i18n.h"

IWindowMenuItem::IWindowMenuItem(const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : hidden(hidden)
    , enabled(enabled)
    , focused(false)
    , selected(false)
    , id_icon(id_icon)
    , roll({ 0 }) {
    SetLabel(label);
}

void IWindowMenuItem::SetLabel(const char *text) {
    strncpy(label.data(), text, label.size());
}

const char *IWindowMenuItem::GetLabel() const {
    return label.data();
}

string_view_utf8 IWindowMenuItem::GetLocalizedLabel() const {
    return _(GetLabel());
}

void IWindowMenuItem::Print(IWindowMenu &window_menu, rect_ui16_t rect) const {
    color_t color_text = IsEnabled() ? window_menu.color_text : window_menu.color_disabled;
    color_t color_back = window_menu.color_back;
    uint8_t swap = IsEnabled() ? 0 : ROPFN_DISABLE;

    if (IsFocused()) {
        color_t swp = color_text;
        color_text = color_back;
        color_back = swp;
        swap |= ROPFN_SWAPBW;
    }

    //printIcon(window_menu, rect, 0, color_back);
    printIcon(window_menu, rect, swap, window_menu.color_back);
    printText(window_menu, rect, color_text, color_back, swap);
}

void IWindowMenuItem::printIcon(IWindowMenu &window_menu, rect_ui16_t rect, uint8_t swap, color_t color_back) const {
    //do not check id
    //id == 0 wil render as black, it is needed
    render_icon_align(getIconRect(window_menu, rect), id_icon, color_back, RENDER_FLG(ALIGN_CENTER, swap));
}

void IWindowMenuItem::printText(IWindowMenu &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const {
    rect_ui16_t rolling_rect = getRollingRect(window_menu, rect);
    printLabel_into_rect(rolling_rect, color_text, color_back, window_menu.font, window_menu.padding, window_menu.alignment);
}

void IWindowMenuItem::printLabel_into_rect(rect_ui16_t rolling_rect, color_t color_text, color_t color_back, const font_t *font, padding_ui8_t padding, uint8_t alignment) const {
    if (focused && roll.setup == TXTROLL_SETUP_DONE) { //draw normally on TXTROLL_SETUP_INIT or TXTROLL_SETUP_IDLE
        render_roll_text_align(rolling_rect, GetLocalizedLabel(), font, padding, alignment, color_back, color_text, &roll);
    } else {
        render_text_align(rolling_rect, GetLocalizedLabel(), font, color_back, color_text, padding, alignment);
    }
}

void IWindowMenuItem::Click(IWindowMenu &window_menu) {
    window_menu.f_invalid = 1;
    if (IsEnabled()) {
        click(window_menu);
    }
}

void IWindowMenuItem::RollInit(IWindowMenu &window_menu, rect_ui16_t rect) {
    roll_init(getRollingRect(window_menu, rect), GetLocalizedLabel(), window_menu.font, window_menu.padding, window_menu.alignment, &roll);
}
void IWindowMenuItem::Roll(IWindowMenu &window_menu) {
    roll_text_phasing(window_menu.id, window_menu.font, &roll); //warning it is accessing gui timer
}

void IWindowMenuItem::SetFocus() {
    focused = true;
    roll.setup = TXTROLL_SETUP_INIT;
}

rect_ui16_t IWindowMenuItem::getIconRect(IWindowMenu &window_menu, rect_ui16_t rect) {
    return { rect.x, rect.y,
        window_menu.icon_rect.w, window_menu.icon_rect.h };
}

rect_ui16_t IWindowMenuItem::getRollingRect(IWindowMenu &window_menu, rect_ui16_t rect) const {
    rect_ui16_t irc = getIconRect(window_menu, rect);
    rect.x += irc.w;
    rect.w -= irc.w;
    return rect;
}
