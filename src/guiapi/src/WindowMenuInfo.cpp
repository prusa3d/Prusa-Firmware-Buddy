/**
 * @file WindowMenuInfo.cpp
 * @author Michal Rudolf
 * @date 2020-12-07
 */

#include "WindowMenuInfo.hpp"

IWiInfo::IWiInfo(const string_view_utf8 &value, const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : IWindowMenuItem(label, 0, id_icon, enabled, hidden)
    , value_(value) {
    update_extension_width();
}

void IWiInfo::update_extension_width() {
    uint16_t new_width;
    if (id_icon && icon_position == IconPosition::right) {
        new_width = id_icon->w;
    } else {
        new_width = value_.computeNumUtf8Chars() * width(font);
    }

    if (!GetLabel().isNULLSTR()) {
        // Make sure there is enough space for at least a few characters of the label
        // This is a bit of a heuristic, we don't know the menu item rect when calling this function
        const uint16_t max_width = GuiDefaults::ScreenWidth - (icon_width + resource_font(getLabelFont())->w * 5 + GuiDefaults::MenuScrollbarWidth);
        new_width = std::min<uint16_t>(new_width, max_width);
    }

    if (new_width != extension_width) {
        extension_width = new_width;
        Invalidate();
    }
}

void IWiInfo::printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const {
    render_text_align(extension_rect, value(), font, color_back, IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
}

void WiInfoString::set_value(const string_view_utf8 &set) {
    if (!value_.is_same_ref(set)) {
        value_ = set;
        update_extension_width();
        InValidateExtension();
    }
}
