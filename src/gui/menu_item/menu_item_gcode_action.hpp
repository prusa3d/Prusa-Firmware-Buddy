#pragma once

#include <i_window_menu_item.hpp>
#include <str_utils.hpp>

/// Menu item that executes a provided gcode when clicked on
class MenuItemGcodeAction : public IWindowMenuItem {
public:
    MenuItemGcodeAction(const string_view_utf8 &label, ConstexprString gcode);

protected:
    void click(IWindowMenu &) override;

private:
    const ConstexprString gcode;
};
