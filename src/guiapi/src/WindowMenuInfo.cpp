/**
 * @file WindowMenuInfo.cpp
 * @author Michal Rudolf
 * @date 2020-12-07
 */

#include "WindowMenuInfo.hpp"

/*****************************************************************************/
// IWiInfo
IWiInfo::IWiInfo(string_view_utf8 label, uint16_t id_icon, size_t info_len, is_enabled_t enabled, is_hidden_t hidden)
    : AddSuper<WI_LABEL_t>(label, id_icon ? icon_width : info_len * InfoFont->w, id_icon, enabled, hidden) {}

void IWiInfo::printInfo(Rect16 extension_rect, color_t color_back, string_view_utf8 info_str) const {

    render_text_align(extension_rect, info_str, InfoFont, color_back,
        (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingSpecial, Align_t::RightCenter());
}
