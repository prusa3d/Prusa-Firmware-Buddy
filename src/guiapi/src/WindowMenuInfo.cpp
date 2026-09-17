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

uint16_t IWiInfo::max_extension_width() {
    return GuiDefaults::ScreenWidth - (icon_width + resource_font(GuiDefaults::FontMenuItems)->w * 5 + GuiDefaults::MenuScrollbarWidth);
}

void IWiInfo::update_extension_width() {
    uint16_t new_width = value_.computeNumUtf8Chars() * width(font);

    if (!GetLabel().isNULLSTR()) {
        // Make sure there is enough space for at least a few characters of the label
        // This is a bit of a heuristic, we don't know the menu item rect when calling this function
        new_width = std::min<uint16_t>(new_width, max_extension_width());
    }

    if (new_width != extension_width) {
        extension_width = new_width;
        Invalidate();
    }
}

Color IWiInfo::extension_text_color(const IWindowMenuItem &item) {
    return item.IsFocused() ? COLOR_DARK_GRAY : COLOR_SILVER;
}

void IWiInfo::printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const {
    render_text_align(extension_rect, value(), font, color_back, extension_text_color(*this), GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
}

void WiInfoString::set_value(const string_view_utf8 &set) {
    if (!value_.is_same_ref(set)) {
        value_ = set;
        update_extension_width();
        InValidateExtension();
    }
}
