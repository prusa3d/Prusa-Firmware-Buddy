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

class IWiInfo : public AddSuper<WI_LABEL_t> {
    static constexpr font_t *&InfoFont = GuiDefaults::FontMenuSpecial;
    static constexpr uint16_t icon_width = 16;

protected:
    void printInfo(Rect16 extension_rect, color_t color_back, string_view_utf8 info_str) const;

public:
    IWiInfo(string_view_utf8 label, uint16_t id_icon, size_t info_len, is_enabled_t enabled, is_hidden_t hidden);
    IWiInfo(uint32_t num_to_print, string_view_utf8 label, is_hidden_t hidden = is_hidden_t::no, uint16_t id_icon = 0);

    virtual void click(IWindowMenu &window_menu) {}
};

template <size_t INFO_LEN>
class WiInfo : public AddSuper<IWiInfo> {
    char information[INFO_LEN];

public:
    WiInfo(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : AddSuper<IWiInfo>(label, id_icon, INFO_LEN, enabled, hidden) {}
    WiInfo(uint32_t num_to_print, string_view_utf8 label, is_hidden_t hidden = is_hidden_t::no, uint16_t id_icon = 0)
        : WiInfo(label, id_icon, is_enabled_t::yes, hidden) {
        itoa(num_to_print, information, 10);
    }

    invalidate_t ChangeInformation(const char *str) {
        if (strncmp(information, str, INFO_LEN)) {
            strlcpy(information, str, INFO_LEN);
            information[INFO_LEN - 1] = 0;
            return invalidate_t::yes;
        }
        return invalidate_t::no;
    }

    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override {
        printInfo(extension_rect, color_back, _(information));
    }
    static constexpr size_t GetInfoLen() { return INFO_LEN; }
};

//Dev version of info
template <size_t INFO_LEN>
class WiInfoDev : public AddSuper<WiInfo<INFO_LEN>> {
public:
    WiInfoDev(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled = is_enabled_t::yes)
        : AddSuper<WiInfo<INFO_LEN>>(label, id_icon, enabled, is_hidden_t::dev) {}
    WiInfoDev(uint32_t num_to_print, string_view_utf8 label, uint16_t id_icon = 0)
        : AddSuper<WiInfo<INFO_LEN>>(num_to_print, label, is_hidden_t::dev, id_icon) {}
};

using WI_INFO_t = WiInfo<GuiDefaults::infoDefaultLen>;
using WI_INFO_DEV_t = WiInfoDev<GuiDefaults::infoDefaultLen>;
