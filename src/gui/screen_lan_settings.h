/*
 * screen_lan_settings.h
 *
 *  Created on: Dec 11, 2019
 *      Author: Migi
 */

#ifndef SRC_GUI_SCREEN_LAN_SETTINGS_H_
#define SRC_GUI_SCREEN_LAN_SETTINGS_H_

#include "gui.h"
#include "config.h"
#include "screen_menu.h"
#include "lwip/netif.h"
#include "eeprom.h"

#define plsd              ((screen_lan_settings_data_t *)screen->pdata)
#define MAC_ADDR_STR_SIZE 18

#define NETVAR_SETFLG_LAN_FLAGS     0b00000001
#define NETVAR_SETFLG_HOSTNAME      0b00000010
#define NETVAR_SETFLG_CONNECT_TOKEN 0b00000100
#define NETVAR_SETFLG_LAN_IP4_ADDR  0b00001000
#define NETVAR_SETFLG_LAN_IP4_MSK   0b00010000
#define NETVAR_SETFLG_LAN_IP4_GW    0b00100000
#define NETVAR_SETFLG_CONNECT_IP4   0b01000000

typedef struct {
    window_frame_t root;
    window_header_t header;
    window_menu_t menu;
    menu_item_t *items;

    window_text_t text;
    char mac_addr_str[MAC_ADDR_STR_SIZE];
} screen_lan_settings_data_t;

typedef struct {
    uint8_t lan_flag;
    char hostname[LAN_HOSTNAME_MAX_LEN + 1];
#ifdef BUDDY_ENABLE_CONNECT    
    char connect_token[CONNECT_TOKEN_SIZE + 1];
    ip4_addr_t connect_ip4;
#endif // BUDDY_ENABLE_CONNECT
    ip4_addr_t lan_ip4_addr;
    ip4_addr_t lan_ip4_msk;
    ip4_addr_t lan_ip4_gw;
    uint8_t set_flag;
} networkconfig_t;

extern screen_t screen_lan_settings;
extern screen_t *const pscreen_lan_settings;

#endif /* SRC_GUI_SCREEN_LAN_SETTINGS_H_ */
