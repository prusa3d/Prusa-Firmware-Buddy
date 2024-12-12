/**
 * @file screen_menu_statistics.cpp
 */
#include "screen_menu_statistics.hpp"
#include "screen_move_z.hpp"
#include "img_resources.hpp"

ScreenMenuStatistics::ScreenMenuStatistics()
    : ScreenMenuStatistics__(_(label)) {
    EnableLongHoldScreenAction();
    header.SetIcon(&img::info_16x16);
}

void ScreenMenuStatistics::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        open_move_z_screen();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}
