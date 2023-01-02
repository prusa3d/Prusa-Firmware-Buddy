/**
 * @file screen_menu_network.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "MItem_lan.hpp"

using ScreenMenuNetwork__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_PRUSALINK,
#ifdef BUDDY_ENABLE_CONNECT
    MI_PRUSA_CONNECT,
#endif
    MI_NET_INTERFACE_t, MI_IP4_ADDR, MI_MAC_ADDR, MI_ETH_SETTINGS, MI_WIFI_SETTINGS>;

class ScreenMenuNetwork : public ScreenMenuNetwork__ {
public:
    constexpr static const char *label = N_("NETWORK");

    ScreenMenuNetwork();

    void refresh_address();

    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;
};
