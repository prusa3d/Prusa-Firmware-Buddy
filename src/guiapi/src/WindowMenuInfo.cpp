/**
 * @file WindowMenuInfo.cpp
 * @author Michal Rudolf
 * @date 2020-12-07
 */

#include "WindowMenuInfo.hpp"

/*****************************************************************************/
// IWiInfo
IWiInfo::IWiInfo(std::span<char> value_buffer, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, ExtensionLikeLabel extension_like_label)
    : IWindowMenuItem(label, id_icon ? icon_width : calculate_extension_width(extension_like_label, value_buffer.size()), id_icon, enabled, hidden)
    , value_bufer_(value_buffer) {
    has_extension_like_label = extension_like_label;
}

void IWiInfo::ChangeInformation(const char *str) {
    if (strncmp(value_bufer_.data(), str, value_bufer_.size())) {
        strlcpy(value_bufer_.data(), str, value_bufer_.size());
        InValidateExtension();
    }
}

void IWiInfo::printExtension(Rect16 extension_rect, [[maybe_unused]] color_t color_text, color_t color_back, [[maybe_unused]] ropfn raster_op) const {
    const auto info_str = string_view_utf8::MakeRAM(value_bufer_.data());

    if (has_extension_like_label == ExtensionLikeLabel::yes) {
        render_text_align(extension_rect, info_str, getLabelFont(), color_back, GetTextColor(),
            { (uint8_t)0U, (uint8_t)0U, (uint8_t)0U, (uint8_t)0U }, Align_t::RightCenter());
    } else {
        render_text_align(extension_rect, info_str, InfoFont, color_back, IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER,
            GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
    }
}
