/// @file
#pragma once

#include <i_window_menu_item.hpp>
#include <window_menu_virtual.hpp>

namespace dialog_menu_virtual {

class MenuItem final : public IWindowMenuItem {

public:
    /// General purpose params - typically used for the label
    StringViewUtf8Parameters<16> params;

    /// Process selecting an item in the DialogMenuVirtual:
    /// - Propagate the clicked item index to the execute() return value
    /// - Close the menu
    static void handle_click(IWindowMenuItem &item, IWindowMenu &menu);

protected:
    void click(IWindowMenu &menu) override;
};

using MenuBase = WindowMenuVirtualSized<WindowMenuVirtualBase::default_item_buffer_size, sizeof(MenuItem)>;
using ItemVariant = MenuBase::ItemVariant;

struct Config {
    string_view_utf8 title;
    int item_count;
    int focused_item = -1;
    stdext::inplace_function<void(int item_index, ItemVariant &)> setup_f;
};

/// Blockingly executes a dialog that shows a menu (WindowMenuVirtual) based on the provided @p configF
/// @returns index of the item that was selected from the menu or nullopt if the menu was exited (for example by a swipe)
std::optional<int> execute(const Config &config);

} // namespace dialog_menu_virtual
