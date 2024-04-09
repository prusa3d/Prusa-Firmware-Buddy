/**
 * @file WindowMenuInfo.hpp
 * @author Michal Rudolf
 * @brief WI_INFO_t menu item, derived from WI_LABEL/IWindowMenuItem
 * @date 2020-12-7
 */

#pragma once

#include "i_window_menu_item.hpp"
#include <guiconfig/GuiDefaults.hpp>

/*****************************************************************************/
// WI_INFO_t

/* MI_INFO_MAX_LEN
 *  Bigger value will chop off some text of MI's label in version screen.
 *  It could be increased, but runtime calculation of extention width is not supported (very complicated).
 *  For now, if string is longer than ..MAX_LEN, it will print only ..MAX_LEN - 1 (null-terminated) chars.
 */

class IWiInfo : public IWindowMenuItem {
    static constexpr Font InfoFont = GuiDefaults::FontMenuSpecial;
    static constexpr uint16_t icon_width = 16;

public:
    IWiInfo(std::span<char> value_buffer, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, ExtensionLikeLabel extension_like_label = ExtensionLikeLabel::no);

    inline auto value_buffer() const {
        return value_bufer_;
    }

    void click([[maybe_unused]] IWindowMenu &window_menu) override {}
    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;

    void ChangeInformation(const char *str);

protected:
    static uint16_t calculate_extension_width(ExtensionLikeLabel extension_like_label, size_t max_characters) {
        return max_characters * (extension_like_label == ExtensionLikeLabel::yes ? width(GuiDefaults::FontMenuItems) : width(InfoFont));
    }

protected:
    const std::span<char> value_bufer_;
};

template <size_t INFO_LEN>
class WiInfo : public IWiInfo {

public:
    WiInfo(string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden, ExtensionLikeLabel extension_like_label = ExtensionLikeLabel::no)
        : IWiInfo(value_array_, label, id_icon, enabled, hidden, extension_like_label) {}

    WiInfo(uint32_t num_to_print, string_view_utf8 label, is_hidden_t hidden = is_hidden_t::no, const img::Resource *id_icon = nullptr)
        : IWiInfo(value_array_, label, id_icon, is_enabled_t::yes, hidden) {
        itoa(num_to_print, value_array_.data(), 10);
    }

    using IWiInfo::ChangeInformation;
    void ChangeInformation(string_view_utf8 str) {
        decltype(value_array_) buf;
        str.copyToRAM(buf.data(), buf.size());
        ChangeInformation(buf.data());
    }

    static constexpr size_t GetInfoLen() { return INFO_LEN; }

protected:
    std::array<char, INFO_LEN> value_array_;
};

// Dev version of info
template <size_t INFO_LEN>
class WiInfoDev : public WiInfo<INFO_LEN> {
public:
    WiInfoDev(string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled = is_enabled_t::yes)
        : WiInfo<INFO_LEN>(label, id_icon, enabled, is_hidden_t::dev) {}
    WiInfoDev(uint32_t num_to_print, string_view_utf8 label, const img::Resource *id_icon = nullptr)
        : WiInfo<INFO_LEN>(num_to_print, label, is_hidden_t::dev, id_icon) {}
};

using WI_INFO_t = WiInfo<GuiDefaults::infoDefaultLen>;
using WI_INFO_DEV_t = WiInfoDev<GuiDefaults::infoDefaultLen>;
