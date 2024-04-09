/**
 * @file screen_menu_network.cpp
 */
#include "screen_menu_network.hpp"
#include "printers.h"
#include "DialogMoveZ.hpp"
#include "wui_api.h"
#include "netdev.h"
#include "network_gui_tools.hpp"
#include <http_lifetime.h>

ScreenMenuNetwork::ScreenMenuNetwork()
    : ScreenMenuNetwork__(_(label)) {
    const uint32_t active_netdev = netdev_get_active_id();
    mac_address_t mac;
    get_MAC_address(&mac, active_netdev);
    Item<MI_MAC_ADDR>().ChangeInformation(mac);
}

void ScreenMenuNetwork::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        update_all_updatable_items();
    }

    SuperWindowEvent(sender, event, param);
}
