/**
 * @file screen_menu_system.cpp
 */
#include "screen_menu_system.hpp"
#include "screen_move_z.hpp"

void ScreenMenuSystem::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        open_move_z_screen();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}

ScreenMenuSystem::ScreenMenuSystem()
    : ScreenMenuSystem__(_(label)) {
    EnableLongHoldScreenAction();
}
