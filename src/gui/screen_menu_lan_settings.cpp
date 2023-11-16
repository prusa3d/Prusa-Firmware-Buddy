/**
 * @file screen_menu_lan_settings.cpp
 */

#include "screen_menu_lan_settings.hpp"
#include "wui_api.h"
#include "RAII.hpp"
#include "ScreenHandler.hpp"
#include "netdev.h"
#include "network_gui_tools.hpp"
#include <http_lifetime.h>
#include <espif.h>
#include "marlin_client.hpp"
#include <option/has_embedded_esp32.h>

ScreenMenuConnectionBase::ScreenMenuConnectionBase(uint32_t dev_id, const char *label)
    : ScreenMenuConnectionBase__(_(label))
    , dev_id(dev_id)
    , mac_init(false)
    , msg_shown(false) {
    refresh_addresses();
}

void ScreenMenuConnectionBase::refresh_addresses() {
    char str[ADDR_LEN];

    if (!mac_init) {
        mac_init = true;

        mac_address_t mac;
        get_MAC_address(&mac, dev_id);
        Item<MI_MAC_ADDR>().ChangeInformation(mac[0] ? mac : UNKNOWN_MAC);
    }

    netdev_status_t n_status = netdev_get_status(dev_id);
    if (n_status == NETDEV_NETIF_UP || n_status == NETDEV_NETIF_NOADDR) {
        lan_t ethconfig = {};
        netdev_get_ipv4_addresses(dev_id, &ethconfig);

        stringify_address_for_screen(str, sizeof(str), ethconfig, ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4));
        Item<MI_IP4_ADDR>().ChangeInformation(str);

        stringify_address_for_screen(str, sizeof(str), ethconfig, ETHVAR_MSK(ETHVAR_LAN_MSK_IP4));
        Item<MI_IP4_NMSK>().ChangeInformation(str);

        stringify_address_for_screen(str, sizeof(str), ethconfig, ETHVAR_MSK(ETHVAR_LAN_GW_IP4));
        Item<MI_IP4_GWAY>().ChangeInformation(str);

        Item<MI_WIFI_STATUS_t>().ChangeInformation("UP");
    } else {
        const char *msg = UNKNOWN_ADDR;
        Item<MI_IP4_ADDR>().ChangeInformation(msg);

        msg = UNKNOWN_ADDR;
        Item<MI_IP4_NMSK>().ChangeInformation(msg);

        msg = UNKNOWN_ADDR;
        Item<MI_IP4_GWAY>().ChangeInformation(msg);

        switch (esp_link_state()) {
        case EspLinkState::Init:
            switch (esp_fw_state()) {
            case EspFwState::Flashing:
            case EspFwState::NoFirmware:
            case EspFwState::WrongVersion:
                Item<MI_WIFI_STATUS_t>().ChangeInformation("!FW");
                break;
            case EspFwState::NoEsp:
                Item<MI_WIFI_STATUS_t>().ChangeInformation("Gone");
                break;
            case EspFwState::Ok:
                Item<MI_WIFI_STATUS_t>().ChangeInformation("Down");
                break;
            case EspFwState::Unknown:
                Item<MI_WIFI_STATUS_t>().ChangeInformation("???");
                break;
            }
            break;
        case EspLinkState::NoAp:
            Item<MI_WIFI_STATUS_t>().ChangeInformation("NO AP");
            break;
        case EspLinkState::Down:
            Item<MI_WIFI_STATUS_t>().ChangeInformation("Down");
            break;
        case EspLinkState::Up:
            Item<MI_WIFI_STATUS_t>().ChangeInformation("Up");
            break;
        case EspLinkState::Silent:
            Item<MI_WIFI_STATUS_t>().ChangeInformation("Silent");
            break;
        }
    }
}

void ScreenMenuConnectionBase::show_msg() {
    if (msg_shown) {
        return;
    }
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
ScreenMenuEthernetSettings::ScreenMenuEthernetSettings()
    : ScreenMenuConnectionBase(NETDEV_ETH_ID, eth_label) {
    // MI for WIFI only have to be defined in the base class' container, but won't be used in ETH screen
    Hide<MI_WIFI_STATUS_t>();
    Hide<MI_WIFI_INIT_t>();
    Hide<MI_WIFI_CREDENTIALS_INI_FILE_t>();
    Hide<MI_WIFI_CREDENTIALS_t>();
}

// ------------------------ WIFI -----------------------------------
ScreenMenuWifiSettings::ScreenMenuWifiSettings()
    : ScreenMenuConnectionBase(NETDEV_ESP_ID, wifi_label) {

#if BOARD_VER_HIGHER_OR_EQUAL_TO(0, 5, 0)
    // This is temporary, remove once everyone has compatible hardware.
    // Requires new sandwich rev. 06 or rev. 05 with R83 removed.

    #if HAS_EMBEDDED_ESP32()
    Hide<MI_WIFI_INIT_t>();
    #endif
#endif

    if (marlin_client::is_printing()) {
        DisableItem<MI_WIFI_INIT_t>();
        DisableItem<MI_WIFI_CREDENTIALS_INI_FILE_t>();
        DisableItem<MI_WIFI_CREDENTIALS_t>();
    }
}
