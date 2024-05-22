/**
 * @file WindowMenuSwitch.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuSwitch.hpp"

/*****************************************************************************/
// IWiSwitch
IWiSwitch::IWiSwitch(string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : IWindowMenuItem(label, 0, id_icon, enabled, hidden) //
{}

invalidate_t IWiSwitch::change(int /*dif*/) {
    if ((++index) >= item_count()) {
        index = 0;
    }
    return invalidate_t::yes;
}

void IWiSwitch::click(IWindowMenu & /*window_menu*/) {
    size_t old_index = index;
    Change(0);
    OnChange(old_index);
    changeExtentionWidth();
}

/**
 * @brief handle touch
 * it behaves the same as click, but only when extension was clicked
 */
void IWiSwitch::touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point) {
    if (is_touch_in_extension_rect(window_menu, relative_touch_point)) {
        click(window_menu);
    }
}

void IWiSwitch::SetIndex(size_t idx) {
    if (idx == index || idx >= item_count()) {
        return;
    }

    index = idx;
    changeExtentionWidth();
    InValidateExtension();
}

Rect16 IWiSwitch::getSwitchRect(Rect16 extension_rect) const {
    if (!has_brackets) {
        return extension_rect;
    }

    extension_rect += Rect16::Left_t(width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    extension_rect -= Rect16::Width_t(width(BracketFont) * 2 + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

Rect16 IWiSwitch::getLeftBracketRect(Rect16 extension_rect) const {
    extension_rect = Rect16::Width_t(width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

Rect16 IWiSwitch::getRightBracketRect(Rect16 extension_rect) const {
    extension_rect += Rect16::Left_t(extension_rect.Width() - (width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right));
    extension_rect = Rect16::Width_t(width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

void IWiSwitch::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, [[maybe_unused]] ropfn raster_op) const {
    // draw switch
    render_text_align(getSwitchRect(extension_rect), current_item_text(), GuiDefaults::FontMenuItems, color_back,
        (IsFocused() && IsEnabled()) ? GuiDefaults::ColorSelected : color_text,
        padding_ui8(0, 4, 0, 0), Align_t::Center(), false);

    // draw brackets
    if (has_brackets) {
        static const uint8_t bf[] = "[";
        static const uint8_t be[] = "]";
        render_text_align(getLeftBracketRect(extension_rect), string_view_utf8::MakeCPUFLASH(bf), BracketFont,
            color_back, IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, Align_t::Center(), false);

        // draw bracket end  TODO: Change font
        render_text_align(getRightBracketRect(extension_rect), string_view_utf8::MakeCPUFLASH(be), BracketFont,
            color_back, IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, Align_t::Center(), false);
    }
}

Rect16::Width_t IWiSwitch::calculateExtensionWidth() const {
    const size_t len = current_item_text().computeNumUtf8CharsAndRewind();
    const size_t ret = width(GuiDefaults::FontMenuItems) * len + Padding.left + Padding.right + (GuiDefaults::MenuSwitchHasBrackets ? (width(BracketFont) + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right) * 2 : 0);
    return ret;
}

void IWiSwitch::changeExtentionWidth() {
    Rect16::Width_t new_extension_width = calculateExtensionWidth();
    if (extension_width != new_extension_width) {
        extension_width = new_extension_width;
        Invalidate();
    }
}
