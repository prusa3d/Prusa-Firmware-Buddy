/**
 * @file WindowMenuSpin.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuSpin.hpp"

IWiSpin::IWiSpin(SpinType val, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, string_view_utf8 units_, size_t extension_width_)
    : IWindowMenuItem(label, extension_width_, id_icon, enabled, hidden)
    , value(val)
    , units(units_) {
    // printSpinToBuffer(); initialized by parrent so it does not have to be virtual
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
        string_view_utf8 un = units; // local var because of const
        unichar uchar = un.getUtf8Char();
        size_t half_space_padding = (uchar == 0 || uchar == 0xB0) ? 0 : unit__half_space_padding;
        un.rewind();
        Rect16::Width_t unit_width = un.computeNumUtf8CharsAndRewind() * width(GuiDefaults::FontMenuSpecial) + Rect16::Width_t(half_space_padding);
        unit_width = unit_width + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right;
        ret = unit_width;
    } else {
        ret = Rect16::Width_t(0);
    }
    ret += Rect16::Left_t(extension_rect.Width() - ret.Width());
    return ret;
}

void IWiSpin::changeExtentionWidth(size_t unit_len, unichar uchar, size_t width) {
    if (width != spin_val_width) {
        spin_val_width = width;
        extension_width = calculateExtensionWidth(unit_len, uchar, width);
        deInitRoll();
    }
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

Rect16::Width_t IWiSpin::calculateExtensionWidth(size_t unit_len, unichar uchar, size_t value_max_digits) {
    size_t ret = value_max_digits * width(TheFont);
    uint8_t half_space = 0;
    if (unit_len) {
        if (GuiDefaults::MenuUseFixedUnitWidth) {
            return GuiDefaults::MenuUseFixedUnitWidth;
        }
        ret += unit_len * width(GuiDefaults::FontMenuSpecial);
        ret += GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right;
        half_space = uchar == '\xB0' ? 0 : unit__half_space_padding;
    }
    ret += Padding.left + Padding.right + half_space;
    return ret;
}

#if ENABLED(CRASH_RECOVERY)

WI_SPIN_CRASH_PERIOD_t::WI_SPIN_CRASH_PERIOD_t(int val, const Config &cnf, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : IWiSpin(std::clamp(int(val), cnf.Min(), cnf.Max()), label, id_icon, enabled, hidden,
        cnf.Unit() == nullptr ? string_view_utf8::MakeNULLSTR() : _(cnf.Unit()), 0)
    , config(cnf) {
    printSpinToBuffer();

    // spin_val_width = cnf.txtMeas(val);
    size_t unit_len = 0;
    unichar uchar = 0;
    if (config.Unit() != nullptr) {
        string_view_utf8 un = units;
        uchar = un.getUtf8Char();
        un.rewind();
        unit_len = un.computeNumUtf8CharsAndRewind();
    }
    extension_width = calculateExtensionWidth(unit_len, uchar, spin_val_width);
}

invalidate_t WI_SPIN_CRASH_PERIOD_t::change(int dif) {
    int val = GetVal();
    int old = val;
    val += (int)dif * config.Step();
    val = dif >= 0 ? std::max(val, old) : std::min(val, old); // check overflow/underflow
    val = std::clamp(val, config.Min(), config.Max());
    set_val(val);
    invalidate_t invalid = (!dif || old != val) ? invalidate_t::yes : invalidate_t::no; // 0 dif forces redraw
    if (invalid == invalidate_t::yes) {
        if (!has_unit || config.Unit() == nullptr) {
            changeExtentionWidth(0, 0, config.txtMeas(GetVal()));
        } else {
            string_view_utf8 un = units;
            unichar uchar = un.getUtf8Char();
            un.rewind();
            changeExtentionWidth(units.computeNumUtf8CharsAndRewind(), uchar, config.txtMeas(GetVal()));
        }
        printSpinToBuffer(); // could be in draw method, but traded little performance for code size (printSpinToBuffer is not virtual when it is here)
    }
    return invalid;
}

#endif
