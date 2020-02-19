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

#define plsd              ((screen_lan_settings_data_t *)screen->pdata)
#define MAC_ADDR_STR_SIZE 18

typedef struct {
    window_frame_t root;
    window_header_t header;
    window_menu_t menu;
    menu_item_t *items;

    window_text_t text;
    char mac_addr_str[MAC_ADDR_STR_SIZE];
} screen_lan_settings_data_t;

typedef struct {
    ip4_addr_t lan_ip4_addr;
    ip4_addr_t lan_ip4_msk;
    ip4_addr_t lan_ip4_gw;
} networkconfig_t;

extern screen_t screen_lan_settings;
extern screen_t *const pscreen_lan_settings;

#endif /* SRC_GUI_SCREEN_LAN_SETTINGS_H_ */
