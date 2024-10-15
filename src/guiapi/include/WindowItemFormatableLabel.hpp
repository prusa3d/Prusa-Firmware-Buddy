#pragma once

#include "i_window_menu_item.hpp"

#include <span>
#include <functional>

#include <timing.h>

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
    WI_LAMBDA_LABEL_t(const string_view_utf8 &label, const PrintFunction &printAs);
};

/// This WI has lambda function for formatting the extension
/// \tparam ValueType must have not, equals, and assignment operators and copy constructor
template <class ValueType>
class WI_FORMATABLE_LABEL_t : public WI_LAMBDA_LABEL_t {

public:
    WI_FORMATABLE_LABEL_t(const string_view_utf8 &label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, ValueType initVal, const PrintFunction &printAs)
        : WI_LAMBDA_LABEL_t(label, icon, enabled, hidden, printAs)
        , value_(initVal) {
    }

    WI_FORMATABLE_LABEL_t(const string_view_utf8 &label, const char *format, ValueType init_value)
        : WI_LAMBDA_LABEL_t(label, nullptr, is_enabled_t::yes, is_hidden_t::no, [this, format](const std::span<char> &buffer) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
            snprintf(buffer.data(), buffer.size(), format, value_);
#pragma GCC diagnostic pop
        })
        , value_(init_value) {
    }

    const ValueType &value() const {
        return value_;
    }

    void UpdateValue(ValueType val) {
        if (value_ != val) {
            value_ = val;
            InValidateExtension();
        }
    }

private:
    ValueType value_;
};

/// Menu item that automatically periodically calls \param getter in the specified interval, and if the value changes, udpates the label
template <typename T>
class MenuItemAutoUpdatingLabel : public WI_LAMBDA_LABEL_t {

public:
    using GetterFunction = T (*)(MenuItemAutoUpdatingLabel<T> *item);

    MenuItemAutoUpdatingLabel(const string_view_utf8 &label, const char *format, const GetterFunction &getter)
        : MenuItemAutoUpdatingLabel(label, print_function(format), getter) //
    {}

    MenuItemAutoUpdatingLabel(const string_view_utf8 &label, const PrintFunction &print_function, const GetterFunction &getter)
        : WI_LAMBDA_LABEL_t(label, print_function)
        , getter_(getter) //
    {}

    const T &value() const {
        return value_;
    }

protected:
    void Loop() override {
        const auto now = ticks_ms();
        if (ticks_diff(now, last_update_ms_) < update_interval_ms_) {
            return;
        }
        last_update_ms_ = now;

        const auto new_value = getter_(this);
        if (value_ == new_value) {
            return;
        }
        value_ = new_value;

        InValidateExtension();
    }

private:
    PrintFunction print_function(const char *format) const {
        return [this, format](const std::span<char> &buffer) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
            snprintf(buffer.data(), buffer.size(), format, value_);
#pragma GCC diagnostic pop
        };
    }

private:
    static constexpr uint16_t update_interval_ms_ = 1000;

    GetterFunction getter_;
    uint32_t last_update_ms_ = 0;
    T value_ {};
};
