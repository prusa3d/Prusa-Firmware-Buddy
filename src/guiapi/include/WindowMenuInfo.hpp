/**
 * @file WindowMenuInfo.hpp
 * @author Michal Rudolf
 * @brief WI_INFO_t menu item, derived from WI_LABEL/IWindowMenuItem
 * @date 2020-12-7
 */

#pragma once

#include <ranges>

#include "i_window_menu_item.hpp"
#include <common/utils/string_builder.hpp>
#include <guiconfig/GuiDefaults.hpp>

/*****************************************************************************/
// WI_INFO_t

/* MI_INFO_MAX_LEN
 *  Bigger value will chop off some text of MI's label in version screen.
 *  It could be increased, but runtime calculation of extention width is not supported (very complicated).
 *  For now, if string is longer than ..MAX_LEN, it will print only ..MAX_LEN - 1 (null-terminated) chars.
 */

class IWiInfo : public IWindowMenuItem {

public:
    static constexpr Font font = GuiDefaults::FontMenuSpecial;
    static Color extension_text_color(const IWindowMenuItem &item);

public:
    IWiInfo(const string_view_utf8 &value, const string_view_utf8 &label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);

    inline const string_view_utf8 &value() const {
        return value_;
    }

    /// @returns maximum reasonable extension width for the current platform
    static uint16_t max_extension_width();

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
    void ChangeInformation(std::string_view str) {
        // Crop to fit the buffer; -1 to ensure space for terminating 0
        str = str.substr(0, value_span_.size() - 1);

        if (std::string_view(value_span_.data()) == str) {
            return;
        }

        std::ranges::copy(str, value_span_.begin());
        value_span_[str.size()] = '\0';
        value_ = string_view_utf8::MakeRAM(value_span_.data());
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
    using Buffer = std::array<char, INFO_LEN>;

public:
    WiInfo(const string_view_utf8 &label)
        : WiInfoArray(value_array_, label) {}

    WiInfo(const string_view_utf8 &label, std::string_view value)
        : WiInfoArray(value_array_, label) {
        ChangeInformation(value);
    }

    WiInfo(const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no)
        : WiInfoArray(value_array_, label, id_icon, enabled, hidden) {}

    WiInfo(uint32_t num_to_print, const string_view_utf8 &label, is_hidden_t hidden = is_hidden_t::no, const img::Resource *id_icon = nullptr)
        : WiInfo(label, id_icon, is_enabled_t::yes, hidden) {
        ArrayStringBuilder<GetInfoLen()> sb;
        sb.append_printf("%" PRIu32, num_to_print).check();
        ChangeInformation(sb.str());
    }

    using WiInfoArray::ChangeInformation;

    void ChangeInformation(const string_view_utf8 &str) {
        Buffer buf;
        str.copyToRAM(buf.data(), buf.size());
        ChangeInformation(buf.data());
    }

    static constexpr size_t GetInfoLen() { return INFO_LEN; }

protected:
    Buffer value_array_ { 0 };
};

using WI_INFO_t = WiInfo<GuiDefaults::infoDefaultLen>;
