#include "WindowMenuItems.hpp"
#include "resource.h"
#include "ScreenHandler.hpp"

/*****************************************************************************/
//WI_LABEL_t
WI_LABEL_t::WI_LABEL_t(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, expands_t expands_screen)
    : IWindowMenuItem(label, id_icon, enabled, hidden, expands_screen) {}

/**
 * return changed (== invalidate)
 **/
bool WI_LABEL_t::Change(int /*dif*/) {
    return false;
}

void WI_LABEL_t::printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const {
    std::array<Rect16, 2> rects = getMenuRects(window_menu, rect);
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
    if (expands == expands_t::yes) {
        render_icon_align(rects[1], IDR_PNG_arrow_right_16px, window_menu.color_back, RENDER_FLG(ALIGN_LEFT_CENTER, swap));
    }
}

std::array<Rect16, 2> WI_LABEL_t::getMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
    Rect16 rolling_rect = getRollingRect(window_menu, rect);
    Rect16 expand_icon_rc = { 0, 0, 0, 0 };
    if (expands == expands_t::yes) {
        expand_icon_rc = { int16_t(rect.Left() + rect.Width() - 26), rect.Top(), 26, rect.Height() };
        if (rolling_rect.Left() + rolling_rect.Width() > expand_icon_rc.Left()) {
            rolling_rect -= Rect16::Width_t(rolling_rect.Left() + rolling_rect.Width() - expand_icon_rc.Left());
        }
    }
    return std::array<Rect16, 2> { rolling_rect, expand_icon_rc };
}

void WI_LABEL_t::InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) {
    reInitRoll(window_menu, getMenuRects(window_menu, rect)[0]);
}

/*****************************************************************************/
//IWiSpin
std::array<char, 10> IWiSpin::temp_buff;

void IWiSpin::click(IWindowMenu & /*window_menu*/) {
    if (selected == is_selected_t::yes) {
        OnClick();
    }
    selected = selected == is_selected_t::yes ? is_selected_t::no : is_selected_t::yes;
}

/**
 * returns array<Rect16,2>
 * with values of
 * {rolling_rect, spin_rect}
 **/
std::array<Rect16, 2> IWiSpin::getMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
    Rect16 base_rolling_rect = getRollingRect(window_menu, rect);
    char *buff = sn_prt();
    Rect16 spin_rect = getCustomRect(window_menu, base_rolling_rect, strlen(buff) * window_menu.font->w + window_menu.padding.left + window_menu.padding.right);

    Rect16 rolling_rect = base_rolling_rect;
    rolling_rect = Rect16::Width_t(spin_rect.Left() - rolling_rect.Left());
    return std::array<Rect16, 2> { rolling_rect, spin_rect };
}

void IWiSpin::InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) {
    reInitRoll(window_menu, getSpinMenuRects(window_menu, rect)[0]);
}

std::array<Rect16, 3> IWiSpin::getSpinMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
    Rect16 base_rect = getRollingRect(window_menu, rect);
    Rect16 unit_rect = { int16_t(base_rect.Left() + base_rect.Width() - UNIT_RECT_CHAR_WIDTH * window_menu.secondary_font->w), base_rect.Top(), uint16_t(UNIT_RECT_CHAR_WIDTH * window_menu.secondary_font->w), base_rect.Height() };
    base_rect -= unit_rect.Width();
    char *buff = sn_prt();
    Rect16 spin_rect = getCustomRect(window_menu, base_rect, strlen(buff) * window_menu.secondary_font->w);
    base_rect -= spin_rect.Width();

    return std::array<Rect16, 3> { base_rect, spin_rect, unit_rect };
}

void IWiSpin::printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const {
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    std::array<Rect16, 2> rects = IWiSpin::getMenuRects(window_menu, rect); // MINI GUI implementation
#else                                                                       // PRINTER TYPE != PRUSA_PRINTER_MINI
    std::array<Rect16, 3> rects = getSpinMenuRects(window_menu, rect);
#endif                                                                      // PRINTER TYPE == PRUSA_PRINTER_MINI
    //draw label
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
    //draw spin
    // this MakeRAM is safe - temp_buff is allocated for the whole life of IWiSpin
    render_text_align(rects[1], string_view_utf8::MakeRAM((const uint8_t *)temp_buff.data()),
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        window_menu.font,
        color_back, IsSelected() ? COLOR_ORANGE : color_text,
        window_menu.padding,
#else  // PRINTER TYPE != PRUSA_PRINTER_MINI
        window_menu.secondary_font,
        color_back, IsSelected() ? COLOR_ORANGE : color_text,
        padding_ui8(0, 6, 0, 0),
#endif // PRINTER TYPE == PRUSA_PRINTER_MINI
        window_menu.GetAlignment());

#if (PRINTER_TYPE != PRINTER_PRUSA_MINI)
    uint8_t unit_padding_left = units[0] == '\177' ? 0 : UNIT_HALFSPACE_PADDING; // °C are not separated with halfspace
    render_text_align(rects[2], string_view_utf8::MakeRAM((const uint8_t *)units), window_menu.secondary_font, color_back,
        COLOR_SILVER, padding_ui8(unit_padding_left, 6, 0, 0), window_menu.GetAlignment());
#endif // (PRINTER_TYPE != PRINTER_PRUSA_MINI
}

