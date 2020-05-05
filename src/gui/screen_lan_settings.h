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

extern screen_t screen_lan_settings;

#endif /* SRC_GUI_SCREEN_LAN_SETTINGS_H_ */
