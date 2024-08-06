/**
 * @file WindowItemFormatableLabel.hpp
 * @brief Derived from WI_LABEL/IWindowMenuItem
 */

#pragma once
#include "i_window_menu_item.hpp"
#include <functional>

class WI_LAMBDA_LABEL_t : public IWindowMenuItem {
protected:
    static constexpr Font InfoFont = GuiDefaults::FontMenuSpecial;
    static constexpr uint16_t icon_width = 16;

    const stdext::inplace_function<void(char *)> printAs;

protected:
    void printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const override {
        char text[GuiDefaults::infoDefaultLen];
        string_view_utf8 stringView;
        printAs(text);
        stringView = string_view_utf8::MakeRAM((uint8_t *)text);
        render_text_align(extension_rect, stringView, InfoFont, color_back,
            (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter(), false);
    }

public:
    WI_LAMBDA_LABEL_t(const string_view_utf8 &label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, stdext::inplace_function<void(char *)> printAs)
        : IWindowMenuItem(label, icon ? icon_width : GuiDefaults::infoDefaultLen * width(InfoFont), icon, enabled, hidden)
        , printAs(printAs) {}
};

/// This WI has lambda function for formatting the extension
/// \tparam ValueType must have not, equals, and assignment operators and copy constructor
template <class ValueType>
class WI_FORMATABLE_LABEL_t : public WI_LAMBDA_LABEL_t {
protected:
    ValueType value;

    virtual void click([[maybe_unused]] IWindowMenu &window_menu) {}

public:
    WI_FORMATABLE_LABEL_t(const string_view_utf8 &label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, ValueType initVal, stdext::inplace_function<void(char *)> printAs)
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
