/**
 * @file screen_menu_lan_settings.cpp
 */

#include "gui.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "wui_api.h"
#include "RAII.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "netdev.h"
#include "network_gui_tools.hpp"
#include "MItem_lan.hpp"
#include <http_lifetime.h>
#include <espif.h>
#include "marlin_client.h"

// Container for this base class contains all MI from both ETH and WIFI screen
// There can be MI, that will not be used in derived class (MI_WIFI_... won't be used in ETH Screen)
// This is a solution to the problem that base class container have to define what MIs will be used, but derived classes will have slightly different ones.
using Screen = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_WIFI_STATUS_t, MI_WIFI_INIT_t, MI_WIFI_CREDENTIALS_INI_FILE_t, MI_WIFI_CREDENTIALS_t, MI_NET_IP_VER_t, MI_NET_IP_t, MI_IP4_ADDR, MI_IP4_NMSK, MI_IP4_GWAY, MI_MAC_ADDR>;

class ScreenMenuConnectionBase : public Screen {

    uint32_t dev_id;
    bool mac_init;
    bool msg_shown;

public:
    ScreenMenuConnectionBase(uint32_t dev_id, const char *label)
        : Screen(_(label))
        , dev_id(dev_id)
        , mac_init(false)
        , msg_shown(false) {
        refresh_addresses();
    }

protected:
    void refresh_addresses();
    void show_msg();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

void ScreenMenuConnectionBase::refresh_addresses() {
    bool refresh = false;
    char str[ADDR_LEN];

    if (!mac_init) {
        mac_init = true;

        mac_address_t mac;
        get_MAC_address(&mac, dev_id);
        if (Item<MI_MAC_ADDR>().ChangeInformation(mac[0] ? mac : UNKNOWN_MAC) == invalidate_t::yes) {
            refresh = true;
        }
    }

    if (netdev_get_status(dev_id) == NETDEV_NETIF_UP) {
        lan_t ethconfig = {};
        netdev_get_ipv4_addresses(dev_id, &ethconfig);

        stringify_address_for_screen(str, sizeof(str), ethconfig, ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4));
        if (Item<MI_IP4_ADDR>().ChangeInformation(str) == invalidate_t::yes) {
            refresh = true;
        }
        stringify_address_for_screen(str, sizeof(str), ethconfig, ETHVAR_MSK(ETHVAR_LAN_MSK_IP4));
        if (Item<MI_IP4_NMSK>().ChangeInformation(str) == invalidate_t::yes) {
            refresh = true;
        }
        stringify_address_for_screen(str, sizeof(str), ethconfig, ETHVAR_MSK(ETHVAR_LAN_GW_IP4));
        if (Item<MI_IP4_GWAY>().ChangeInformation(str) == invalidate_t::yes) {
            refresh = true;
        }

        if (Item<MI_WIFI_STATUS_t>().ChangeInformation("UP") == invalidate_t::yes) {
            refresh = true;
        }
    } else {
        const char *msg = UNKNOWN_ADDR;
        if (Item<MI_IP4_ADDR>().ChangeInformation(msg) == invalidate_t::yes) {
            refresh = true;
        }
        msg = UNKNOWN_ADDR;
        if (Item<MI_IP4_NMSK>().ChangeInformation(msg) == invalidate_t::yes) {
            refresh = true;
        }
        msg = UNKNOWN_ADDR;
        if (Item<MI_IP4_GWAY>().ChangeInformation(msg) == invalidate_t::yes) {
            refresh = true;
        }

        switch (esp_link_state()) {
        case EspLinkState::Init:
            switch (esp_fw_state()) {
            case EspFwState::Flashing:
            case EspFwState::NoFirmware:
            case EspFwState::WrongVersion:
                if (Item<MI_WIFI_STATUS_t>().ChangeInformation("!FW") == invalidate_t::yes) {
                    refresh = true;
                }
                break;
            case EspFwState::NoEsp:
                if (Item<MI_WIFI_STATUS_t>().ChangeInformation("Gone") == invalidate_t::yes) {
                    refresh = true;
                }
                break;
            case EspFwState::Ok:
                if (Item<MI_WIFI_STATUS_t>().ChangeInformation("Down") == invalidate_t::yes) {
                    refresh = true;
                }
                break;
            case EspFwState::Unknown:
                if (Item<MI_WIFI_STATUS_t>().ChangeInformation("???") == invalidate_t::yes) {
                    refresh = true;
                }
                break;
            }
            break;
        case EspLinkState::NoAp:
            if (Item<MI_WIFI_STATUS_t>().ChangeInformation("NO AP") == invalidate_t::yes) {
                refresh = true;
            }
            break;
        case EspLinkState::Down:
            if (Item<MI_WIFI_STATUS_t>().ChangeInformation("Down") == invalidate_t::yes) {
                refresh = true;
            }
            break;
        case EspLinkState::Up:
            if (Item<MI_WIFI_STATUS_t>().ChangeInformation("Up") == invalidate_t::yes) {
                refresh = true;
            }
            break;
        case EspLinkState::Silent:
            if (Item<MI_WIFI_STATUS_t>().ChangeInformation("Silent") == invalidate_t::yes) {
                refresh = true;
            }
            break;
        }
    }
    if (refresh) {
        Invalidate();
    }
}

