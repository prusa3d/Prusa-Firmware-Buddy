/**
 * @file WindowItemFormatableLabel.hpp
 * @brief Derived from WI_LABEL/IWindowMenuItem
 */

#pragma once
#include "WindowMenuLabel.hpp"
#include <functional>

class WI_LAMBDA_LABEL_t : public AddSuper<WI_LABEL_t> {
protected:
    static constexpr font_t *&InfoFont = GuiDefaults::FontMenuSpecial;
    static constexpr uint16_t icon_width = 16;

    const std::function<void(char *)> printAs;

protected:
    constexpr static const char *NA = N_("N/A");
    constexpr static const char *NI = N_("Not initialized");

    void printExtension(Rect16 extension_rect, [[maybe_unused]] color_t color_text, color_t color_back, [[maybe_unused]] ropfn raster_op) const override {
        char text[GuiDefaults::infoDefaultLen];
        string_view_utf8 stringView;
        printAs(text);
        stringView = string_view_utf8::MakeRAM((uint8_t *)text);
        render_text_align(extension_rect, stringView, InfoFont, color_back,
            (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter(), false);
    }
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {}

public:
    WI_LAMBDA_LABEL_t(string_view_utf8 label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, std::function<void(char *)> printAs)
        : AddSuper<WI_LABEL_t>(label, icon ? icon_width : GuiDefaults::infoDefaultLen * InfoFont->w, icon, enabled, hidden)
        , printAs(printAs) {}
};

/// This WI has lambda function for formatting the extension
/// \tparam ValueType must have not, equals, and assignment operators and copy constructor
template <class ValueType>
class WI_FORMATABLE_LABEL_t : public WI_LAMBDA_LABEL_t {
protected:
    ValueType value;
    ValueType oldVal;

    virtual void click([[maybe_unused]] IWindowMenu &window_menu) {}

public:
    WI_FORMATABLE_LABEL_t(string_view_utf8 label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, ValueType initVal, std::function<void(char *)> printAs)
        : WI_LAMBDA_LABEL_t(label, icon, enabled, hidden, printAs)
        , value(initVal)
        , oldVal(initVal) {
    }
    void UpdateValue(ValueType val) {
        value = val;
        if (value != oldVal) {
            oldVal = value;
            InValidateExtension();
        }
    }
};
