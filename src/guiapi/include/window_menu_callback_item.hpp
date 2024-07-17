#pragma once

#include <i_window_menu_item.hpp>

/// Simple menu item with a callback function
class WindowMenuCallbackItem : public IWindowMenuItem {

public:
    using Callback = stdext::inplace_function<void()>;

public:
    inline WindowMenuCallbackItem(const string_view_utf8 &label, Callback &&callback, const img::Resource *icon = nullptr, expands_t expands = expands_t::no)
        : IWindowMenuItem(label, icon, is_enabled_t::yes, is_hidden_t::no, expands)
        , callback(callback) {}

protected:
    inline void click(IWindowMenu &) override {
        if (callback) {
            callback();
        }
    }

public:
    /// Callback that gets called on click
    Callback callback;
};
