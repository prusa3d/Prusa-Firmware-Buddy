/**
 * @file screen_menu_hw_setup.cpp
 */
#include "screen_menu_hw_setup.hpp"
#include "ScreenHandler.hpp"
#include "screen_menu_steel_sheets.hpp"

MI_STEEL_SHEETS::MI_STEEL_SHEETS()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {};

void MI_STEEL_SHEETS::click([[maybe_unused]] IWindowMenu &window_menu) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuSteelSheets>);
}

ScreenMenuHwSetup::ScreenMenuHwSetup()
    : ScreenMenuHwSetup__(_(label)) {
}
