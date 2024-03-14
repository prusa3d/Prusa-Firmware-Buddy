/**
 * @file WindowMenuSwitch.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuSwitch.hpp"

/*****************************************************************************/
// IWiSwitch
IWiSwitch::IWiSwitch(int32_t index, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, Items_t items_)
    : AddSuper<WI_LABEL_t>(label, calculateExtensionWidth(items_, index), id_icon, enabled, hidden)
    , index(index)
    , items(items_) {
}

invalidate_t IWiSwitch::change(int /*dif*/) {
    if ((++index) >= items.size) {
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
    if ((index != idx) && (idx < items.size)) {
        index = idx;
        changeExtentionWidth();
        InValidateExtension();
    }
}

size_t IWiSwitch::GetIndex() const {
    return index;
}

Rect16 IWiSwitch::getSwitchRect(Rect16 extension_rect) const {
    if (!has_brackets) {
        return extension_rect;
    }

    extension_rect += Rect16::Left_t(BracketFont->w + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    extension_rect -= Rect16::Width_t(BracketFont->w * 2 + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

Rect16 IWiSwitch::getLeftBracketRect(Rect16 extension_rect) const {
    extension_rect = Rect16::Width_t(BracketFont->w + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

Rect16 IWiSwitch::getRightBracketRect(Rect16 extension_rect) const {
    extension_rect += Rect16::Left_t(extension_rect.Width() - (BracketFont->w + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right));
    extension_rect = Rect16::Width_t(BracketFont->w + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right);
    return extension_rect;
}

void IWiSwitch::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const {
    switch (items.type) {
    case Items_t::type_t::text:
        printExtension_text(extension_rect, color_text, color_back, raster_op);
        break;
    case Items_t::type_t::icon:
        printExtension_icon(extension_rect, color_text, color_back, raster_op);
        break;
    }
}

void IWiSwitch::printExtension_text(Rect16 extension_rect, color_t color_text, color_t color_back, [[maybe_unused]] ropfn raster_op) const {
    // draw switch
    render_text_align(getSwitchRect(extension_rect), items.texts[index], GuiDefaults::FontMenuItems, color_back,
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

void IWiSwitch::printExtension_icon(Rect16 extension_rect, [[maybe_unused]] color_t color_text, color_t color_back, ropfn raster_op) const {
    render_icon_align(extension_rect, items.icon_resources[index], color_back, { Align_t::Center(), raster_op });
}

Rect16::Width_t IWiSwitch::calculateExtensionWidth(Items_t items, int32_t idx) {
    switch (items.type) {
    case Items_t::type_t::text:
        return calculateExtensionWidth_text(items, idx);
        break;
    case Items_t::type_t::icon:
        return calculateExtensionWidth_icon(items);
        break;
    }
    return 0;
}

void IWiSwitch::changeExtentionWidth() {
    if (items.type == Items_t::type_t::text) {
        Rect16::Width_t new_extension_width = calculateExtensionWidth_text(items, index);
        if (extension_width != new_extension_width) {
            extension_width = new_extension_width;
            Invalidate();
        }
    }
}

Rect16::Width_t IWiSwitch::calculateExtensionWidth_text(Items_t items, int32_t idx) {
    size_t len = items.texts[idx].computeNumUtf8CharsAndRewind();
    size_t ret = GuiDefaults::FontMenuItems->w * len + Padding.left + Padding.right + (GuiDefaults::MenuSwitchHasBrackets ? (BracketFont->w + GuiDefaults::MenuPaddingSpecial.left + GuiDefaults::MenuPaddingSpecial.right) * 2 : 0);
    return ret;
}

Rect16::Width_t IWiSwitch::calculateExtensionWidth_icon(Items_t items) {
    size_t max_width = 0;
    for (size_t i = 0; i < items.size; ++i) {
        size_t width = items.icon_resources[i]->w;
        if (width > max_width) {
            max_width = width;
        }
    }
    return max_width + Padding.left + Padding.right;
}
