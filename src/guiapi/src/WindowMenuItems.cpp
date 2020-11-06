#include "WindowMenuItems.hpp"
#include "resource.h"
#include "ScreenHandler.hpp"

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

/**
 * return changed (== invalidate)
 **/
bool IWindowMenuItem::Change(int /*dif*/) {
    return false;
}

void IWindowMenuItem::printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const {
    std::array<Rect16, 2> rects = getMenuRects(window_menu, rect);
    printLabel_into_rect(rects[0], color_text, color_back, window_menu.font, window_menu.padding, window_menu.GetAlignment());
    if (expands == expands_t::yes) {
        render_icon_align(rects[1], IDR_PNG_arrow_right_16px, window_menu.color_back, RENDER_FLG(ALIGN_LEFT_CENTER, swap));
    }
}

std::array<Rect16, 2> IWindowMenuItem::getMenuRects(IWindowMenu &window_menu, Rect16 rect) const {
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

void IWindowMenuItem::InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) {
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
    : AddSuper<WI_LABEL_t>(label, id_icon, enabled, hidden)
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
    : WI_LABEL_t(string_view_utf8::MakeNULLSTR(), id_icon, enabled, hidden)
    , index(index)
    , strings(strings) {}

void WI_SELECT_t::printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const {
    Rect16 rolling_rect = getRollingRect(window_menu, rect);
    WI_LABEL_t::printItem(window_menu, rolling_rect, color_text, color_back, swap);
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
