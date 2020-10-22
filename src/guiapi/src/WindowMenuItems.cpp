#include "WindowMenuItems.hpp"
#include "resource.h"
#include "ScreenHandler.hpp"

/*****************************************************************************/
//WI_LABEL_t
WI_LABEL_t::WI_LABEL_t(string_view_utf8 label, uint16_t id_icon, bool enabled, bool hidden)
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

Rect16 IWiSpin::getRollingRect(IWindowMenu &window_menu, Rect16 rect) const {
    return getRollingSpinRects(window_menu, rect)[0];
}

/**
 * returns array<Rect16,2>
 * with values of
 * {rolling_rect, spin_rect}
 **/
std::array<Rect16, 2> IWiSpin::getRollingSpinRects(IWindowMenu &window_menu, Rect16 rect) const {
    Rect16 base_rolling_rect = super::getRollingRect(window_menu, rect);
    char *buff = sn_prt();
    Rect16 spin_rect = getSpinRect(window_menu, base_rolling_rect, strlen(buff));

    Rect16 rolling_rect = base_rolling_rect;
    rolling_rect = Rect16::Width_t(spin_rect.Left() - rolling_rect.Left());
    return std::array<Rect16, 2> { rolling_rect, spin_rect };
}

//helper method - to be used by getRollingRect
Rect16 IWiSpin::getSpinRect(IWindowMenu &window_menu, Rect16 base_rolling_rect, size_t spin_strlen) {
    Rect16 spin_rect = base_rolling_rect;
    spin_rect = Rect16::Width_t(window_menu.font->w * spin_strlen + window_menu.padding.left + window_menu.padding.right);
    spin_rect += Rect16::Left_t(base_rolling_rect.Width() - spin_rect.Width());
    return spin_rect;
}

void IWiSpin::printText(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const {
    std::array<Rect16, 2> rects = getRollingSpinRects(window_menu, rect);

    //draw label
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
    //draw spin
    // this MakeRAM is safe - temp_buff is allocated for the whole life of IWiSpin
    render_text_align(rects[1], string_view_utf8::MakeRAM((const uint8_t *)temp_buff.data()), window_menu.font,
        color_back, IsSelected() ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.GetAlignment());
}

/*****************************************************************************/
//IWiSwitch
IWiSwitch::IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, bool enabled, bool hidden)
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
Rect16 IWiSwitch::getSpinRect(IWindowMenu &window_menu, Rect16 base_rolling_rect, size_t spin_strlen) const {
    Rect16 spin_rect = base_rolling_rect;
    uint16_t spin_w = window_menu.font->w * spin_strlen + window_menu.padding.left + window_menu.padding.right;
    spin_rect = Rect16::Left_t(base_rolling_rect.Left() + base_rolling_rect.Width() - spin_w);
    spin_rect = Rect16::Width_t(spin_w);
    return spin_rect;
}

/**
 * returns array<Rect16,2>
 * with values of
 * {rolling_rect, spin_rect}
 **/
std::array<Rect16, 2> IWiSwitch::getRollingSpinRects(IWindowMenu &window_menu, Rect16 rect) const {
    Rect16 base_rolling_rect = super::getRollingRect(window_menu, rect);
    string_view_utf8 localizedItem = _(get_item());
    Rect16 spin_rect = getSpinRect(window_menu, base_rolling_rect, localizedItem.computeNumUtf8CharsAndRewind());

    Rect16 rolling_rect = base_rolling_rect;
    rolling_rect = Rect16::Width_t(spin_rect.Left() - rolling_rect.Left());
    return std::array<Rect16, 2> { rolling_rect, spin_rect };
}

Rect16 IWiSwitch::getRollingRect(IWindowMenu &window_menu, Rect16 rect) const {
    return getRollingSpinRects(window_menu, rect)[0];
}

void IWiSwitch::printText(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t /*swap*/) const {
    std::array<Rect16, 2> rects = getRollingSpinRects(window_menu, rect);

    //draw label
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
    //draw spin
    render_text_align(rects[1], _(get_item()), window_menu.font,
        color_back, (IsFocused() && IsEnabled()) ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.GetAlignment());
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
    : IWindowMenuItem(string_view_utf8::MakeNULLSTR(), id_icon, enabled, hidden)
    , index(index)
    , strings(strings) {}

void WI_SELECT_t::printText(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const {
    Rect16 rolling_rect = getRollingRect(window_menu, rect);
    IWindowMenuItem::printText(window_menu, rolling_rect, color_text, color_back, swap);
    const char *txt = strings[index];

    Rect16 vrc = {
        int16_t(rect.Left() + rect.Width()), rect.Top(), uint16_t(window_menu.font->w * strlen(txt) + window_menu.padding.left + window_menu.padding.right), rect.Height()
    };
    vrc -= Rect16::Left_t(vrc.Width());
    rect -= vrc.Width();

    render_text_align(vrc, _(txt), window_menu.font,
        color_back, color_text, window_menu.padding, window_menu.GetAlignment());
}

/*****************************************************************************/
//specific WindowMenuItems

/*****************************************************************************/
//MI_RETURN
MI_RETURN::MI_RETURN()
    : WI_LABEL_t(_(label), IDR_PNG_folder_up_16px, true, false) {
}

void MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    Screens::Access()->Close();
}

MI_TEST_DISABLED_RETURN::MI_TEST_DISABLED_RETURN()
    //just for test (in debug), do not translate
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), IDR_PNG_folder_up_16px, false, false) {
}

void MI_TEST_DISABLED_RETURN::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Close();
}
