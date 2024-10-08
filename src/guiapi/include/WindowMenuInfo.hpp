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
    static constexpr Font font = GuiDefaults::FontMenuSpecial;

public:
    IWiInfo(string_view_utf8 value, size_t max_characters, string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, ExtensionLikeLabel extension_like_label = ExtensionLikeLabel::no);

    inline string_view_utf8 value() const {
        return value_;
    }

    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;

protected:
    static uint16_t calculate_extension_width(ExtensionLikeLabel extension_like_label, size_t max_characters) {
        return max_characters * (extension_like_label == ExtensionLikeLabel::yes ? width(GuiDefaults::FontMenuItems) : width(font));
    }

protected:
    string_view_utf8 value_;
};

/// IWiInfo with rebindable string view value
class WiInfoString : public IWiInfo {

public:
    using IWiInfo::IWiInfo;

    void set_value(string_view_utf8 set);
};

/// IWiInfo working over a non-rebindable mutable string buffer
class WiInfoArray : public IWiInfo {

public:
    WiInfoArray(std::span<char> value_span, string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, ExtensionLikeLabel extension_like_label = ExtensionLikeLabel::no)
        : IWiInfo(string_view_utf8::MakeRAM(value_span.data()), value_span.size() - 1 /* exclude terminating \0 */, label, id_icon, enabled, hidden, extension_like_label)
        , value_span_(value_span) {}

public:
    void ChangeInformation(const char *str) {
        // -1 because value_span_ last char is always terminating \0 (would cause mismatch when cropped)
        if (strncmp(value_span_.data(), str, value_span_.size() - 1) == 0) {
            return;
        }

        strlcpy(value_span_.data(), str, value_span_.size());
        InValidateExtension();
    }

private:
    std::span<char> value_span_;
};

/// WiInfoArray, including the buffer
template <size_t INFO_LEN>
class WiInfo : public WiInfoArray {

public:
    WiInfo(string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, ExtensionLikeLabel extension_like_label = ExtensionLikeLabel::no)
        : WiInfoArray(value_array_, label, id_icon, enabled, hidden, extension_like_label) {}

    WiInfo(uint32_t num_to_print, string_view_utf8 label, is_hidden_t hidden = is_hidden_t::no, const img::Resource *id_icon = nullptr)
        : WiInfo(label, id_icon, is_enabled_t::yes, hidden) {
        itoa(num_to_print, value_array_.data(), 10);
    }

    using WiInfoArray::ChangeInformation;

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
