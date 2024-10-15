#pragma once

#include <netdev.h>

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_network.hpp"

// ------------------- ETHERNET -----------------------
using ScreenMenuEthernetSettings_ = ScreenMenu<EFooter::Off, MI_RETURN, //
    MI_NET_IP_VER_t,
    WMI_NET<MI_NET_IP, NETDEV_ETH_ID>,
    WMI_NET<MI_IP4_ADDR, NETDEV_ETH_ID>,
    WMI_NET<MI_IP4_NMSK, NETDEV_ETH_ID>,
    WMI_NET<MI_IP4_GWAY, NETDEV_ETH_ID>,
    MI_HOSTNAME,
    WMI_NET<MI_MAC_ADDR, NETDEV_ETH_ID> //
    >;

class ScreenMenuEthernetSettings final : public ScreenMenuEthernetSettings_ {
    constexpr static const char *label = N_("ETHERNET SETTINGS");

public:
    ScreenMenuEthernetSettings();
};

// ------------------------ WIFI -----------------------------------
using ScreenMenuWifiSettings_ = ScreenMenu<EFooter::Off, MI_RETURN, //
    MI_WIFI_STATUS_t, MI_WIFI_SSID, MI_WIFI_SETUP,
    MI_NET_IP_VER_t,
    WMI_NET<MI_NET_IP, NETDEV_ESP_ID>,
    WMI_NET<MI_IP4_ADDR, NETDEV_ESP_ID>,
    WMI_NET<MI_IP4_NMSK, NETDEV_ESP_ID>,
    WMI_NET<MI_IP4_GWAY, NETDEV_ESP_ID>,
    MI_HOSTNAME,
    WMI_NET<MI_MAC_ADDR, NETDEV_ESP_ID> //
    >;

class ScreenMenuWifiSettings final : public ScreenMenuWifiSettings_ {
    constexpr static const char *label = N_("WI-FI SETTINGS");

public:
    ScreenMenuWifiSettings();
};