/*****************************************************************************/
//IWiSwitch
IWiSwitch::IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
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

std::array<Rect16, 2> IWiSwitch::getMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
    Rect16 base_rolling_rect = getRollingRect(window_menu, rect);
    Rect16 switch_rect;
    string_view_utf8 localizedItem = _(get_item());
    switch_rect = getCustomRect(window_menu, base_rolling_rect, localizedItem.computeNumUtf8CharsAndRewind() * window_menu.font->w + window_menu.padding.left + window_menu.padding.right);

    Rect16 rolling_rect = base_rolling_rect;
    rolling_rect = Rect16::Width_t(switch_rect.Left() - rolling_rect.Left());
    return std::array<Rect16, 2> { rolling_rect, switch_rect };
}

std::array<Rect16, 4> IWiSwitch::getSwitchMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
    Rect16 base_rect = getRollingRect(window_menu, rect);
    Rect16 switch_rect, bracket_start, bracket_end;

    bracket_end = { int16_t(base_rect.Left() + base_rect.Width() - window_menu.secondary_font->w), base_rect.Top(), uint16_t(window_menu.secondary_font->w), base_rect.Height() };
    base_rect -= bracket_end.Width();

    string_view_utf8 localizedItem = _(get_item());
    switch_rect = getCustomRect(window_menu, base_rect, localizedItem.computeNumUtf8CharsAndRewind() * window_menu.secondary_font->w);
    base_rect -= switch_rect.Width();

    bracket_start = { int16_t(base_rect.Left() + base_rect.Width() - window_menu.secondary_font->w), base_rect.Top(), uint16_t(window_menu.secondary_font->w), base_rect.Height() };
    base_rect -= bracket_start.Width();

    return std::array<Rect16, 4> { base_rect, bracket_start, switch_rect, bracket_end };
}

void IWiSwitch::printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const {
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    std::array<Rect16, 2> rects = IWiSwitch::getMenuRects(window_menu, rect);
#else  // PRINTER_TYPE != PRINTER_PRUSA_MINI
    std::array<Rect16, 4> rects = getSwitchMenuRects(window_menu, rect);
#endif // PRINTER_TYPE == PRINTER_PRUSA_MINI

    //draw label
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    render_text_align(rects[1], _(get_item()), window_menu.font,
        color_back, (IsFocused() && IsEnabled()) ? COLOR_ORANGE : color_text, window_menu.padding, window_menu.GetAlignment());
#else  // PRINTER_TYPE != PRINTER_PRUSA_MINI
    render_text_align(rects[1], _("["), window_menu.secondary_font,
        color_back, COLOR_SILVER, padding_ui8(0, 6, 0, 0), window_menu.GetAlignment());
    //draw switch
    render_text_align(rects[2], _(get_item()), window_menu.secondary_font,
        color_back, (IsFocused() && IsEnabled()) ? COLOR_ORANGE : color_text, padding_ui8(0, 6, 0, 0), window_menu.GetAlignment());
    //draw bracket end  TODO: Change font
    render_text_align(rects[3], _("]"), window_menu.secondary_font,
        color_back, COLOR_SILVER, padding_ui8(0, 6, 0, 0), window_menu.GetAlignment());
#endif // PRINTER_TYPE == PRINTER_PRUSA_MINI
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

WI_SELECT_t::WI_SELECT_t(int32_t index, const char **strings, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : IWindowMenuItem(string_view_utf8::MakeNULLSTR(), id_icon, enabled, hidden)
    , index(index)
    , strings(strings) {}

void WI_SELECT_t::printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const {
    Rect16 rolling_rect = getRollingRect(window_menu, rect);
    IWindowMenuItem::printItem(window_menu, rolling_rect, color_text, color_back, swap);
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
    : WI_LABEL_t(_(label), IDR_PNG_folder_up_16px, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    Screens::Access()->Close();
}

MI_TEST_DISABLED_RETURN::MI_TEST_DISABLED_RETURN()
    //just for test (in debug), do not translate
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), IDR_PNG_folder_up_16px, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_DISABLED_RETURN::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Close();
}
