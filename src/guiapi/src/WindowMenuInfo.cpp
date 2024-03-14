/**
 * @file WindowMenuInfo.cpp
 * @author Michal Rudolf
 * @date 2020-12-07
 */

#include "WindowMenuInfo.hpp"

/*****************************************************************************/
// IWiInfo
IWiInfo::IWiInfo(string_view_utf8 label, const img::Resource *id_icon, size_t info_len, is_enabled_t enabled, is_hidden_t hidden, ExtensionLikeLabel extension_like_label)
    : AddSuper<WI_LABEL_t>(label, id_icon ? icon_width : calculate_extension_width(extension_like_label, info_len), id_icon, enabled, hidden) {
    has_extension_like_label = extension_like_label;
}

void IWiInfo::printInfo(Rect16 extension_rect, color_t color_back, string_view_utf8 info_str) const {

    if (has_extension_like_label == ExtensionLikeLabel::yes) {
        render_text_align(extension_rect, info_str, getLabelFont(), color_back, GetTextColor(),
            { (uint8_t)0U, (uint8_t)0U, (uint8_t)0U, (uint8_t)0U }, Align_t::RightCenter());
    } else {
        render_text_align(extension_rect, info_str, InfoFont, color_back, IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER,
            GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
    }
}
