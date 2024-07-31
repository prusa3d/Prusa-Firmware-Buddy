#include "window_menu_gcode_item.hpp"

#include <MItem_tools.hpp>

WindowMenuGCodeItem::WindowMenuGCodeItem(const string_view_utf8 &label, ConstexprString gcode)
    : IWindowMenuItem(label)
    , gcode(gcode) {}

void WindowMenuGCodeItem::click(IWindowMenu &) {
    gui_try_gcode_with_msg(gcode);
}
