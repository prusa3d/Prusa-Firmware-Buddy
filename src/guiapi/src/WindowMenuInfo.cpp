/**
 * @file WindowMenuInfo.cpp
 * @author Michal Rudolf
 * @date 2020-12-07
 */

#include "WindowMenuInfo.hpp"

IWiInfo::IWiInfo(string_view_utf8 value, size_t max_characters, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, ExtensionLikeLabel extension_like_label)
    : IWindowMenuItem(label, id_icon ? id_icon->w : calculate_extension_width(extension_like_label, max_characters), id_icon, enabled, hidden)
    , value_(value) {
}

void IWiInfo::printExtension(Rect16 extension_rect, [[maybe_unused]] color_t color_text, color_t color_back, [[maybe_unused]] ropfn raster_op) const {
    const auto info_str = value();

    if (has_extension_like_label == ExtensionLikeLabel::yes) {
        render_text_align(extension_rect, info_str, getLabelFont(), color_back, GetTextColor(),
            { (uint8_t)0U, (uint8_t)0U, (uint8_t)0U, (uint8_t)0U }, Align_t::RightCenter());
    } else {
        render_text_align(extension_rect, info_str, font, color_back, IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER,
            GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
    }
}

void WiInfoString::set_value(string_view_utf8 set) {
    if (!value_.is_same_ref(set)) {
        value_ = set;
        InValidateExtension();
    }
}
