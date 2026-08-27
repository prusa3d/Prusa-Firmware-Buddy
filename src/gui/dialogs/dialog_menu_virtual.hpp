/// @file
#pragma once

#include <i_window_menu_item.hpp>
#include <window_menu_virtual.hpp>

namespace dialog_menu_virtual {

class MenuItem;

struct Config {
    string_view_utf8 title;
    int item_count;
    int focused_item = -1;
    stdext::inplace_function<void(int item_index, MenuItem &)> setup_f;
};

class MenuItem final : public IWindowMenuItem {

public:
    /// Sets up the menu item as the "return" item - with appropriate icon and text
    void set_up_as_return();

    /// General purpose params - typically used for the label
    StringViewUtf8Parameters<16> params;

protected:
    void click(IWindowMenu &menu) override;
};

/// Blockingly executes a dialog that shows a menu (WindowMenuVirtual) based on the provided @p configF
/// @returns index of the item that was selected from the menu or nullopt if the menu was exited (for example by a swipe)
std::optional<int> execute(const Config &config);

} // namespace dialog_menu_virtual
