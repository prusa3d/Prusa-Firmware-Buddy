/**
 * @file screen_menu_cancel_object.cpp
 */

#include "screen_menu_cancel_object.hpp"
#include "ScreenHandler.hpp"

ScreenMenuCancelObject::ScreenMenuCancelObject()
    : ScreenMenuCancelObject__(_(label)) {}

MI_CO_CANCEL_OBJECT::MI_CO_CANCEL_OBJECT()
    : WI_LABEL_t(
        _(label), nullptr, []() {
            return cancelable.object_count > 0 ? is_enabled_t::yes : is_enabled_t::no;
        }(),
        is_hidden_t::no, expands_t::yes) {
}

void MI_CO_CANCEL_OBJECT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuCancelObject>);
}
