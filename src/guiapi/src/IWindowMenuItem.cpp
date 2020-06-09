#include "IWindowMenuItem.hpp"
#include "display_helper.h" //render_icon_align

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
    if (focused && roll.setup == TXTROLL_SETUP_DONE) { //draw normally on TXTROLL_SETUP_INIT or TXTROLL_SETUP_IDLE
        render_roll_text_align(rect, label.data(), window_menu.font, window_menu.padding, window_menu.alignment, color_back, color_text, &roll);
    } else {
        render_text_align(rect, label.data(), window_menu.font, color_back, color_text, window_menu.padding, window_menu.alignment);
    }
}

void IWindowMenuItem::Click(Iwindow_menu_t &window_menu) {
    window_menu.win.f_invalid = 1;
    if (IsEnabled()) {
        click(window_menu);
    }
}

void IWindowMenuItem::RollInit(Iwindow_menu_t &window_menu, rect_ui16_t rect) {
    roll_init(rect, label.data(), window_menu.font, window_menu.padding, window_menu.alignment, &roll);
}
void IWindowMenuItem::Roll(Iwindow_menu_t &window_menu) {
    roll_text_phasing(window_menu.win.id, window_menu.font, &roll); //warning it is accessing gui timer
}

void IWindowMenuItem::SetFocus() {
    focused = true;
    roll.setup = TXTROLL_SETUP_INIT;
}
