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
    refresh_address();
}

void ScreenMenuNetwork::refresh_address() {
    const uint32_t active_netdev = netdev_get_active_id();
    netdev_status_t n_status = netdev_get_status(active_netdev);
    Item<MI_HOSTNAME>().ChangeInformation(active_netdev == NETDEV_ESP_ID ? config_store().wifi_hostname.get_c_str() : config_store().lan_hostname.get_c_str());
    if (n_status == NETDEV_NETIF_UP || n_status == NETDEV_NETIF_NOADDR) {
        char str[ADDR_LEN];
        lan_t ethconfig = {};
        netdev_get_ipv4_addresses(active_netdev, &ethconfig);
        stringify_address_for_screen(str, sizeof(str), ethconfig, ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4));
        Item<MI_IP4_ADDR>().ChangeInformation(str);
    } else {
        Item<MI_IP4_ADDR>().ChangeInformation(UNKNOWN_ADDR);
    }
}

void ScreenMenuNetwork::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        uint32_t action = ((uint32_t)param) & 0xFFFF;
        uint32_t type = ((uint32_t)param) & 0xFFFF0000;
        switch (type) {
        case MI_NET_INTERFACE_t::EventMask::value:
            netdev_set_active_id(action);
            // TODO: By now, the network is not yet fully reconfigured. Do
            // it periodically (below)? Have some notification?
            refresh_address();
            break;
        }
    } else if (event == GUI_event_t::LOOP) {
        refresh_address();
    }
}
