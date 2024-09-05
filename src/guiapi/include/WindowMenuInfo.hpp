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
    IWiInfo(const string_view_utf8 &value, const string_view_utf8 &label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);

    inline const string_view_utf8 &value() const {
        return value_;
    }

    /// Updates extension width to match the info value
    void update_extension_width();

    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;

protected:
    string_view_utf8 value_;
};

/// IWiInfo with rebindable string view value
class WiInfoString : public IWiInfo {

public:
    using IWiInfo::IWiInfo;

    void set_value(const string_view_utf8 &set);
};

/// IWiInfo working over a non-rebindable mutable string buffer
class WiInfoArray : public IWiInfo {

public:
    WiInfoArray(std::span<char> value_span, const string_view_utf8 &label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : IWiInfo({}, label, id_icon, enabled, hidden)
        , value_span_(value_span) {}

public:
    void ChangeInformation(const char *str) {
        // -1 because value_span_ last char is always terminating \0 (would cause mismatch when cropped)
        if (strncmp(value_span_.data(), str, value_span_.size() - 1) == 0) {
            return;
        }

        strlcpy(value_span_.data(), str, value_span_.size());
        value_ = string_view_utf8::MakeRAM(value_span_.data()); // Force update to reset cached string size
        update_extension_width();
        InValidateExtension();
    }

private:
    std::span<char> value_span_;
};

/// WiInfoArray, including the buffer
template <size_t INFO_LEN>
class WiInfo : public WiInfoArray {

public:
    WiInfo(const string_view_utf8 &label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : WiInfoArray(value_array_, label, id_icon, enabled, hidden) {}

    WiInfo(uint32_t num_to_print, const string_view_utf8 &label, is_hidden_t hidden = is_hidden_t::no, const img::Resource *id_icon = nullptr)
        : WiInfo(label, id_icon, is_enabled_t::yes, hidden) {
        decltype(value_array_) buf;
        itoa(num_to_print, buf.data(), 10);
        ChangeInformation(buf.data());
    }

    using WiInfoArray::ChangeInformation;

    void ChangeInformation(const string_view_utf8 &str) {
        decltype(value_array_) buf;
        str.copyToRAM(buf.data(), buf.size());
        ChangeInformation(buf.data());
    }

    static constexpr size_t GetInfoLen() { return INFO_LEN; }

protected:
    std::array<char, INFO_LEN> value_array_ { 0 };
};

using WI_INFO_t = WiInfo<GuiDefaults::infoDefaultLen>;
