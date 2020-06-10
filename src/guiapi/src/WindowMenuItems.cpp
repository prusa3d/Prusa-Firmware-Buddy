#include "WindowMenuItems.hpp"
#include "resource.h"
#include "screen.h" //screen_close

std::array<char, 10> IWiSpin::temp_buff;

/*****************************************************************************/
//ctors
WI_LABEL_t::WI_LABEL_t(const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden) {}

WI_SELECT_t::WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(no_lbl, id_icon, enabled, hidden)
    , index(index)
    , strings(strings) {}

/*****************************************************************************/
//return changed (== invalidate)

bool WI_LABEL_t::Change(int /*dif*/) {
    return false;
}

bool WI_SELECT_t::Change(int dif) {
    size_t size = 0;
    while (strings[size] != nullptr) {
        size++;
    }

    if (dif >= 0) {
        ++index;
        if (index >= size) {
            index = 0;
        }
    } else {
        --index;
        if (index < 0) {
            index = size - 1;
        }
    }

    return true;
}

/*****************************************************************************/

void WI_SELECT_t::printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
    rect_ui16_t rolling_rect = getRollingRect(window_menu, rect);
    IWindowMenuItem::printText(window_menu, rolling_rect, color_text, color_back, swap);
    const char *txt = strings[index];

    rect_ui16_t vrc = {
        uint16_t(rect.x + rect.w), rect.y, uint16_t(window_menu.font->w * strlen(txt) + window_menu.padding.left + window_menu.padding.right), rect.h
    };
    vrc.x -= vrc.w;
    rect.w -= vrc.w;

    render_text_align(vrc, _(txt), window_menu.font,
        color_back, color_text, window_menu.padding, window_menu.alignment);
}

/*****************************************************************************/
//specific WindowMenuItems

/*****************************************************************************/
//MI_RETURN
MI_RETURN::MI_RETURN()
    : WI_LABEL_t(label, IDR_PNG_filescreen_icon_up_folder, true, false) {
}

void MI_RETURN::click(Iwindow_menu_t &window_menu) {
    screen_close();
}
