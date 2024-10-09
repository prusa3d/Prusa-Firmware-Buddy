/**
 * @file WindowItemFormatableLabel.hpp
 * @brief Derived from WI_LABEL/IWindowMenuItem
 */

#pragma once

#include "i_window_menu_item.hpp"

#include <span>
#include <functional>

class WI_LAMBDA_LABEL_t : public IWindowMenuItem {
protected:
    static constexpr Font InfoFont = GuiDefaults::FontMenuSpecial;
    static constexpr uint16_t icon_width = 16;

    using PrintFunction = stdext::inplace_function<void(const std::span<char> &)>;
    const PrintFunction printAs;

protected:
    void printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const override;

public:
    WI_LAMBDA_LABEL_t(const string_view_utf8 &label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, const PrintFunction &printAs);
};

/// This WI has lambda function for formatting the extension
/// \tparam ValueType must have not, equals, and assignment operators and copy constructor
template <class ValueType>
class WI_FORMATABLE_LABEL_t : public WI_LAMBDA_LABEL_t {

protected:
    ValueType value;

public:
    WI_FORMATABLE_LABEL_t(const string_view_utf8 &label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, ValueType initVal, const PrintFunction &printAs)
        : WI_LAMBDA_LABEL_t(label, icon, enabled, hidden, printAs)
        , value(initVal) {
    }

    void UpdateValue(ValueType val) {
        if (value != val) {
            value = val;
            InValidateExtension();
        }
    }
};
