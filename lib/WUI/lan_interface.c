/*
 * lan_interface.c
 *
 *  Created on: Apr 12, 2021
 *      Author: joshy
 */
#include "lan_interface.h"
#include "netif_settings.h"
#include "wui_api.h"

lan_interface_type get_lan_type() {
    if (IS_LAN_INTERFACE_ETH(get_lan_flag()))
        return BUDDY_LAN_ETH;
    else
        return BUDDY_LAN_WIFI;
}
