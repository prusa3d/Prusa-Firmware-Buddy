#include "WindowMenuItems.hpp"
#include "resource.h"
#include "ScreenHandler.hpp"

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, expands_t expands)
    : IWindowMenuItem(label, expands == expands_t::yes ? expand_icon_width : Rect16::Width_t(0), id_icon, enabled, hidden) {
}

IWindowMenuItem::IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : label(label)
    , hidden(hidden)
    , enabled(enabled)
    , focused(is_focused_t::no)
    , selected(is_selected_t::no)
    , id_icon(id_icon)
    , extension_width(extension_width_) {
}

/*****************************************************************************/
//rectangles
Rect16 IWindowMenuItem::getCustomRect(Rect16 base_rect, uint16_t custom_rect_width) {
    Rect16 custom_rect = { base_rect.Left(), base_rect.Top(), custom_rect_width, base_rect.Height() };
    custom_rect += Rect16::Left_t(base_rect.Width() - custom_rect.Width());
    return custom_rect;
}

Rect16 IWindowMenuItem::getIconRect(Rect16 rect) const {
    rect = icon_width;
    return rect;
}

Rect16 IWindowMenuItem::getLabelRect(Rect16 rect) const {
    rect -= icon_width;
    rect -= extension_width;
    rect += Rect16::Left_t(icon_width);
    return rect;
}

Rect16 IWindowMenuItem::getExtensionRect(Rect16 rect) const {
    rect += Rect16::Left_t(rect.Width() - extension_width);
    rect = extension_width;
    return rect;
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

    printIcon(getIconRect(rect), swap, window_menu.color_back);
    printLabel(getLabelRect(rect), window_menu, color_text, color_back);
    if (extension_width)
        printExtension(window_menu, getExtensionRect(rect), color_text, color_back, swap);
}

void IWindowMenuItem::printIcon(Rect16 icon_rect, uint8_t swap, color_t color_back) const {
    //do not check id. id == 0 will render as black, it is needed
    render_icon_align(icon_rect, id_icon, color_back, RENDER_FLG(ALIGN_CENTER, swap));
}

void IWindowMenuItem::printLabel(Rect16 label_rect, IWindowMenu &window_menu, color_t color_text, color_t color_back) const {
    roll.RenderTextAlign(label_rect, GetLabel(), window_menu.font, color_back, color_text, window_menu.padding, window_menu.GetAlignment());
}

void IWindowMenuItem::printExtension(IWindowMenu &window_menu, Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    render_icon_align(extension_rect, IDR_PNG_arrow_right_16px, window_menu.color_back, RENDER_FLG(ALIGN_LEFT_CENTER, swap));
}

