/**
 * @file screen_menu_user_interface.cpp
 */

#include "screen_menu_user_interface.hpp"
#include "gcode_info.hpp"
#include "DialogMoveZ.hpp"

void ScreenMenuUserInterface::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}

ScreenMenuUserInterface::ScreenMenuUserInterface()
    : ScreenMenuUserInterface__(_(label)) {
    EnableLongHoldScreenAction();
}
