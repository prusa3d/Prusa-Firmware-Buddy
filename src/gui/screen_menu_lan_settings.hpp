/**
 * @file screen_menu_lan_settings.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_lan.hpp"

// Container for this base class contains all MI from both ETH and WIFI screen
// There can be MI, that will not be used in derived class (MI_WIFI_... won't be used in ETH Screen)
// This is a solution to the problem that base class container have to define what MIs will be used, but derived classes will have slightly different ones.
using ScreenMenuConnectionBase__ = ScreenMenu<EFooter::Off, MI_RETURN, MI_WIFI_STATUS_t, MI_WIFI_INIT_t, MI_WIFI_CREDENTIALS_INI_FILE_t, MI_WIFI_CREDENTIALS_t, MI_NET_IP_VER_t, MI_NET_IP_t, MI_IP4_ADDR, MI_IP4_NMSK, MI_IP4_GWAY, MI_MAC_ADDR>;

class ScreenMenuConnectionBase : public ScreenMenuConnectionBase__ {

    uint32_t dev_id;
    bool mac_init;
    bool msg_shown;

public:
    ScreenMenuConnectionBase(uint32_t dev_id, const char *label);

protected:
    void refresh_addresses();
    void show_msg();
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};

// ------------------- ETHERNET -----------------------
class ScreenMenuEthernetSettings : public ScreenMenuConnectionBase {
    constexpr static const char *eth_label = N_("ETHERNET SETTINGS");

public:
    ScreenMenuEthernetSettings();
};

// ------------------------ WIFI -----------------------------------
class ScreenMenuWifiSettings : public ScreenMenuConnectionBase {
    constexpr static const char *wifi_label = N_("WI-FI SETTINGS");

public:
    ScreenMenuWifiSettings();
};