void IWindowMenuItem::Click(IWindowMenu &window_menu) {
    roll.Deinit();
    window_menu.Invalidate();
    if (IsEnabled()) {
        click(window_menu);
    }
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

// Reinits text rolling in case of focus/defocus/click
void IWindowMenuItem::reInitRoll(IWindowMenu &window_menu, Rect16 rect) {
    if (roll.NeedInit()) {
        roll.Init(rect, GetLabel(), window_menu.font, window_menu.padding, window_menu.GetAlignment());
    }
}

/*****************************************************************************/
//IWiSpin
IWiSpin::IWiSpin(SpinType val, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, string_view_utf8 units_, size_t extension_width_)
    : AddSuper<WI_LABEL_t>(label, extension_width_, id_icon, enabled, hidden)
    , units(units_)
    , value(val) {
    //printSpinToBuffer(); initialized by parrent so it does not have to be virtual
    has_unit = !units_.isNULLSTR();
}

void IWiSpin::click(IWindowMenu & /*window_menu*/) {
    if (selected == is_selected_t::yes) {
        OnClick();
    }
    selected = selected == is_selected_t::yes ? is_selected_t::no : is_selected_t::yes;
}

Rect16 IWiSpin::getSpinRect(Rect16 rect) const {
    if (has_unit) {
        //do not remove this commented code .. dynamical version
        /*
        string_view_utf8 un = units;//local var because of const
        un.rewind();
        Rect16::Width_t unit_width = un.computeNumUtf8CharsAndRewind() * GuiDefaults::FontMenuSpecial->w;
        */
        Rect16::Width_t unit_width = unit__number_of_characters * GuiDefaults::FontMenuSpecial->w;
        rect -= unit_width;
    }
    return rect;
}

Rect16 IWiSpin::getUnitRect(Rect16 rect) const {
    Rect16 ret = getExtensionRect(rect);
    Rect16::Width_t unit_width = unit__number_of_characters * GuiDefaults::FontMenuSpecial->w;
    ret += Rect16::Left_t(getSpinRect(rect).Width());
    ret = unit_width;
    return ret;
}

void IWiSpin::printExtension(IWindowMenu &window_menu, Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    const Rect16 spin_rc = getSpinRect(extension_rect);
    const Rect16 unit_rc = getUnitRect(extension_rect);

    font_t *fnt = has_unit ? window_menu.font : GuiDefaults::FontMenuSpecial;
    padding_ui8_t padding = has_unit ? window_menu.padding : padding_ui8(0, 6, 0, 0);
    color_t cl_txt = IsSelected() ? COLOR_ORANGE : color_text;
    string_view_utf8 spin_txt = string_view_utf8::MakeRAM((const uint8_t *)spin_text_buff.data());
    uint8_t align = window_menu.GetAlignment();

    render_text_align(spin_rc, spin_txt, fnt, color_back, cl_txt, padding, align); //render spin number
    if (has_unit) {
        string_view_utf8 un = units; //local var because of const
        un.rewind();
        uint32_t Utf8Char = un.getUtf8Char();
        padding.left = Utf8Char == '\177' ? 0 : unit__half_space_padding;                 //177oct (127dec) todo check
        render_text_align(unit_rc, units, fnt, color_back, COLOR_SILVER, padding, align); //render unit
    }
}

/*****************************************************************************/
//IWiSwitch
IWiSwitch::IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, size_t extension_width_)
    : AddSuper<WI_LABEL_t>(label, extension_width_, id_icon, enabled, hidden)
    , index(index) {
    has_brackets = GuiDefaults::MenuSwitchHasBrackets;
}

invalidate_t IWiSwitch::Change(int /*dif*/) {
    if ((++index) >= size()) {
        index = 0;
    }
    return invalidate_t::yes;
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

Rect16 IWiSwitch::getSwitchRect(Rect16 extension_rect) const {
    if (!has_brackets)
        return extension_rect;

    extension_rect += Rect16::Left_t(BracketFont->w);
    extension_rect -= Rect16::Width_t(BracketFont->w * 2);
    return extension_rect;
}

Rect16 IWiSwitch::getLeftBracketRect(Rect16 extension_rect) const {
    extension_rect = Rect16::Width_t(BracketFont->w);
    return extension_rect;
}

Rect16 IWiSwitch::getRightBracketRect(Rect16 extension_rect) const {
    extension_rect += Rect16::Left_t(extension_rect.Width() - BracketFont->w);
    extension_rect = Rect16::Width_t(BracketFont->w);
    return extension_rect;
}

void IWiSwitch::printExtension(IWindowMenu &window_menu, Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    //draw switch
    render_text_align(getSwitchRect(extension_rect), _(get_item()), window_menu.font,
        color_back, (IsFocused() && IsEnabled()) ? COLOR_ORANGE : color_text,
        has_brackets ? padding_ui8(0, 6, 0, 0) : window_menu.padding,
        window_menu.GetAlignment());

    //draw brackets
    if (has_brackets) {
        render_text_align(getLeftBracketRect(extension_rect), _("["), BracketFont,
            color_back, COLOR_SILVER, padding_ui8(0, 6, 0, 0), window_menu.GetAlignment());

        //draw bracket end  TODO: Change font
        render_text_align(getRightBracketRect(extension_rect), _("]"), BracketFont,
            color_back, COLOR_SILVER, padding_ui8(0, 6, 0, 0), window_menu.GetAlignment());
    }
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
