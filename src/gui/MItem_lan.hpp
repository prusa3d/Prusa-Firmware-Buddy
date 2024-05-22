/**
 * @file MItem_lan.hpp
 * @author Radek Vana
 * @brief lan menu items
 * @date 2021-10-03
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"

class MI_WIFI_STATUS_t : public WI_INFO_t {
    constexpr static const char *const label = N_("Status");

public:
    MI_WIFI_STATUS_t();
};

class MI_WIFI_INIT_t : public IWindowMenuItem {
    constexpr static const char *const label = N_("Setup Wi-Fi Module");

public:
    MI_WIFI_INIT_t();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_WIFI_CREDENTIALS_t : public IWindowMenuItem {
    constexpr static const char *const label = N_("Load Credentials");

public:
    MI_WIFI_CREDENTIALS_t();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_WIFI_CREDENTIALS_INI_FILE_t : public IWindowMenuItem {
    constexpr static const char *const label = N_("Create Credentials");

public:
    MI_WIFI_CREDENTIALS_INI_FILE_t();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_NET_INTERFACE_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = N_("Default");

    constexpr static const char *str_eth = "Eth"; // do not translate
    constexpr static const char *str_wifi = "Wi-Fi"; // do not translate

public:
    enum EventMask { value = 1 << 16 };

public:
    MI_NET_INTERFACE_t();
    virtual void OnChange(size_t old_index) override;
};

class MI_HOSTNAME : public WiInfo<config_store_ns::lan_hostname_max_len + 1> {
    // Printer's name within network
    constexpr static const char *const label { N_("Hostname") };

public:
    MI_HOSTNAME();
};
class MI_NET_IP_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = "LAN"; // do not translate

    constexpr static const char *str_static = "static"; // do not translate
    constexpr static const char *str_DHCP = "DHCP"; // do not translate

public:
    enum EventMask { value = 1 << 17 };

public:
    MI_NET_IP_t();
    virtual void OnChange(size_t old_index) override;
};

class MI_NET_IP_VER_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = N_("Protocol");

    constexpr static const char *str_v4 = "IPv4"; // do not translate
    constexpr static const char *str_v6 = "IPv6"; // do not translate

public:
    MI_NET_IP_VER_t();
    // virtual void OnChange(size_t old_index) override; //TODO
};

#define UNKNOWN_ADDR "N/A"
#define ADDR_LEN     sizeof("255.255.255.255")
#define UNKNOWN_MAC  "N/A"
#define MAC_LEN      sizeof("ff.ff.ff.ff.ff.ff")

class MI_IP4_ADDR : public WiInfo<ADDR_LEN> {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Address") : N_("IP");

public:
    MI_IP4_ADDR();
};

class MI_IP4_NMSK : public WiInfo<ADDR_LEN> {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Netmask") : N_("Mask");

public:
    MI_IP4_NMSK();
};

class MI_IP4_GWAY : public WiInfo<ADDR_LEN> {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Gateway") : N_("GW");

public:
    MI_IP4_GWAY();
};

class MI_MAC_ADDR : public WiInfo<MAC_LEN> {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("MAC Address") : N_("MAC");

public:
    MI_MAC_ADDR();
};
