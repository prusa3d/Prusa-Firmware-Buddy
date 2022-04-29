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

class MI_WIFI_INIT_t : public WI_LABEL_t {
    constexpr static const char *const label = N_("Initialise");

public:
    MI_WIFI_INIT_t();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_WIFI_CREDENTIALS_t : public WI_LABEL_t {
    constexpr static const char *const label = N_("Credentials");

public:
    MI_WIFI_CREDENTIALS_t();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_NET_INTERFACE_t : public WI_SWITCH_t<3> {
    constexpr static const char *const label = "Network Interface"; //do not translate

    constexpr static const char *str_off = N_("None");
    constexpr static const char *str_eth = "Eth";    //do not translate
    constexpr static const char *str_wifi = "Wi-Fi"; //do not translate

public:
    enum EventMask { value = 1 << 16 };

public:
    MI_NET_INTERFACE_t();
    virtual void OnChange(size_t old_index) override;
};

class MI_NET_IP_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = "LAN IPv4"; //do not translate

    constexpr static const char *str_static = "static"; //do not translate
    constexpr static const char *str_DHCP = "DHCP";     //do not translate

public:
    enum EventMask { value = 1 << 17 };

public:
    MI_NET_IP_t();
    virtual void OnChange(size_t old_index) override;
};
