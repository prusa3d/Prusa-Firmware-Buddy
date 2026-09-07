/// @file
#pragma once

#include <i_window_menu_item.hpp>

struct LoopGCodeInjectMenuItemArgs {
    /// The item will get disabled if the inject queue is occupied, and enabled otherwise
    bool update_enabled : 1;

    /// The item will get a spinning icon if the inject queue is occupied, and no icon otherwise
    bool update_icon : 1;

    /// If false, the menu item will not get enabled even if the inject queue is empty
    bool enabled : 1 = true;
};

/// To be inserted into Loop() of menu items that inject G-Codes as an action
/// Disables the menu item and sets a progress icon if the inject queue is not empty
void loop_gcode_inject_menu_item(IWindowMenuItem &item, LoopGCodeInjectMenuItemArgs args);
