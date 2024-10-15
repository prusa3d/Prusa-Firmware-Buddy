/**
 * @file screen_menu_network.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include "MItem_network.hpp"
#include <option/buddy_enable_connect.h>

using ScreenMenuNetwork__ = ScreenMenu<EFooter::Off, MI_RETURN, MI_NET_INTERFACE_t, MI_NETWORK_STATUS, MI_WIFI_SETTINGS, MI_ETH_SETTINGS,
#if BUDDY_ENABLE_CONNECT()
    MI_PRUSA_CONNECT,
#endif
    MI_PRUSALINK,
    MI_METRICS_SETTINGS>;

class ScreenMenuNetwork : public ScreenMenuNetwork__ {
public:
    constexpr static const char *label = N_("NETWORK");

    ScreenMenuNetwork();
};
