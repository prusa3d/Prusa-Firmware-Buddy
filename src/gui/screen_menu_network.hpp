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
#include <option/has_esp.h>

using ScreenMenuNetwork__ = ScreenMenu<EFooter::Off,
    MI_RETURN,
#if HAS_ESP()
    MI_NET_INTERFACE_t,
#endif
    MI_NETWORK_STATUS,
#if HAS_ESP()
    MI_WIFI_SETTINGS,
#endif
    MI_ETH_SETTINGS,
    MI_NTP_VIA_DHCP,
    MI_NTP_ADDR,
#if BUDDY_ENABLE_CONNECT()
    MI_PRUSA_CONNECT,
#endif
#if !PRINTER_IS_PRUSA_iX()
    MI_PRUSALINK,
#endif
    MI_METRICS_SETTINGS>;

class ScreenMenuNetwork : public ScreenMenuNetwork__ {
public:
    constexpr static const char *label = N_("NETWORK");

    ScreenMenuNetwork();
};
