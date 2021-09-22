/**
 * @file WindowMenuInfo.hpp
 * @author Michal Rudolf
 * @brief WI_INFO_t menu item, derived from WI_LABEL/IWindowMenuItem
 * @date 2020-12-7
 */

#pragma once

#include "WindowMenuLabel.hpp"
#include "GuiDefaults.hpp"

/*****************************************************************************/
//WI_INFO_t

/* MI_INFO_MAX_LEN
*  Bigger value will chop off some text of MI's label in version screen.
*  It could be increased, but runtime calculation of extention width is not supported (very complicated).
*  For now, if string is longer than ..MAX_LEN, it will print only ..MAX_LEN - 1 (null-terminated) chars.
*/

class WI_INFO_t : public AddSuper<WI_LABEL_t> {
private:
    static constexpr font_t *&InfoFont = GuiDefaults::FontMenuSpecial;
    static constexpr uint16_t icon_width = 16;
    char information[GuiDefaults::infoMaxLen];

public:
    WI_INFO_t(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden);
    WI_INFO_t(uint32_t num_to_print, string_view_utf8 label, is_hidden_t hidden = is_hidden_t::no, uint16_t id_icon = 0);

    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;
    virtual void click(IWindowMenu &window_menu) final {}
    invalidate_t ChangeInformation(const char *str);
};

//Dev version of info
class WI_INFO_DEV_t : public AddSuper<WI_INFO_t> {
public:
    WI_INFO_DEV_t(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled = is_enabled_t::yes)
        : AddSuper<WI_INFO_t>(label, id_icon, enabled, is_hidden_t::dev) {}
    WI_INFO_DEV_t(uint32_t num_to_print, string_view_utf8 label, uint16_t id_icon = 0)
        : AddSuper<WI_INFO_t>(num_to_print, label, is_hidden_t::dev, id_icon) {}
};
