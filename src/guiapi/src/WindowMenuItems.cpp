#include "WindowMenuItems.hpp"
#include "resource.h"
#include "screen.h" //screen_close

/*****************************************************************************/
//WI_LABEL_t
WI_LABEL_t::WI_LABEL_t(const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(label, id_icon, enabled, hidden) {}

/**
 * return changed (== invalidate)
 **/
bool WI_LABEL_t::Change(int /*dif*/) {
    return false;
}

/*****************************************************************************/
//IWiSpin
std::array<char, 10> IWiSpin::temp_buff;

void IWiSpin::click(IWindowMenu & /*window_menu*/) {
    if (selected) {
        OnClick();
    }
    selected = !selected;
}

rect_ui16_t IWiSpin::getRollingRect(IWindowMenu &window_menu, rect_ui16_t rect) const {
    return getRollingSpinRects(window_menu, rect)[0];
}

/**
 * returns array<rect_ui16_t,2>
 * with values of
 * {rolling_rect, spin_rect}
 **/
std::array<rect_ui16_t, 2> IWiSpin::getRollingSpinRects(IWindowMenu &window_menu, rect_ui16_t rect) const {
    rect_ui16_t base_rolling_rect = super::getRollingRect(window_menu, rect);
    char *buff = sn_prt();
    rect_ui16_t spin_rect = getSpinRect(window_menu, base_rolling_rect, strlen(buff));

    rect_ui16_t rolling_rect = base_rolling_rect;
    rolling_rect.w = spin_rect.x - rolling_rect.x;
    return std::array<rect_ui16_t, 2> { rolling_rect, spin_rect };
}

//helper method - to be used by getRollingRect
rect_ui16_t IWiSpin::getSpinRect(IWindowMenu &window_menu, rect_ui16_t base_rolling_rect, size_t spin_strlen) {
    rect_ui16_t spin_rect = {
        uint16_t(base_rolling_rect.x + base_rolling_rect.w), base_rolling_rect.y, uint16_t(window_menu.font->w * spin_strlen + window_menu.padding.left + window_menu.padding.right), base_rolling_rect.h
    };
    spin_rect.x -= spin_rect.w;
    return spin_rect;
}

void IWiSpin::printText(IWindowMenu &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const {
    std::array<rect_ui16_t, 2> rects = getRollingSpinRects(window_menu, rect);

    //draw label
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.alignment);
    //draw spin
    // this MakeRAM is safe - temp_buff is allocated for the whole life of IWiSpin
    render_text_align(rects[1], string_view_utf8::MakeRAM((const uint8_t *)temp_buff.data()), window_menu.font,
        color_back, IsSelected() ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.alignment);
}

/*****************************************************************************/
//IWiSwitch
IWiSwitch::IWiSwitch(int32_t index, const char *label, uint16_t id_icon, bool enabled, bool hidden)
    : AddSuper<IWindowMenuItem>(label, id_icon, enabled, hidden)
    , index(index) {}

bool IWiSwitch::Change(int /*dif*/) {
    if ((++index) >= size()) {
        index = 0;
    }
    return true;
}

void IWiSwitch::click(IWindowMenu & /*window_menu*/) {
    size_t old_index = index;
    Change(0);
    OnChange(old_index);
}

bool IWiSwitch::SetIndex(size_t idx) {
    if (idx >= size())
        return false;
    else {
        index = idx;
        return true;
    }
}

//helper method - to be used by getRollingRect
rect_ui16_t IWiSwitch::getSpinRect(IWindowMenu &window_menu, rect_ui16_t base_rolling_rect, size_t spin_strlen) const {
    rect_ui16_t spin_rect = base_rolling_rect;
    uint16_t spin_w = window_menu.font->w * spin_strlen + window_menu.padding.left + window_menu.padding.right;
    spin_rect.x = base_rolling_rect.x + base_rolling_rect.w - spin_w;
    spin_rect.w = spin_w;
    return spin_rect;
}

/**
 * returns array<rect_ui16_t,2>
 * with values of
 * {rolling_rect, spin_rect}
 **/
std::array<rect_ui16_t, 2> IWiSwitch::getRollingSpinRects(IWindowMenu &window_menu, rect_ui16_t rect) const {
    rect_ui16_t base_rolling_rect = super::getRollingRect(window_menu, rect);
    rect_ui16_t spin_rect = getSpinRect(window_menu, base_rolling_rect, strlen(get_item()));

    rect_ui16_t rolling_rect = base_rolling_rect;
    rolling_rect.w = spin_rect.x - rolling_rect.x;
    return std::array<rect_ui16_t, 2> { rolling_rect, spin_rect };
}

rect_ui16_t IWiSwitch::getRollingRect(IWindowMenu &window_menu, rect_ui16_t rect) const {
    return getRollingSpinRects(window_menu, rect)[0];
}

void IWiSwitch::printText(IWindowMenu &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const {
    std::array<rect_ui16_t, 2> rects = getRollingSpinRects(window_menu, rect);

    //draw label
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.alignment);
    //draw spin
    render_text_align(rects[1], _(get_item()), window_menu.font,
        color_back, (IsFocused() && IsEnabled()) ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.alignment);
}

/*****************************************************************************/
//WI_SELECT_t
/**
 * return changed (== invalidate)
 **/
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

WI_SELECT_t::WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, bool enabled, bool hidden)
    : IWindowMenuItem(no_lbl, id_icon, enabled, hidden)
    , index(index)
    , strings(strings) {}

void WI_SELECT_t::printText(IWindowMenu &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const {
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

void MI_RETURN::click(IWindowMenu & /*window_menu*/) {
    screen_close();
}

MI_TEST_DISABLED_RETURN::MI_TEST_DISABLED_RETURN()
    : WI_LABEL_t(label, IDR_PNG_filescreen_icon_up_folder, false, false) {
}

void MI_TEST_DISABLED_RETURN::click(IWindowMenu & /*window_menu*/) {
    screen_close();
}
