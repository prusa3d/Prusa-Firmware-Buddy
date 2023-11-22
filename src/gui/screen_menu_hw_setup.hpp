/**
 * @file screen_menu_hw_setup.hpp
 */

#pragma once

#include "screen_menu.hpp"

class MI_STEEL_SHEETS : public IWindowMenuItem {
    static constexpr const char *const label = N_("Steel Sheets");

public:
    MI_STEEL_SHEETS();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

using ScreenMenuHwSetup__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_STEEL_SHEETS>;

class ScreenMenuHwSetup : public ScreenMenuHwSetup__ {
public:
    constexpr static const char *label = N_("HW Setup");
    ScreenMenuHwSetup();
};
