/**
 * @file screen_menu_statistics.cpp
 */
#include "screen_menu_statistics.hpp"
#include "DialogMoveZ.hpp"
#include "img_resources.hpp"

ScreenMenuStatistics::ScreenMenuStatistics()
    : ScreenMenuStatistics__(_(label)) {
    EnableLongHoldScreenAction();
    header.SetIcon(&img::info_16x16);
}

void ScreenMenuStatistics::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    SuperWindowEvent(sender, event, param);
}
