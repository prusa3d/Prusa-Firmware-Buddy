/**
 * @file WindowMenuInfo.cpp
 * @author Michal Rudolf
 * @date 2020-12-07
 */

#include "WindowMenuInfo.hpp"

/*****************************************************************************/
// WI_INFO_t

WI_INFO_t::WI_INFO_t(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : AddSuper<WI_LABEL_t>(label, id_icon ? icon_width : GuiDefaults::infoMaxLen * InfoFont->w, id_icon, enabled, hidden) {}

WI_INFO_t::WI_INFO_t(uint32_t num_to_print, string_view_utf8 label, is_hidden_t hidden, uint16_t id_icon)
    : WI_INFO_t(label, id_icon, is_enabled_t::yes, hidden) {
    itoa(num_to_print, information, 10);
}

invalidate_t WI_INFO_t::ChangeInformation(const char *str) {
    if (strncmp(information, str, GuiDefaults::infoMaxLen)) {
        strlcpy(information, str, GuiDefaults::infoMaxLen);
        information[GuiDefaults::infoMaxLen - 1] = 0;
        return invalidate_t::yes;
    }
    return invalidate_t::no;
}

void WI_INFO_t::printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const {

    render_text_align(extension_rect, _(information), InfoFont, color_back,
        (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
}
