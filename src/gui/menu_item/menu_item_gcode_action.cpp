#include "menu_item_gcode_action.hpp"

#include <MItem_tools.hpp>

MenuItemGcodeAction::MenuItemGcodeAction(const string_view_utf8 &label, ConstexprString gcode)
    : IWindowMenuItem(label)
    , gcode(gcode) {}

void MenuItemGcodeAction::click(IWindowMenu &) {
    gui_try_gcode_with_msg(gcode);
}
