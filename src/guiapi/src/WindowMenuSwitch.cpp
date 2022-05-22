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
    : AddSuper<WI_LABEL_t>(label, calculateExtensionWidth(items_), id_icon, enabled, hidden)
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
}

void IWiSwitch::SetIndex(size_t idx) {
    if ((index != idx) && (idx < items.size)) {
        index = idx;
        InValidateExtension();
    }
}

size_t IWiSwitch::GetIndex() const {
    return index;
}

Rect16 IWiSwitch::getSwitchRect(Rect16 extension_rect) const {
    if (!has_brackets)
        return extension_rect;

    extension_rect += Rect16::Left_t(BracketFont->w + Padding.left + Padding.right);
    extension_rect -= Rect16::Width_t(BracketFont->w * 2 + Padding.left + Padding.right);
    return extension_rect;
}

Rect16 IWiSwitch::getLeftBracketRect(Rect16 extension_rect) const {
    extension_rect = Rect16::Width_t(BracketFont->w + Padding.left + Padding.right);
    return extension_rect;
}

Rect16 IWiSwitch::getRightBracketRect(Rect16 extension_rect) const {
    extension_rect += Rect16::Left_t(extension_rect.Width() - (BracketFont->w + Padding.left + Padding.right));
    extension_rect = Rect16::Width_t(BracketFont->w * +Padding.left + Padding.right);
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

void IWiSwitch::printExtension_text(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const {
    //draw switch
    render_text_align(getSwitchRect(extension_rect), items.texts[index], GuiDefaults::FontMenuItems, color_back,
        (IsFocused() && IsEnabled()) ? GuiDefaults::ColorSelected : color_text,
        Padding, Align_t::RightTop());

    //draw brackets
    if (has_brackets) {
        static const uint8_t bf[] = "[";
        static const uint8_t be[] = "]";
        render_text_align(getLeftBracketRect(extension_rect), string_view_utf8::MakeCPUFLASH(bf), BracketFont,
            color_back, COLOR_SILVER, Padding, GuiDefaults::MenuAlignment());

        //draw bracket end  TODO: Change font
        render_text_align(getRightBracketRect(extension_rect), string_view_utf8::MakeCPUFLASH(be), BracketFont,
            color_back, COLOR_SILVER, Padding, GuiDefaults::MenuAlignment());
    }
}

void IWiSwitch::printExtension_icon(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const {
    //draw icon
    render_icon_align(extension_rect, items.icon_resources[index], color_back, { Align_t::Center(), raster_op });
}

Rect16::Width_t IWiSwitch::calculateExtensionWidth(Items_t items) {
    switch (items.type) {
    case Items_t::type_t::text:
        return calculateExtensionWidth_text(items);
        break;
    case Items_t::type_t::icon:
        return calculateExtensionWidth_icon(items);
        break;
    }
    return 0;
}

Rect16::Width_t IWiSwitch::calculateExtensionWidth_text(Items_t items) {
    size_t max_len = 0;
    for (size_t i = 0; i < items.size; ++i) {
        size_t len = items.texts[i].computeNumUtf8CharsAndRewind();
        if (len > max_len)
            max_len = len;
    }
    size_t ret = GuiDefaults::FontMenuItems->w * max_len + Padding.left + Padding.right + (GuiDefaults::MenuSwitchHasBrackets ? (BracketFont->w + Padding.left + Padding.right) * 2 : 0);
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
