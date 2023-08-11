/**
 * @file WindowMenuItems.cpp
 * @author Radek Vana
 * @date 2020-11-09
 */

#include "WindowMenuItems.hpp"
#include "ScreenHandler.hpp"
#include "img_resources.hpp"

MI_RETURN::MI_RETURN()
    : WI_LABEL_t(_(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_RETURN::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    Screens::Access()->Close();
}

MI_EXIT::MI_EXIT()
    : WI_LABEL_t(_(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_EXIT::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    Screens::Access()->Close();
}

MI_TEST_DISABLED_RETURN::MI_TEST_DISABLED_RETURN()
    // just for test (in debug), do not translate
    : WI_LABEL_t(string_view_utf8::MakeCPUFLASH((uint8_t *)label), &img::folder_up_16x16, is_enabled_t::no, is_hidden_t::no) {
}

void MI_TEST_DISABLED_RETURN::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Close();
}

WI_ICON_SWITCH_OFF_ON_t::WI_ICON_SWITCH_OFF_ON_t(bool index, string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
    : WI_ICON_SWITCH_t(size_t(index), label, id_icon, enabled, hidden, &img::switch_off_36x18, &img::switch_on_36x18) {}
