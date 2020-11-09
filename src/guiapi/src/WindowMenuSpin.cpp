/**
 * @file WindowMenuSpin.cpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-09
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "WindowMenuItems.hpp"
#include "resource.h"

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

Rect16 IWiSpin::getSpinRect(Rect16 extension_rect) const {
    extension_rect -= getUnitRect(extension_rect).Width();
    return extension_rect;
}

Rect16 IWiSpin::getUnitRect(Rect16 extension_rect) const {
    Rect16 ret = extension_rect;
    if (has_unit) {
        string_view_utf8 un = units; //local var because of const
        un.rewind();
        Rect16::Width_t unit_width = un.computeNumUtf8CharsAndRewind() * GuiDefaults::FontMenuSpecial->w;
        ret = unit_width;
    } else {
        ret = Rect16::Width_t(0);
    }
    ret += Rect16::Left_t(extension_rect.Width() - ret.Width());
    return ret;
}

void IWiSpin::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    const Rect16 spin_rc = getSpinRect(extension_rect);
    const Rect16 unit_rc = getUnitRect(extension_rect);

    font_t *fnt = has_unit ? GuiDefaults::FontMenuItems : GuiDefaults::FontMenuSpecial;
    padding_ui8_t padding = has_unit ? GuiDefaults::MenuPadding : padding_ui8(0, 6, 0, 0);
    color_t cl_txt = IsSelected() ? COLOR_ORANGE : color_text;
    string_view_utf8 spin_txt = string_view_utf8::MakeRAM((const uint8_t *)spin_text_buff.data());
    uint8_t align = GuiDefaults::MenuAlignment;

    render_text_align(spin_rc, spin_txt, fnt, color_back, cl_txt, padding, align); //render spin number
    if (has_unit) {
        string_view_utf8 un = units; //local var because of const
        un.rewind();
        uint32_t Utf8Char = un.getUtf8Char();
        padding.left = Utf8Char == '\177' ? 0 : unit__half_space_padding;                 //177oct (127dec) todo check
        render_text_align(unit_rc, units, fnt, color_back, COLOR_SILVER, padding, align); //render unit
    }
}
