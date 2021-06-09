/*
 * lan_interface.h
 *
 *  Created on: Apr 12, 2021
 *      Author: joshy
 */

#pragma once

#include <stdbool.h>

typedef enum {
    BUDDY_LAN_ETH,
    BUDDY_LAN_WIFI
} lan_interface_type;

lan_interface_type get_lan_type();
