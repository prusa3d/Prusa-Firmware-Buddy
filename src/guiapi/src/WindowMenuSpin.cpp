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
        un.rewind();
        Rect16::Width_t unit_width = un.computeNumUtf8CharsAndRewind() * GuiDefaults::FontMenuSpecial->w;
        unit_width = unit_width + Padding.left + Padding.right;
        ret = unit_width;
    } else {
        ret = Rect16::Width_t(0);
    }
    ret += Rect16::Left_t(extension_rect.Width() - ret.Width());
    return ret;
}

void IWiSpin::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const {

    string_view_utf8 spin_txt = string_view_utf8::MakeRAM((const uint8_t *)spin_text_buff.data());
    const color_t cl_txt = IsSelected() ? COLOR_ORANGE : color_text;
    const Align_t align = Align_t::RightTop();

    // If there is spin_off_opt::yes set in SpinConfig (with units), it prints "Off" instead of "0"
    if (spin_txt.getUtf8Char() == 'O') {
        spin_txt.rewind();
        uint16_t curr_width = extension_rect.Width();
        uint16_t off_opt_width = Font->w * spin_txt.computeNumUtf8CharsAndRewind() + Padding.left + Padding.right;
        if (curr_width < off_opt_width) {
            extension_rect -= Rect16::Left_t(off_opt_width - curr_width);
            extension_rect = Rect16::Width_t(off_opt_width);
        }
        render_text_align(extension_rect, spin_txt, Font, color_back, cl_txt, Padding, align); //render spin number
        return;
    }

    spin_txt.rewind();
    const Rect16 spin_rc = getSpinRect(extension_rect);
    const Rect16 unit_rc = getUnitRect(extension_rect);
    render_text_align(spin_rc, spin_txt, Font, color_back, cl_txt, Padding, align); //render spin number

    if (has_unit) {
        string_view_utf8 un = units; //local var because of const
        un.rewind();
        uint32_t Utf8Char = un.getUtf8Char();
        padding_ui8_t padding = Padding;
        padding.left = Utf8Char == '\177' ? 0 : unit__half_space_padding;                  //177oct (127dec) todo check
        render_text_align(unit_rc, units, Font, color_back, COLOR_SILVER, padding, align); //render unit
    }
}

Rect16::Width_t IWiSpin::calculateExtensionWidth(const char *unit, size_t value_max_digits) {
    size_t ret = value_max_digits * Font->w;
    if (unit) {
        if (GuiDefaults::MenuUseFixedUnitWidth)
            return GuiDefaults::MenuUseFixedUnitWidth;
        ret += 2 * (Padding.left + Padding.right);
        ret += _(unit).computeNumUtf8CharsAndRewind() * GuiDefaults::FontMenuSpecial->w;
    } else {
        ret += Padding.left + Padding.right;
    }
    return ret;
}
