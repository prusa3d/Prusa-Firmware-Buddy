/**
 * @file screen_menu_statistics.cpp
 */
#include "screen_menu_statistics.hpp"
#include "DialogMoveZ.hpp"
#include "img_resources.hpp"
#include "app_metrics.h"

ScreenMenuStatistics::ScreenMenuStatistics()
    : ScreenMenuStatistics__(_(label)) {
    EnableLongHoldScreenAction();
    header.SetIcon(&img::info_16x16);
    AddItem(&uptime_label);
    updateUptime();
}

void ScreenMenuStatistics::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::HELD_RELEASED) {
        DialogMoveZ::Show();
        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}

void ScreenMenuStatistics::updateUptime() {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Uptime: %lu hours", ticks_s() / 3600);
    uptime_label.SetText(buffer);
}
