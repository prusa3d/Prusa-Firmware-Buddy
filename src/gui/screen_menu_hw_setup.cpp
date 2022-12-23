/**
 * @file screen_menu_hw_setup.cpp
 */
#include "screen_menu_hw_setup.hpp"
#include "ScreenHandler.hpp"
#include "screen_menus.hpp"

MI_STEEL_SHEETS::MI_STEEL_SHEETS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_STEEL_SHEETS::click(IWindowMenu &window_menu) {
    Screens::Access()->Open(GetScreenMenuSteelSheets);
}

ScreenMenuHwSetup::ScreenMenuHwSetup()
    : ScreenMenuHwSetup__(_(label)) {
}
