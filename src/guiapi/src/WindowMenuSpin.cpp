/**
 * @file WindowMenuSpin.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuSpin.hpp"

IWiSpin::IWiSpin(string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, string_view_utf8 units_, size_t extension_width_)
    : IWindowMenuItem(label, extension_width_, id_icon, enabled, hidden)
    , units(units_) {
}

void IWiSpin::click(IWindowMenu & /*window_menu*/) {
    if (is_edited()) {
        OnClick();
    }
    toggle_edit_mode();
}

/**
 * @brief handle touch
 * it behaves the same as click, but only when extension was clicked
 */
void IWiSpin::touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) {
    if (is_touch_in_extension_rect(window_menu, relative_touch_point)) {
        set_is_edited(true);
    }
}

Rect16 IWiSpin::getSpinRect(Rect16 extension_rect) const {
    extension_rect -= getUnitRect(extension_rect).Width();
    return extension_rect;
}

Rect16 IWiSpin::getUnitRect(Rect16 extension_rect) const {
    Rect16 ret = extension_rect;
    if (has_unit && !units.isNULLSTR()) {
        const unichar uchar = units.getFirstUtf8Char();
        size_t half_space_padding = (uchar == 0 || uchar == 0xB0) ? 0 : unit__half_space_padding;
        Rect16::Width_t unit_width = units.computeNumUtf8Chars() * width(GuiDefaults::FontMenuSpecial) + Rect16::Width_t(half_space_padding);
        unit_width = unit_width + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right;
        ret = unit_width;
    } else {
        ret = Rect16::Width_t(0);
    }
    ret += Rect16::Left_t(extension_rect.Width() - ret.Width());
    return ret;
}

static constexpr Font TheFont = GuiDefaults::MenuSpinHasUnits ? GuiDefaults::FontMenuSpecial : GuiDefaults::FontMenuItems;

void IWiSpin::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, [[maybe_unused]] ropfn raster_op) const {

    string_view_utf8 spin_txt = string_view_utf8::MakeRAM((const uint8_t *)spin_text_buff.data());
    const color_t cl_txt = is_edited() ? COLOR_ORANGE : color_text;
    const Align_t align = Align_t::RightTop(); // This have to be aligned this way and set up with padding, because number and units have different fonts
    padding_ui8_t extension_padding = Padding;
    if constexpr (GuiDefaults::MenuSpinHasUnits) {
        extension_padding.top = 12;
    }

    // If there is spin_off_opt::yes set in SpinConfig (with units), it prints "Off" instead of "0"
    unichar ch = spin_txt.getUtf8Char();
    if (ch > 57 || (ch < 48 && ch != '-' && ch != '+')) { // first character is not a number (or +-). This is necessary because "Off" is translated
        spin_txt.rewind();
        uint16_t curr_width = extension_rect.Width();
        uint16_t off_opt_width = width(TheFont) * spin_txt.computeNumUtf8CharsAndRewind() + extension_padding.left + extension_padding.right;
        if (curr_width < off_opt_width) {
            extension_rect -= Rect16::Left_t(off_opt_width - curr_width);
            extension_rect = Rect16::Width_t(off_opt_width);
        }
        render_text_align(extension_rect, spin_txt, TheFont, color_back, cl_txt, extension_padding, align); // render spin number
        return;
    }

    spin_txt.rewind();
    const Rect16 spin_rc = getSpinRect(extension_rect);
    const Rect16 unit_rc = getUnitRect(extension_rect);
    render_text_align(spin_rc, spin_txt, TheFont, color_back, cl_txt, extension_padding, align); // render spin number

    if (has_unit) {
        string_view_utf8 un = units; // local var because of const
        un.rewind();
        unichar Utf8Char = un.getUtf8Char();
        padding_ui8_t unit_padding = extension_padding;
        unit_padding.left = Utf8Char == 0xB0 ? 0 : unit__half_space_padding;
        render_text_align(unit_rc, units, TheFont, color_back, IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER, unit_padding, align); // render unit
    }
}

Rect16::Width_t IWiSpin::calculateExtensionWidth(const string_view_utf8 &units, size_t value_max_digits) {
    size_t unit_len = 0;
    if (has_unit) {
        unit_len = units.computeNumUtf8Chars();
    }
    size_t ret = value_max_digits * width(TheFont);
    uint8_t half_space = 0;
    if (unit_len) {
        if (GuiDefaults::MenuUseFixedUnitWidth) {
            return GuiDefaults::MenuUseFixedUnitWidth;
        }
        ret += unit_len * width(GuiDefaults::FontMenuSpecial);
        ret += GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right;
        const unichar uchar = units.getFirstUtf8Char();
        half_space = uchar == '\xB0' ? 0 : unit__half_space_padding;
    }
    ret += Padding.left + Padding.right + half_space;
    return ret;
}
