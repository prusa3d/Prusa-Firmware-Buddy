#pragma once

#include <guitypes.hpp>
#include <i_window_menu_item.hpp>
#include <str_utils.hpp>

/// Menu item that executes a provided gcode when clicked on
class MenuItemGcodeAction : public IWindowMenuItem {
public:
    MenuItemGcodeAction(const string_view_utf8 &label, ConstexprString gcode, const img::Resource *icon = nullptr);

protected:
    void click(IWindowMenu &) override;

private:
    const ConstexprString gcode;
};
