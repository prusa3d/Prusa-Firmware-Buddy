#include "IWindowMenuItem.hpp"
#include "display_helper.h" //render_icon_align

IWindowMenuItem::IWindowMenuItem(window_menu_t &window_menu, const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : window_menu(window_menu)
    , hidden(hidden)
    , enabled(enabled)
    , focused(false)
    , id_icon(id_icon) {
    SetLabel(label);
}

void IWindowMenuItem::SetLabel(const char *text) {
    strncpy(label.data(), text, label.size());
}

const char *IWindowMenuItem::GetLabel() const {
    return label.data();
}

void IWindowMenuItem::Print(rect_ui16_t rect) const {
    color_t color_text = IsEnabled() ? window_menu.color_text : window_menu.color_disabled;
    color_t color_back = window_menu.color_back;
    uint8_t swap = 0;

    if (IsFocused()) {
        color_t swp = color_text;
        color_text = color_back;
        color_back = swp;
        swap = ROPFN_SWAPBW;
    }

    printIcon(rect, swap);
    printText(rect, color_text, color_back, swap);
}

void IWindowMenuItem::printIcon(rect_ui16_t &rect, uint8_t swap) const {
    if (id_icon) {
        rect_ui16_t irc = { rect.x, rect.y,
            window_menu.icon_rect.w, window_menu.icon_rect.h };
        rect.x += irc.w;
        rect.w -= irc.w;
        render_icon_align(irc, id_icon,
            window_menu.color_back, RENDER_FLG(ALIGN_CENTER, swap));
    } else {
        window_menu.padding.left += window_menu.icon_rect.w;
    }
}

void IWindowMenuItem::printText(rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    render_text_align(rect, label.data(), window_menu.font,
        color_back, color_text,
        window_menu.padding, window_menu.alignment);
}
