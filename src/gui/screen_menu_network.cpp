#include "gui.hpp"
#include "screen_menus.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "MItem_lan.hpp"
#include "wui_api.h"
#include "netdev.h"
#include "network_gui_tools.hpp"
#include <http_lifetime.h>
#include "printers.h"
#include "DialogMoveZ.hpp"

using Screen = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_PRUSALINK, MI_NET_INTERFACE_t, MI_IP4_ADDR, MI_MAC_ADDR, MI_ETH_SETTINGS, MI_WIFI_SETTINGS>;

class ScreenMenuNetwork : public Screen {
public:
    constexpr static const char *label = N_("NETWORK");

    ScreenMenuNetwork()
        : Screen(_(label)) {
        const uint32_t active_netdev = netdev_get_active_id();
        mac_address_t mac;
        get_MAC_address(&mac, active_netdev);
        Item<MI_MAC_ADDR>().ChangeInformation(mac);
        refresh_address();
    }

    void refresh_address() {
        const uint32_t active_netdev = netdev_get_active_id();
        if (netdev_get_status(active_netdev) == NETDEV_NETIF_UP) {
            char str[ADDR_LEN];
            lan_t ethconfig = {};
            netdev_get_ipv4_addresses(active_netdev, &ethconfig);
            stringify_address_for_screen(str, sizeof(str), ethconfig, ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4));
            if (Item<MI_IP4_ADDR>().ChangeInformation(str) == invalidate_t::yes) {
                Invalidate();
            }
        } else {
            if (Item<MI_IP4_ADDR>().ChangeInformation(UNKNOWN_ADDR) == invalidate_t::yes) {
                Invalidate();
            }
        }
    }

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override {
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
};

ScreenFactory::UniquePtr GetScreenMenuNetwork() {
    return ScreenFactory::Screen<ScreenMenuNetwork>();
}
