/*
 * screen_lan_settings.c
 *
 *  Created on: Nov 27, 2019
 *      Author: Migi
 */

#include "screen_lan_settings.h"
#include "ini_handler.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#include "lwip.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAC_ADDR_START    0x1FFF781A //MM:MM:MM:SS:SS:SS
#define MAC_ADDR_SIZE     6
#define IP4_ADDR_STR_SIZE 16

#define _change_static_to_static() _change_dhcp_to_static()

typedef enum {
    MI_RETURN,
    MI_SWITCH,
    MI_TYPE,
    MI_SAVE,
    MI_LOAD,
} MI_t;

static char *plan_str = NULL;
static bool conn_flg = false; // wait for dhcp to supply addresses
static networkconfig_t config;
static const char *LAN_switch_opt[] = { "On", "Off", NULL };
static const char *LAN_type_opt[] = { "DHCP", "static", NULL };
extern bool media_is_inserted();
const menu_item_t _menu_lan_items[] = {
    { { "LAN", 0, WI_SWITCH, .wi_switch_select = { 0, LAN_switch_opt } }, SCREEN_MENU_NO_SCREEN },
    { { "LAN IP", 0, WI_SWITCH, .wi_switch_select = { 0, LAN_type_opt } }, SCREEN_MENU_NO_SCREEN },
    { { "Save settings", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Load settings", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
};

static void _screen_lan_settings_item(window_menu_t *pwindow_menu, uint16_t index,
    window_menu_item_t **ppitem, void *data) {
    screen_t *screen = (screen_t *)data;
    *ppitem = &(plsd->items[index].item);
}

static void _get_ip4_addrs(void) {
    if (!(config.lan_flag & LAN_EEFLG_ONOFF)) {
        if ((!(config.lan_flag & LAN_EEFLG_TYPE) && dhcp_supplied_address(&eth0)) || config.lan_flag & LAN_EEFLG_TYPE) {
            config.lan_ip4_addr.addr = netif_ip4_addr(&eth0)->addr;
            config.lan_ip4_msk.addr = netif_ip4_netmask(&eth0)->addr;
            config.lan_ip4_gw.addr = netif_ip4_gw(&eth0)->addr;
            return;
        }
    }
    if (!(config.lan_flag & LAN_EEFLG_TYPE)) {
        config.lan_ip4_addr.addr = 0;
        config.lan_ip4_msk.addr = 0;
        config.lan_ip4_gw.addr = 0;
        return;
    }
    config.lan_ip4_addr.addr = eeprom_get_var(EEVAR_LAN_IP4_ADDR).ui32;
    config.lan_ip4_msk.addr = eeprom_get_var(EEVAR_LAN_IP4_MSK).ui32;
    config.lan_ip4_gw.addr = eeprom_get_var(EEVAR_LAN_IP4_GW).ui32;
}

/** Puts wanted net info in the string.
*
*   It as two options:
*       1) flg == 0: function creates string for LAN_SETTINGS display (param_str = mac address)
*       2) flg == 1: function creates string for ini file (param_str = destination pointer)
*/
static void stringify_netinfo(char *param_str, uint8_t flg) {
    static char ip4_addr_str[IP4_ADDR_STR_SIZE];
    static char ip4_msk_str[IP4_ADDR_STR_SIZE];
    static char ip4_gw_str[IP4_ADDR_STR_SIZE];
    strlcpy(ip4_addr_str, ip4addr_ntoa(&(config.lan_ip4_addr)), IP4_ADDR_STR_SIZE);
    strlcpy(ip4_msk_str, ip4addr_ntoa(&(config.lan_ip4_msk)), IP4_ADDR_STR_SIZE);
    strlcpy(ip4_gw_str, ip4addr_ntoa(&(config.lan_ip4_gw)), IP4_ADDR_STR_SIZE);

    if (flg) {
        char save_hostname[LAN_HOSTNAME_MAX_LEN + 1];
        variant8_t hostname = eeprom_get_var(EEVAR_LAN_HOSTNAME);
        strlcpy(save_hostname, hostname.pch, LAN_HOSTNAME_MAX_LEN + 1);
        variant8_done(&hostname);
#ifdef BUDDY_ENABLE_CONNECT
        char ip4_connect_str[IP4_ADDR_STR_SIZE];
        char save_connect_token[CONNECT_TOKEN_SIZE + 1];
        variant8_t connect_token = eeprom_get_var(EEVAR_CONNECT_TOKEN);
        strlcpy(save_connect_token, connect_token.pch, CONNECT_TOKEN_SIZE + 1);
        variant8_done(&connect_token);
        config.connect_ip4.addr = eeprom_get_var(EEVAR_CONNECT_IP4).ui32;
        strlcpy(ip4_connect_str, ip4addr_ntoa(&(config.connect_ip4)), IP4_ADDR_STR_SIZE);
        snprintf(param_str, MAX_INI_SIZE, "[lan_ip4]\ntype=%s\nhostname=%s\naddress=%s\nmask=%s\ngateway=%s\n\n[connect]\naddress=%s\ntoken=%s\n",
            config.lan_flag & LAN_EEFLG_TYPE ? LAN_type_opt[1] : LAN_type_opt[0], save_hostname, ip4_addr_str, ip4_msk_str, ip4_gw_str, ip4_connect_str, save_connect_token);
#else
        snprintf(param_str, MAX_INI_SIZE, "[lan_ip4]\ntype=%s\nhostname=%s\naddress=%s\nmask=%s\ngateway=%s\n",
            config.lan_flag & LAN_EEFLG_TYPE ? LAN_type_opt[1] : LAN_type_opt[0], save_hostname, ip4_addr_str, ip4_msk_str, ip4_gw_str);
#endif // BUDDY_ENABLE_CONNECT
    } else {
        snprintf(plan_str, 150, "IPv4 Address:\n  %s      \nIPv4 Netmask:\n  %s      \nIPv4 Gateway:\n  %s      \nMAC Address:\n  %s",
            ip4_addr_str, ip4_msk_str, ip4_gw_str, param_str);
    }
}

static void _refresh_addresses(screen_t *screen) {
    _get_ip4_addrs();
    stringify_netinfo(plsd->mac_addr_str, 0);
    plsd->text.text = plan_str;
    plsd->text.win.flg |= WINDOW_FLG_INVALID;
    gui_invalidate();
}

static void _parse_MAC_addr(char *mac_addr_str) {
    volatile uint8_t mac_addr[] = { 0, 0, 0, 0, 0, 0 };
    for (uint8_t i = 0; i < MAC_ADDR_SIZE; i++)
        mac_addr[i] = *(volatile uint8_t *)(MAC_ADDR_START + i);

    sprintf(mac_addr_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

static void screen_lan_settings_init(screen_t *screen) {
    //============= SCREEN INIT ===============

    size_t count = sizeof(_menu_lan_items) / sizeof(menu_item_t);

    plsd->items = (menu_item_t *)malloc(sizeof(menu_item_t) * (count + 1));
    memset(plsd->items, '\0', sizeof(menu_item_t) * (count + 1));

    rect_ui16_t menu_rect = rect_ui16(10, 32, 220, 150);

    int16_t id;
    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(plsd->root));
    window_disable(root);

    id = window_create_ptr(WINDOW_CLS_HEADER, root, rect_ui16(0, 0, 240, 31), &(plsd->header));
    p_window_header_set_text(&(plsd->header), "LAN SETTINGS");

    id = window_create_ptr(WINDOW_CLS_MENU, root, menu_rect, &(plsd->menu));
    plsd->menu.padding = padding_ui8(20, 6, 2, 6);
    plsd->menu.icon_rect = rect_ui16(0, 0, 16, 30);
    plsd->menu.count = count + 1;
    plsd->menu.menu_items = _screen_lan_settings_item;
    plsd->menu.data = (void *)screen;

    window_set_capture(id); // set capture to list
    window_set_focus(id);

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(10, 183, 230, 137), &(plsd->text));
    plsd->text.font = resource_font(IDR_FNT_SPECIAL);

    plsd->items[0] = menu_item_return;
    memcpy(plsd->items + 1, _menu_lan_items, count * sizeof(menu_item_t));

    config.lan_flag = eeprom_get_var(EEVAR_LAN_FLAG).ui8;

    plsd->items[MI_SWITCH].item.wi_switch_select.index = config.lan_flag & LAN_EEFLG_ONOFF;
    plsd->items[MI_TYPE].item.wi_switch_select.index = config.lan_flag & LAN_EEFLG_TYPE ? 1 : 0;
    if (!(config.lan_flag & LAN_EEFLG_ONOFF) && !(config.lan_flag & LAN_EEFLG_TYPE) && !dhcp_supplied_address(&eth0)) {
        conn_flg = true;
    }

    //============== DECLARE VARIABLES ================

    plan_str = (char *)gui_malloc(150 * sizeof(char));

    //============= FILL VARIABLES ============

    _parse_MAC_addr(plsd->mac_addr_str);
    _refresh_addresses(screen);
}
static uint8_t _save_config(void) {
    stringify_netinfo(ini_file_str, 1); //1 means parsing to ini file format
    return ini_save_file(ini_file_str);
}

static void _change_any_to_static(void) {
    if (netif_is_up(&eth0)) {
        netifapi_netif_set_down(&eth0);
    }
    config.lan_flag |= LAN_EEFLG_TYPE;
    eeprom_set_var(EEVAR_LAN_FLAG, variant8_ui8(config.lan_flag));
#ifdef DNS_MODULE_ON
    ip4_addr_t ip4_dns1, ip4_dns2;
#endif //DNS_MODULE_ON
    config.lan_ip4_addr.addr = eeprom_get_var(EEVAR_LAN_IP4_ADDR).ui32;
    config.lan_ip4_msk.addr = eeprom_get_var(EEVAR_LAN_IP4_MSK).ui32;
    config.lan_ip4_gw.addr = eeprom_get_var(EEVAR_LAN_IP4_GW).ui32;
#ifdef DNS_MODULE_ON
    ip4_dns1.addr = eeprom_get_var(EEVAR_LAN_IP4_DNS1).ui32;
    ip4_dns2.addr = eeprom_get_var(EEVAR_LAN_IP4_DNS2).ui32;
#endif //DNS_MODULE_ON
    netifapi_netif_set_addr(&eth0,
        (const ip4_addr_t *)&config.lan_ip4_addr,
        (const ip4_addr_t *)&config.lan_ip4_msk,
        (const ip4_addr_t *)&config.lan_ip4_gw);
#ifdef DNS_MODULE_ON
    dns_setserver(0, (const ip4_addr_t *)&ip4_dns1);
    dns_setserver(1, (const ip4_addr_t *)&ip4_dns2);
#endif //DNS_MODULE_ON
    if (netif_is_link_up(&eth0) && !(config.lan_flag & LAN_EEFLG_ONOFF)) {
        netifapi_netif_set_up(&eth0);
    }
}

static void _change_static_to_dhcp(void) {
    if (netif_is_up(&eth0)) {
        netifapi_netif_set_down(&eth0);
    }
    config.lan_flag &= ~LAN_EEFLG_TYPE;
    eeprom_set_var(EEVAR_LAN_FLAG, variant8_ui8(config.lan_flag));
    if (netif_is_link_up(&eth0) && !(config.lan_flag & LAN_EEFLG_ONOFF)) {
        netifapi_netif_set_up(&eth0);
    }
}

static int ini_load_handler(void *user, const char *section, const char *name, const char *value) {
    networkconfig_t *tmp_config = (networkconfig_t *)user;
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("lan_ip4", "type")) {
        if (strncmp(value, "DHCP", 4) == 0 || strncmp(value, "dhcp", 4) == 0) {
            tmp_config->lan_flag &= ~LAN_EEFLG_TYPE;
            tmp_config->set_flag |= NETVAR_SETFLG_LAN_FLAGS;
        } else if (strncmp(value, "STATIC", 6) == 0 || strncmp(value, "static", 6) == 0) {
            tmp_config->lan_flag |= LAN_EEFLG_TYPE;
            tmp_config->set_flag |= NETVAR_SETFLG_LAN_FLAGS;
        }
    } else if (MATCH("lan_ip4", "hostname")) {
        strlcpy(tmp_config->hostname, value, LAN_HOSTNAME_MAX_LEN + 1);
        tmp_config->hostname[LAN_HOSTNAME_MAX_LEN] = '\0';
        tmp_config->set_flag |= NETVAR_SETFLG_HOSTNAME;
    } else if (MATCH("lan_ip4", "address")) {
        if (ip4addr_aton(value, &tmp_config->lan_ip4_addr)) {
            tmp_config->set_flag |= NETVAR_SETFLG_LAN_IP4_ADDR;
        }
    } else if (MATCH("lan_ip4", "mask")) {
        if (ip4addr_aton(value, &tmp_config->lan_ip4_msk)) {
            tmp_config->set_flag |= NETVAR_SETFLG_LAN_IP4_MSK;
        }
    } else if (MATCH("lan_ip4", "gateway")) {
        if (ip4addr_aton(value, &tmp_config->lan_ip4_gw)) {
            tmp_config->set_flag |= NETVAR_SETFLG_LAN_IP4_GW;
        }
    } 
#ifdef BUDDY_ENABLE_CONNECT    
    else if (MATCH("connect", "address")) {
        if (ip4addr_aton(value, &tmp_config->connect_ip4)) {
            tmp_config->set_flag |= NETVAR_SETFLG_CONNECT_IP4;
        }
    } else if (MATCH("connect", "token")) {
        strlcpy(tmp_config->connect_token, value, CONNECT_TOKEN_SIZE + 1);
        tmp_config->connect_token[CONNECT_TOKEN_SIZE] = '\0';
        tmp_config->set_flag |= NETVAR_SETFLG_CONNECT_TOKEN;
    } 
#endif // BUDDY_ENABLE_CONNECT    
    else {
        return 0; /* unknown section/name, error */
    }
    return 1;
}

static uint8_t _load_config(void) {

    networkconfig_t tmp_config;
    tmp_config.lan_flag = config.lan_flag;
    tmp_config.set_flag = 0;

    if(ini_load_file(ini_load_handler, &tmp_config) == 0){
        return 0;
    }

    if (!(tmp_config.lan_flag & LAN_EEFLG_TYPE)) {
        if(tmp_config.set_flag & NETVAR_SETFLG_HOSTNAME){
            strlcpy(interface_hostname, tmp_config.hostname, LAN_HOSTNAME_MAX_LEN + 1);
            eth0.hostname = interface_hostname;
            variant8_t hostname = variant8_pchar(interface_hostname, 0, 0);
            eeprom_set_var(EEVAR_LAN_HOSTNAME, hostname);
            variant8_done(&hostname);
        }
        if (tmp_config.lan_flag & LAN_EEFLG_TYPE) {
            _change_static_to_dhcp();
        }
#ifdef BUDDY_ENABLE_CONNECT
        if(tmp_config.set_flag & NETVAR_SETFLG_CONNECT_TOKEN){
            variant8_t token = variant8_pchar(tmp_config.connect_token, 0, 0);
            eeprom_set_var(EEVAR_CONNECT_TOKEN, token);
            variant8_done(&token);
        }
        if (tmp_config.set_flag & NETVAR_SETFLG_CONNECT_IP4) {
            eeprom_set_var(EEVAR_CONNECT_IP4, variant8_ui32(tmp_config.connect_ip4.addr));
        }
#endif // BUDDY_ENABLE_CONNECT
    } else {
        if ((tmp_config.set_flag & (NETVAR_SETFLG_LAN_IP4_ADDR | NETVAR_SETFLG_LAN_IP4_MSK | NETVAR_SETFLG_LAN_IP4_GW))
             != (NETVAR_SETFLG_LAN_IP4_ADDR | NETVAR_SETFLG_LAN_IP4_MSK | NETVAR_SETFLG_LAN_IP4_GW)) {
            return 0;
        } else {
            if(tmp_config.set_flag & NETVAR_SETFLG_HOSTNAME){
                strlcpy(interface_hostname, tmp_config.hostname, LAN_HOSTNAME_MAX_LEN + 1);
                eth0.hostname = interface_hostname;
                variant8_t hostname = variant8_pchar(interface_hostname, 0, 0);
                eeprom_set_var(EEVAR_LAN_HOSTNAME, hostname);
                variant8_done(&hostname);
            }
#ifdef BUDDY_ENABLE_CONNECT
            if(tmp_config.set_flag & NETVAR_SETFLG_CONNECT_TOKEN){
                variant8_t token = variant8_pchar(tmp_config.connect_token, 0, 0);
                eeprom_set_var(EEVAR_CONNECT_TOKEN, token);
                variant8_done(&token);
            }
            if(tmp_config.set_flag & NETVAR_SETFLG_CONNECT_IP4){
                eeprom_set_var(EEVAR_CONNECT_IP4, variant8_ui32(tmp_config.connect_ip4.addr));
            }
#endif // BUDDY_ENABLE_CONNECT
            eeprom_set_var(EEVAR_LAN_IP4_ADDR, variant8_ui32(tmp_config.lan_ip4_addr.addr));
            eeprom_set_var(EEVAR_LAN_IP4_MSK, variant8_ui32(tmp_config.lan_ip4_msk.addr));
            eeprom_set_var(EEVAR_LAN_IP4_GW, variant8_ui32(tmp_config.lan_ip4_gw.addr));
            
            _change_any_to_static();
        }
    }
    return 1;
}
static int screen_lan_settings_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {

    window_header_events(&(plsd->header));

    if (conn_flg) {
        if (config.lan_flag & LAN_EEFLG_TYPE || dhcp_supplied_address(&eth0)) {
            conn_flg = false;
            _refresh_addresses(screen);
        }
    }

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    switch ((int)param) {
    case MI_RETURN:
        screen_close();
        return 1;
    case MI_SWITCH: {
        if (!(config.lan_flag & LAN_EEFLG_ONOFF)) {
            if (netif_is_up(&eth0)) {
                netifapi_netif_set_down(&eth0);
            }
            config.lan_flag |= LAN_EEFLG_ONOFF;
            eeprom_set_var(EEVAR_LAN_FLAG, variant8_ui8(config.lan_flag));
            _refresh_addresses(screen);
        } else {
            config.lan_flag &= ~LAN_EEFLG_ONOFF;
            eeprom_set_var(EEVAR_LAN_FLAG, variant8_ui8(config.lan_flag));
            if (netif_is_link_up(&eth0)) {
                netifapi_netif_set_up(&eth0);
            }
            _refresh_addresses(screen);
            conn_flg = true;
        }
        break;
    }
    case MI_TYPE: {
        if (!(config.lan_flag & LAN_EEFLG_TYPE)) {
            if (eeprom_get_var(EEVAR_LAN_IP4_ADDR).ui32 == 0) {
                if (gui_msgbox("Static IPv4 addresses were not set.",
                        MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                    == MSGBOX_RES_OK) {
                    plsd->items[MI_TYPE].item.wi_switch_select.index = 0;
                }
                return 0;
            }
            _change_any_to_static();
            stringify_netinfo(plsd->mac_addr_str, 0);
            plsd->text.text = plan_str;
            plsd->text.win.flg |= WINDOW_FLG_INVALID;
            gui_invalidate();
        } else {
            _change_static_to_dhcp();
            _refresh_addresses(screen);
            if (!(config.lan_flag & LAN_EEFLG_ONOFF)) {
                conn_flg = true;
            }
        }
        break;
    }
    case MI_SAVE:
        if (media_is_inserted() == false) {
            if (gui_msgbox("Please insert a USB drive and try again.",
                    MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                == MSGBOX_RES_OK) {
            }
        } else {
            if (_save_config()) { // !its possible to save empty configurations!
                if (gui_msgbox("The settings have been saved successfully in the \"lan_settings.ini\" file.",
                        MSGBOX_BTN_OK | MSGBOX_ICO_INFO)
                    == MSGBOX_RES_OK) {
                }
            } else {
                if (gui_msgbox("There was an error saving the settings in the \"lan_settings.ini\" file.",
                        MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                    == MSGBOX_RES_OK) {
                }
            }
        }
        break;
    case MI_LOAD:
        if (media_is_inserted() == false) {
            if (gui_msgbox("Please insert USB flash disk and try again.",
                    MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                == MSGBOX_RES_OK) {
            }
        } else {
            if (_load_config()) {
                if (gui_msgbox("Settings successfully loaded", MSGBOX_BTN_OK | MSGBOX_ICO_INFO) == MSGBOX_RES_OK) {
                    plsd->items[MI_TYPE].item.wi_switch_select.index = config.lan_flag & LAN_EEFLG_TYPE ? 1 : 0;
                    window_invalidate(plsd->menu.win.id);
                    if (!(config.lan_flag & LAN_EEFLG_TYPE)) {
                        _refresh_addresses(screen);
                        conn_flg = true;
                    } else {
                        stringify_netinfo(plsd->mac_addr_str, 0);
                        plsd->text.text = plan_str;
                        plsd->text.win.flg |= WINDOW_FLG_INVALID;
                    }
                }

            } else {
                if (gui_msgbox("IP addresses are not valid or the file \"lan_settings.ini\" is not in the root directory of the USB drive.",
                        MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                    == MSGBOX_RES_OK) {
                }
            }
        }
        break;
    }
    return 0;
}

static void screen_lan_settings_draw(screen_t *screen) {
}

static void screen_lan_settings_done(screen_t *screen) {

    if (plan_str)
        gui_free(plan_str);
    window_destroy(plsd->root.win.id);
}

screen_t screen_lan_settings = {
    0,
    0,
    screen_lan_settings_init,
    screen_lan_settings_done,
    screen_lan_settings_draw,
    screen_lan_settings_event,
    sizeof(screen_lan_settings_data_t), //data_size
    0,                                  //pdata
};

screen_t *const pscreen_lan_settings = &screen_lan_settings;
