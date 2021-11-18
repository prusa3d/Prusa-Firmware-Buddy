/*
 * screen_lan_settings.cpp
 *
 *  Created on: Nov 27, 2019
 *      Author: Migi
 */

#include "gui.hpp"
#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "wui_api.h"
#include "netdev.h"
#include "RAII.hpp"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include <httpd.h>

/*****************************************************************************/
class MI_NET_INTERFACE_t : public WI_SWITCH_t<3> {
    constexpr static const char *const label = "Network Interface"; //do not translate

    constexpr static const char *str_off = "OFF";   //do not translate
    constexpr static const char *str_eth = "ETH";   //do not translate
    constexpr static const char *str_wifi = "WIFI"; //do not translate

public:
    enum EventMask { value = 1 << 16 };

public:
    MI_NET_INTERFACE_t()
        : WI_SWITCH_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_off), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_eth), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_wifi)) {
        if (netdev_get_active_id() == NETDEV_ESP_ID) {
            this->index = 2;
        } else if (netdev_get_active_id() == NETDEV_ETH_ID) {
            this->index = 1;
        } else {
            this->index = 0;
        }
    }

    virtual void OnChange(size_t old_index) override {
        uint32_t param = EventMask::value + (this->index == 0 ? 2 : this->index - 1);
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)param);
    }
};

class MI_NET_IP_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = "LAN IP"; //do not translate

    constexpr static const char *str_static = "static"; //do not translate
    constexpr static const char *str_DHCP = "DHCP";     //do not translate

public:
    enum EventMask { value = 1 << 17 };

public:
    MI_NET_IP_t()
        : WI_SWITCH_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_DHCP), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_static)) {
        this->index = netdev_get_ip_obtained_type(netdev_get_active_id()) == NETDEV_DHCP
            ? 0
            : 1;
    }

    virtual void OnChange(size_t old_index) override {
        Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)(EventMask::value | this->index));
    }
};

/*****************************************************************************/
//parent alias
using MenuContainer = WinMenuContainer<MI_RETURN, MI_NET_INTERFACE_t, MI_NET_IP_t>;

class ScreenMenuLanSettings : public AddSuperWindow<screen_t> {
    constexpr static const char *label = N_("LAN SETTINGS");
    static constexpr size_t helper_lines = 8;
    static constexpr int helper_font = IDR_FNT_SPECIAL;

    MenuContainer container;
    window_menu_t menu;
    window_header_t header;
    window_text_t help;

    lan_descp_str_t plan_str; //todo not initialized in constructor
    bool msg_shown;           //todo not initialized in constructor
    void refresh_addresses();
    void show_msg();

public:
    ScreenMenuLanSettings();

    uint32_t StringifyNetworkDevice(uint32_t netdev_id, lan_descp_str_t out_buffer);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

    static inline uint16_t get_help_h() {
        return helper_lines * (resource_font(helper_font)->h);
    }
};

ScreenMenuLanSettings::ScreenMenuLanSettings()
    : AddSuperWindow<screen_t>(nullptr, win_type_t::normal, is_closed_on_timeout_t::no)
    , menu(this, GuiDefaults::RectScreenBody - Rect16::Height_t(get_help_h()), &container)
    , header(this)
    , help(this, Rect16(GuiDefaults::RectScreen.Left(), uint16_t(GuiDefaults::RectScreen.Height()) - get_help_h(), GuiDefaults::RectScreen.Width(), get_help_h()), is_multiline::yes) {
    header.SetText(_(label));
    help.font = resource_font(helper_font);
    menu.GetActiveItem()->SetFocus(); // set focus on new item//containder was not valid during construction, have to set its index again
    CaptureNormalWindow(menu);        // set capture to list

    refresh_addresses();
    msg_shown = false;
}

uint32_t ScreenMenuLanSettings::StringifyNetworkDevice(uint32_t netdev_id, lan_descp_str_t buffer) {
    lan_t config = { 0 };
    char addr[IP4_ADDR_STR_SIZE], msk[IP4_ADDR_STR_SIZE], gw[IP4_ADDR_STR_SIZE];
    mac_address_t mac_str;
    uint8_t mac[6];
    int written = 0;

    netdev_get_ipv4_addresses(netdev_id, &config);
    netdev_get_MAC_address(netdev_id, mac);

    ip4addr_ntoa_r(&(config.addr_ip4), addr, IP4_ADDR_STR_SIZE);
    ip4addr_ntoa_r(&(config.msk_ip4), msk, IP4_ADDR_STR_SIZE);
    ip4addr_ntoa_r(&(config.gw_ip4), gw, IP4_ADDR_STR_SIZE);

    snprintf(mac_str, MAC_ADDR_STR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    written = snprintf(buffer, LAN_DESCP_SIZE, "IPv4 Address:\n%s\nIPv4 Netmask:\n%s\nIPv4 Gateway:\n%s\nMAC Address:\n%s",
        addr, msk, gw, mac_str);
    return written > 0 ? written : 0;
}
/*****************************************************************************/
//non static member function definition
void ScreenMenuLanSettings::refresh_addresses() {
    const uint32_t active_netdev_id = netdev_get_active_id();
    if (netdev_get_status(active_netdev_id) == NETDEV_NETIF_UP) {
        StringifyNetworkDevice(active_netdev_id, plan_str);
    } else {
        snprintf(plan_str, LAN_DESCP_SIZE, "NO CONNECTION\n");
    }
    help.text = string_view_utf8::MakeRAM((const uint8_t *)plan_str);
    help.Invalidate();
    gui_invalidate();
}

ScreenFactory::UniquePtr GetScreenMenuLanSettings() {
    return ScreenFactory::Screen<ScreenMenuLanSettings>();
}

void ScreenMenuLanSettings::show_msg() {
    if (msg_shown)
        return;
    AutoRestore<bool> AR(msg_shown);
    msg_shown = true;
    MsgBoxError(_("Static IPv4 addresses were not set."), Responses_Ok);
}

void ScreenMenuLanSettings::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        uint32_t action = ((uint32_t)param) & 0xFFFF;
        uint32_t type = ((uint32_t)param) & 0xFFFF0000;
        switch (type) {
        case MI_NET_INTERFACE_t::EventMask::value:
            httpd_close();
            netdev_set_down(netdev_get_active_id());
            netdev_set_active_id(action);
            netdev_set_up(action);
            httpd_reinit();
            break;
        case MI_NET_IP_t::EventMask::value:
            httpd_close();
            if (action == NETDEV_STATIC) {
                netdev_set_static(netdev_get_active_id());
            } else {
                netdev_set_dhcp(netdev_get_active_id());
            }
            httpd_reinit();
            break;
        default:
            break;
        }
    } else if (event == GUI_event_t::LOOP) {
        refresh_addresses();
    } else {
        SuperWindowEvent(sender, event, param);
    }
}
