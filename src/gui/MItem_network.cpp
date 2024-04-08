#include <guiconfig/guiconfig.h>
#include "MItem_network.hpp"
#include "wui_api.h"
#include "netdev.h"
#include "ScreenHandler.hpp"
#include "marlin_client.hpp"

MI_WIFI_STATUS_t::MI_WIFI_STATUS_t()
    : WI_INFO_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_WIFI_INIT_t::MI_WIFI_INIT_t()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIFI_INIT_t::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("M997 S1 O");
}

MI_WIFI_CREDENTIALS_t::MI_WIFI_CREDENTIALS_t()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIFI_CREDENTIALS_t::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("M1587");
}

MI_WIFI_CREDENTIALS_INI_FILE_t::MI_WIFI_CREDENTIALS_INI_FILE_t()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIFI_CREDENTIALS_INI_FILE_t::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("M1587 I");
}

MI_NET_INTERFACE_t::MI_NET_INTERFACE_t()
    : WI_SWITCH_t(0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_eth), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_wifi)) {
    if (netdev_get_active_id() == NETDEV_ESP_ID) {
        this->SetIndex(1);
    } else {
        this->SetIndex(0);
    }
}

void MI_NET_INTERFACE_t::OnChange([[maybe_unused]] size_t old_index) {
    netdev_set_active_id(this->index);
}

MI_HOSTNAME::MI_HOSTNAME()
    : WiInfo<config_store_ns::lan_hostname_max_len + 1>(_(label), nullptr, is_enabled_t::yes,
#if defined(USE_ST7789) || defined(USE_MOCK_DISPLAY)
        is_hidden_t::dev
#elif defined(USE_ILI9488)
        is_hidden_t::no
#endif
    ) {
}

MI_NET_IP::MI_NET_IP(uint32_t device_id)
    : WI_SWITCH_t(netdev_get_ip_obtained_type(device_id), string_view_utf8::MakeCPUFLASH(label), nullptr, is_enabled_t::yes, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_DHCP), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_static))
    , device_id(device_id) {
}

void MI_NET_IP::OnChange([[maybe_unused]] size_t old_index) {
    if (index == NETDEV_STATIC) {
        netdev_set_static(device_id);
    } else {
        netdev_set_dhcp(device_id);
    }
}

MI_NET_IP_VER_t::MI_NET_IP_VER_t()
    : WI_SWITCH_t(0, _(label), nullptr, is_enabled_t::no, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_v4), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_v6)) {
    this->index = 0;
}

MI_IP4_ADDR::MI_IP4_ADDR()
    : WiInfo<ADDR_LEN>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_IP4_NMSK::MI_IP4_NMSK()
    : WiInfo<ADDR_LEN>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_IP4_GWAY::MI_IP4_GWAY()
    : WiInfo<ADDR_LEN>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

MI_MAC_ADDR::MI_MAC_ADDR()
    : WiInfo<MAC_LEN>(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