void ScreenMenuConnectionBase::show_msg() {
    if (msg_shown)
        return;
    AutoRestore<bool> AR(msg_shown);
    msg_shown = true;
    MsgBoxError(_("Static IPv4 addresses were not set."), Responses_Ok);
}

void ScreenMenuConnectionBase::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        uint32_t action = ((uint32_t)param) & 0xFFFF;
        uint32_t type = ((uint32_t)param) & 0xFFFF0000;
        switch (type) {
        case MI_NET_IP_t::EventMask::value:
            if (action == NETDEV_STATIC) {
                netdev_set_static(dev_id);
            } else {
                netdev_set_dhcp(dev_id);
            }
            break;
        default:
            break;
        }
        refresh_addresses(); // This might have changed the IPs or even the whole interface.
    } else if (event == GUI_event_t::LOOP) {
        refresh_addresses();
    } else {
        SuperWindowEvent(sender, event, param);
    }
}

// ------------------- ETHERNET -----------------------

class ScreenMenuEthernetSettings : public ScreenMenuConnectionBase {
    constexpr static const char *eth_label = N_("ETHERNET SETTINGS");

public:
    ScreenMenuEthernetSettings()
        : ScreenMenuConnectionBase(NETDEV_ETH_ID, eth_label) {
        // MI for WIFI only have to be defined in the base class' container, but won't be used in ETH screen
        Hide<MI_WIFI_STATUS_t>();
        Hide<MI_WIFI_INIT_t>();
        Hide<MI_WIFI_CREDENTIALS_INI_FILE_t>();
        Hide<MI_WIFI_CREDENTIALS_t>();
    }
};

ScreenFactory::UniquePtr GetScreenMenuEthernetSettings() {
    return ScreenFactory::Screen<ScreenMenuEthernetSettings>();
}

// ------------------------ WIFI -----------------------------------

class ScreenMenuWifiSettings : public ScreenMenuConnectionBase {
    constexpr static const char *wifi_label = N_("WI-FI SETTINGS");

public:
    ScreenMenuWifiSettings()
        : ScreenMenuConnectionBase(NETDEV_ESP_ID, wifi_label) {
        if (marlin_is_printing()) {
            DisableItem<MI_WIFI_INIT_t>();
            DisableItem<MI_WIFI_CREDENTIALS_INI_FILE_t>();
            DisableItem<MI_WIFI_CREDENTIALS_t>();
        }
    }
};

ScreenFactory::UniquePtr GetScreenMenuWifiSettings() {
    return ScreenFactory::Screen<ScreenMenuWifiSettings>();
}
