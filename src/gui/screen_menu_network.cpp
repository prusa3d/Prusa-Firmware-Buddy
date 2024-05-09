/**
 * @file screen_menu_network.cpp
 */
#include "screen_menu_network.hpp"
#include "printers.h"
#include "DialogMoveZ.hpp"
#include "wui_api.h"
#include "netdev.h"
#include <http_lifetime.h>

ScreenMenuNetwork::ScreenMenuNetwork()
    : ScreenMenuNetwork__(_(label)) {
}

void ScreenMenuNetwork::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        update_all_updatable_items();
    }

    ScreenMenu::windowEvent(sender, event, param);
}
