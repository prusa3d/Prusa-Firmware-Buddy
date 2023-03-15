/**
 * @file MItem_loadcell.cpp
 * @author Radek Vana
 * @date 2021-10-03
 */
#include "MItem_loadcell.hpp"
#include "printer_selftest.hpp"
#include "marlin_client.hpp"
#include "ScreenSelftest.hpp"
#include "ScreenHandler.hpp"

/*****************************************************************************/
//MI_TEST_LOADCELL
MI_TEST_LOADCELL::MI_TEST_LOADCELL()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_TEST_LOADCELL::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
    marlin_test_start(stmLoadcell);
}
