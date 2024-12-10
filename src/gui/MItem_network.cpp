#include <guiconfig/guiconfig.h>

#include "MItem_network.hpp"
#include "MItem_tools.hpp"
#include <espif.h>
#include <dns.h>
#include "wui_api.h"
#include <netdev.h>
#include "ScreenHandler.hpp"
#include "marlin_client.hpp"
#include <dialog_text_input.hpp>
#include <wui.h>
#include <str_utils.hpp>
#include <window_msgbox.hpp>

namespace {
bool is_device_connected(netdev_status_t status) {
    return status == NETDEV_NETIF_UP || status == NETDEV_NETIF_NOADDR;
}

void ip2str(const ip4_addr_t &addr, char *buf, size_t len) {
    if (!addr.addr) {
        strlcpy(buf, "-", len);
        return;
    }

    ip4addr_ntoa_r(&addr, buf, len);
}
} // namespace

inline uint32_t NetDeviceID::get() const {
    return value_ == active_id ? netdev_get_active_id() : value_;
}

bool NetDeviceID::is_connected() const {
    return is_device_connected(netdev_get_status(get()));
}

MI_WIFI_STATUS_t::MI_WIFI_STATUS_t()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIFI_STATUS_t::Loop() {
    const netdev_status_t status = netdev_get_status(NETDEV_ESP_ID);
    if (is_device_connected(status)) {
        ChangeInformation(status == NETDEV_NETIF_UP ? _("Connected") : _("Link down"));
        return;
    }

    const char *state_str = []() -> const char * {
        switch (esp_link_state()) {

        case EspLinkState::Init:
            switch (esp_fw_state()) {
            case EspFwState::FlashingErrorNotConnected:
                // Need short string here
                return N_("FlashConn");

            case EspFwState::FlashingErrorOther:
                // Need short string here
                return N_("FlashErr");

            case EspFwState::NoFirmware:
            case EspFwState::WrongVersion:
                return N_("Flash ESP");

            case EspFwState::Scanning:
                // Wifi module is scanning for Access points
                return N_("Scanning");

            case EspFwState::NoEsp:
                return N_("Gone");

            case EspFwState::Ok:
                return N_("Link down");

            case EspFwState::Unknown:
                return N_("Unknown");
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
    ChangeInformation(_(state_str));
}

// ===================================================
// MI_WIFI_SSID
// ===================================================
MI_WIFI_SSID::MI_WIFI_SSID()
    : WiInfoString(string_view_utf8::MakeRAM(config_store().wifi_ap_ssid.get_c_str()), _(label)) {
}

// ===================================================
// MI_WIFI_SETUP
// ===================================================
MI_WIFI_SETUP::MI_WIFI_SETUP()
    : IWindowMenuItem(_(label)) {
}

void MI_WIFI_SETUP::click(IWindowMenu &) {
    gui_try_gcode_with_msg("M1703");
}

static constexpr const char *interface_items[] = {
    "Eth",
    "Wi-Fi",
};

MI_NET_INTERFACE_t::MI_NET_INTERFACE_t()
    : MenuItemSwitch(_("Active Interface"), interface_items, netdev_get_active_id() == NETDEV_ESP_ID) //
{
    set_translate_items(false);
}

void MI_NET_INTERFACE_t::OnChange([[maybe_unused]] size_t old_index) {
    netdev_set_active_id(this->index);
}

MI_HOSTNAME::MI_HOSTNAME()
    : WiInfo(_(label)) {
}

void MI_HOSTNAME::Loop() {
    ChangeInformation(config_store().hostname.get().data());
}

void MI_HOSTNAME::click(IWindowMenu &) {
    std::array<char, config_store_ns::lan_hostname_max_len + 1> hostname = config_store().hostname.get();

    for (bool hostname_is_valid = false; !hostname_is_valid;) {
        if (!DialogTextInput::exec(_("Hostname"), hostname)) {
            return;
        }

        hostname_is_valid = [&] {
            const auto len = strlen(hostname.data());
            if (len == 0) {
                return false;
            }

            if (hostname[0] == '-') {
                return false;
            }

            for (char *chp = hostname.data(); *chp; chp++) {
                const char ch = *chp;
                if (!isalnum(ch) && ch != '-') {
                    return false;
                }
            }

            return true;
        }();

        if (!hostname_is_valid) {
            MsgBoxError(_("Hostname is not valid. Following conditions must apply:\n- Not empty\n- Contains only characters 'a-z A-Z 0-9 -'\n- Not starting with '-'"), Responses_Ok);
        }
    }

    config_store().hostname.set(hostname);
    ChangeInformation(config_store().hostname.get().data());
    notify_reconfigure();
}

static constexpr const char *net_ip_values[] = {
    "DHCP",
    "static",
};

MI_NET_IP::MI_NET_IP(NetDeviceID device_id)
    : MenuItemSwitch(string_view_utf8::MakeCPUFLASH("LAN"), net_ip_values, 0)
    , device_id(device_id) //
{
    set_translate_items(false);
    this->SetIndex(netdev_get_ip_obtained_type(this->device_id()));
}

void MI_NET_IP::OnChange([[maybe_unused]] size_t old_index) {
    const auto dev_id = device_id();
    if (this->GetIndex() == NETDEV_STATIC) {
        netdev_set_static(dev_id);
    } else {
        netdev_set_dhcp(dev_id);
    }
}

static constexpr const char *protocol_values[] = {
    "IPv4",
    "IPv6",
};

MI_NET_IP_VER_t::MI_NET_IP_VER_t()
    : MenuItemSwitch(_("Protocol"), protocol_values, 0) //
{
    set_translate_items(false);
    set_is_enabled(false);
}

IMI_IP4_ADDR::IMI_IP4_ADDR(const char *label, NetDeviceID device_id, AddrType addr)
    : WiInfo(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , device_id(device_id)
    , addr(addr) {
}

void IMI_IP4_ADDR::Loop() {
    if (!device_id.is_connected()) {
        ChangeInformation(UNKNOWN_ADDR);
        return;
    }

    lan_t config = {};
    netdev_get_ipv4_addresses(device_id(), &config);

    std::array<char, ADDR_LEN> str;
    ip2str(config.*addr, str.data(), str.size());
    ChangeInformation(str.data());
}

MI_IP4_DNS1::MI_IP4_DNS1()
    : WiInfo(_(label)) {
}

void MI_IP4_DNS1::Loop() {
    std::array<char, ADDR_LEN> str;
    ip2str(*dns_getserver(0), str.data(), str.size());
    ChangeInformation(str.data());
}

MI_MAC_ADDR::MI_MAC_ADDR(NetDeviceID device_id)
    : WiInfo<MAC_LEN>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no)
    , device_id(device_id) {
}

void MI_MAC_ADDR::Loop() {
    mac_address_t mac;
    get_MAC_address(&mac, device_id());
    ChangeInformation(mac);
}
