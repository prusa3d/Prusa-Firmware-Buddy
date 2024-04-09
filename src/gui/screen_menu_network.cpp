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

void ScreenMenuNetwork::refresh_address() {
    const uint32_t active_netdev = netdev_get_active_id();
    Item<MI_HOSTNAME>().ChangeInformation(active_netdev == NETDEV_ESP_ID ? config_store().wifi_hostname.get_c_str() : config_store().lan_hostname.get_c_str());
}

void ScreenMenuNetwork::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        update_all_updatable_items();
        refresh_address();
    }

    SuperWindowEvent(sender, event, param);
}
