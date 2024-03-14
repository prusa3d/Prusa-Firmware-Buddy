/**
 * @file menu_item_event_dispatcher.cpp
 */

#include "menu_item_event_dispatcher.hpp"
#include "ScreenHandler.hpp"

void MI_event_dispatcher::click(IWindowMenu & /*window_menu*/) {
    // no way to change header at this level, have to dispatch event
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CLICK, (void *)this); // WI_LABEL is not a window, cannot set sender param
}
