/**
 * @file WindowMenuItems.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuItems.hpp"
#include "resource.h"
#include "ScreenHandler.hpp"

MI_RETURN::MI_RETURN()
    : WI_LABEL_t(_(label), &png::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    Screens::Access()->Close();
}

MI_TEST_DISABLED_RETURN::MI_TEST_DISABLED_RETURN()
    //just for test (in debug), do not translate
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), &png::folder_up_16x16, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_DISABLED_RETURN::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Close();
}
