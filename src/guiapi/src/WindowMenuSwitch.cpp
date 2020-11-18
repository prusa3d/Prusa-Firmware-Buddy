/**
 * @file WindowMenuSwitch.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuSwitch.hpp"
#include "resource.h"

/*****************************************************************************/
//IWiSwitch
IWiSwitch::IWiSwitch(int32_t index, string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden, Items_t items_)
    : AddSuper<WI_LABEL_t>(label, calculateExtensionWidth(items_, index), id_icon, enabled, hidden)
    , index(index)
    , items(items_) {
}

invalidate_t IWiSwitch::Change(int /*dif*/) {
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

bool IWiSwitch::SetIndex(size_t idx) {
    if (index == idx)
        return false;
    if (idx >= items.size)
        return false;
    else {
        index = idx;
        return true;
    }
}

Rect16 IWiSwitch::getSwitchRect(Rect16 extension_rect) const {
    if (!has_brackets)
        return extension_rect;

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

void IWiSwitch::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    switch (items.type) {
    case Items_t::type_t::text:
        printExtension_text(extension_rect, color_text, color_back, swap);
        break;
    case Items_t::type_t::icon:
        printExtension_icon(extension_rect, color_text, color_back, swap);
        break;
    }
}

void IWiSwitch::printExtension_text(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    //draw switch
    render_text_align(getSwitchRect(extension_rect), items.texts[index], GuiDefaults::FontMenuItems, color_back,
        (IsFocused() && IsEnabled()) ? GuiDefaults::ColorSelected : color_text,
        Padding, ALIGN_RIGHT_TOP);

    //draw brackets
    if (has_brackets) {
        static const uint8_t bf[] = "[";
        static const uint8_t be[] = "]";
        render_text_align(getLeftBracketRect(extension_rect), string_view_utf8::MakeCPUFLASH(bf), BracketFont,
            color_back, COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, GuiDefaults::MenuAlignment);

        //draw bracket end  TODO: Change font
        render_text_align(getRightBracketRect(extension_rect), string_view_utf8::MakeCPUFLASH(be), BracketFont,
            color_back, COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, GuiDefaults::MenuAlignment);
    }
}

void IWiSwitch::printExtension_icon(Rect16 extension_rect, color_t color_text, color_t color_back, uint8_t swap) const {
    //draw icon
    render_icon_align(extension_rect, items.icon_resources[index], color_back, RENDER_FLG(ALIGN_CENTER, swap));
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
        extension_width = calculateExtensionWidth_text(items, index);
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
        size_t width = window_icon_t::CalculateMinimalSize(items.icon_resources[i]).w;
        if (width > max_width)
            max_width = width;
    }
    return max_width + Padding.left + Padding.right;
}
