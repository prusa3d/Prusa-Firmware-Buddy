#include "screen_menu_network_settings.hpp"
#include "wui_api.h"
#include "RAII.hpp"
#include "ScreenHandler.hpp"
#include "netdev.h"
#include "network_gui_tools.hpp"
#include <http_lifetime.h>
#include <espif.h>
#include "marlin_client.hpp"
#include <option/has_embedded_esp32.h>

template <typename T, uint32_t dev_id>
void update_screen(T &menu) {
    menu.update_all_updatable_items();

    if (menu.template Item<MI_MAC_ADDR>().value()[0] == '\0') {
        mac_address_t mac;
        get_MAC_address(&mac, dev_id);
        menu.template Item<MI_MAC_ADDR>().ChangeInformation(mac[0] ? mac : UNKNOWN_MAC);
    }

    netdev_status_t n_status = netdev_get_status(dev_id);
    if (n_status == NETDEV_NETIF_UP || n_status == NETDEV_NETIF_NOADDR) {
        if constexpr (dev_id == NETDEV_ESP_ID) {
            menu.template Item<MI_WIFI_STATUS_t>().ChangeInformation(n_status == NETDEV_NETIF_UP ? _("Connected") : _("Link down"));
        }
    } else {
        if constexpr (dev_id == NETDEV_ESP_ID) {
            const char *state_str = [&]() -> const char * {
                switch (esp_link_state()) {

                case EspLinkState::Init:
                    switch (esp_fw_state()) {

                    case EspFwState::Flashing:
                    case EspFwState::NoFirmware:
                    case EspFwState::WrongVersion:
                        return N_("Flash ESP");

                    case EspFwState::NoEsp:
                        return N_("Gone");

                    case EspFwState::Ok:
                        return N_("Link down");

                    case EspFwState::Unknown:
                        return N_("???");
                    }
                    return nullptr;

                case EspLinkState::NoAp:
                    return N_("No AP");

                case EspLinkState::Up:
                    return N_("Connected");

                case EspLinkState::Silent:
                    return N_("ESP error");
                }

                return nullptr;
            }();
            menu.template Item<MI_WIFI_STATUS_t>().ChangeInformation(_(state_str));
        }
    }
}

// ------------------- ETHERNET -----------------------
ScreenMenuEthernetSettings::ScreenMenuEthernetSettings()
    : ScreenMenuEthernetSettings_(_(label)) {
    // MI for WIFI only have to be defined in the base class' container, but won't be used in ETH screen
    Item<MI_HOSTNAME>().ChangeInformation(config_store().lan_hostname.get_c_str());
}

void ScreenMenuEthernetSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        update_screen<ScreenMenuEthernetSettings, NETDEV_ETH_ID>(*this);
    }

    SuperWindowEvent(sender, event, param);
}

// ------------------------ WIFI -----------------------------------
ScreenMenuWifiSettings::ScreenMenuWifiSettings()
    : ScreenMenuWifiSettings_(_(label)) {

    if (marlin_client::is_printing()) {
        DisableItem<MI_WIFI_INIT_t>();
        DisableItem<MI_WIFI_CREDENTIALS_INI_FILE_t>();
        DisableItem<MI_WIFI_CREDENTIALS_t>();
    }
    Item<MI_HOSTNAME>().ChangeInformation(config_store().wifi_hostname.get_c_str());
}

void ScreenMenuWifiSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        update_screen<ScreenMenuWifiSettings, NETDEV_ESP_ID>(*this);
    }

    SuperWindowEvent(sender, event, param);
}
