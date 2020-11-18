/**
 * @file WindowMenuSpin.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuSpin.hpp"
#include "resource.h"

IWiSpin::IWiSpin(SpinType val, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, string_view_utf8 units_, size_t extension_width_)
    : AddSuper<WI_LABEL_t>(label, extension_width_, id_icon, enabled, hidden)
    , units(units_)
    , value(val) {
    //printSpinToBuffer(); initialized by parrent so it does not have to be virtual
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
        size_t half_space_padding = un.getUtf8Char() == '\177' ? 0 : unit__half_space_padding;
        un.rewind();
        Rect16::Width_t unit_width = un.computeNumUtf8CharsAndRewind() * GuiDefaults::FontMenuSpecial->w + Rect16::Width_t(half_space_padding);
        unit_width = unit_width + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right;
        ret = unit_width;
    } else {
        ret = Rect16::Width_t(0);
    }
    ret += Rect16::Left_t(extension_rect.Width() - ret.Width());
    return ret;
}

void IWiSpin::changeExtentionWidth(size_t unit_len, char uchar, size_t width) {
    if (width != spin_val_width) {
        spin_val_width = width;
        extension_width = calculateExtensionWidth(unit_len, uchar, width);
        deInitRoll();
    }
}

void IWiSpin::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    const Rect16 spin_rc = getSpinRect(extension_rect);
    const Rect16 unit_rc = getUnitRect(extension_rect);

    const color_t cl_txt = IsSelected() ? COLOR_ORANGE : color_text;
    string_view_utf8 spin_txt = string_view_utf8::MakeRAM((const uint8_t *)spin_text_buff.data());
    const uint8_t align = ALIGN_RIGHT_TOP;

    render_text_align(spin_rc, spin_txt, Font, color_back, cl_txt, Padding, align); //render spin number
    if (has_unit) {
        string_view_utf8 un = units; //local var because of const
        un.rewind();
        uint32_t Utf8Char = un.getUtf8Char();
        padding_ui8_t padding = GuiDefaults::MenuPaddingSpecial;
        padding.left = Utf8Char == '\177' ? 0 : unit__half_space_padding;                                          //177oct (127dec) todo check
        render_text_align(unit_rc, units, GuiDefaults::FontMenuSpecial, color_back, COLOR_SILVER, padding, align); //render unit
    }
}

Rect16::Width_t IWiSpin::calculateExtensionWidth(size_t unit_len, char uchar, size_t value_max_digits) {
    size_t ret = value_max_digits * Font->w;
    uint8_t half_space = 0;
    if (unit_len) {
        if (GuiDefaults::MenuUseFixedUnitWidth)
            return GuiDefaults::MenuUseFixedUnitWidth;
        ret += unit_len * GuiDefaults::FontMenuSpecial->w;
        ret += GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right;
        half_space = uchar == '\177' ? 0 : unit__half_space_padding;
    }
    ret += Padding.left + Padding.right + half_space;
    return ret;
}
