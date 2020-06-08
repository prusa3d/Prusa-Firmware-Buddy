#include "IWindowMenuItem.hpp"
#include "display_helper.h" //render_icon_align
#include "../lang/i18n.h"

IWindowMenuItem::IWindowMenuItem(const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : hidden(hidden)
    , enabled(enabled)
    , focused(false)
    , selected(false)
    , id_icon(id_icon) {
    SetLabel(label);
}

void IWindowMenuItem::SetLabel(const char *text) {
    strncpy(label.data(), text, label.size());
}

const char *IWindowMenuItem::GetLabel() const {
    return label.data();
}

void IWindowMenuItem::Print(Iwindow_menu_t &window_menu, rect_ui16_t rect) const {
    color_t color_text = IsEnabled() ? window_menu.color_text : window_menu.color_disabled;
    color_t color_back = window_menu.color_back;
    uint8_t swap = 0;

    if (IsFocused()) {
        color_t swp = color_text;
        color_text = color_back;
        color_back = swp;
        swap = ROPFN_SWAPBW;
    }

    printIcon(window_menu, rect, swap);
    printText(window_menu, rect, color_text, color_back, swap);
}

void IWindowMenuItem::printIcon(Iwindow_menu_t &window_menu, rect_ui16_t &rect, uint8_t swap) const {
    rect_ui16_t irc = { rect.x, rect.y,
        window_menu.icon_rect.w, window_menu.icon_rect.h };
    rect.x += irc.w;
    rect.w -= irc.w;

    //do not check id
    //id == 0 wil render as black, it is needed
    render_icon_align(irc, id_icon, window_menu.color_back, RENDER_FLG(ALIGN_CENTER, swap));
}

void IWindowMenuItem::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const {
    render_text_align(rect, _(label.data()), window_menu.font,
        color_back, color_text,
        window_menu.padding, window_menu.alignment);
}

void IWindowMenuItem::Click(Iwindow_menu_t &window_menu) {
    if (IsEnabled()) {
        click(window_menu);
    }
}
