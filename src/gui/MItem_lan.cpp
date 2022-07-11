/**
 * @file MItem_lan.cpp
 * @author Radek Vana
 * @date 2021-10-31
 */

#include "MItem_lan.hpp"
#include "wui_api.h"
#include "netdev.h"
#include "ScreenHandler.hpp"
#include "marlin_client.h"

MI_WIFI_STATUS_t::MI_WIFI_STATUS_t()
    : WI_INFO_t(_(label), 0, is_enabled_t::yes, is_hidden_t::dev) {
}

MI_WIFI_INIT_t::MI_WIFI_INIT_t()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIFI_INIT_t::click(IWindowMenu &window_menu) {
    marlin_gcode("M997 S1 O");
}

MI_WIFI_CREDENTIALS_t::MI_WIFI_CREDENTIALS_t()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIFI_CREDENTIALS_t::click(IWindowMenu &window_menu) {
    marlin_gcode("M1587");
}

MI_WIFI_CREDENTIALS_INI_FILE_t::MI_WIFI_CREDENTIALS_INI_FILE_t()
    : WI_LABEL_t(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_WIFI_CREDENTIALS_INI_FILE_t::click(IWindowMenu &window_menu) {
    marlin_gcode("M1587 I");
}

MI_NET_INTERFACE_t::MI_NET_INTERFACE_t()
    : WI_SWITCH_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_off), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_eth), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_wifi)) {
    if (netdev_get_active_id() == NETDEV_ESP_ID) {
        this->index = 2;
    } else if (netdev_get_active_id() == NETDEV_ETH_ID) {
        this->index = 1;
    } else {
        this->index = 0;
    }
}

void MI_NET_INTERFACE_t::OnChange(size_t old_index) {
    uint32_t param = EventMask::value + (this->index == 0 ? 2 : this->index - 1);
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)param);
}

MI_NET_IP_t::MI_NET_IP_t()
    : WI_SWITCH_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::yes, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_DHCP), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_static)) {
    this->index = netdev_get_ip_obtained_type(netdev_get_active_id()) == NETDEV_DHCP
        ? 0
        : 1;
}

void MI_NET_IP_t::OnChange(size_t old_index) {
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, (void *)(EventMask::value | this->index));
}

MI_NET_IP_VER_t::MI_NET_IP_VER_t()
    : WI_SWITCH_t(0, string_view_utf8::MakeCPUFLASH((const uint8_t *)label), 0, is_enabled_t::no, is_hidden_t::no, string_view_utf8::MakeCPUFLASH((const uint8_t *)str_v4), string_view_utf8::MakeCPUFLASH((const uint8_t *)str_v6)) {
    this->index = 0;
}

MI_IP4_ADDR::MI_IP4_ADDR()
    : WiInfo<ADDR_LEN>(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

MI_IP4_NMSK::MI_IP4_NMSK()
    : WiInfo<ADDR_LEN>(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

MI_IP4_GWAY::MI_IP4_GWAY()
    : WiInfo<ADDR_LEN>(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}

MI_MAC_ADDR::MI_MAC_ADDR()
    : WiInfo<MAC_LEN>(_(label), 0, is_enabled_t::yes, is_hidden_t::no) {
}
