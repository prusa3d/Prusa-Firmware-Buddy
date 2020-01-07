/*
 * screen_lan_settings.c
 *
 *  Created on: Nov 27, 2019
 *      Author: Migi
 */

#include "screen_lan_settings.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#include "lwip.h"
#include <stdlib.h>
#include <stdbool.h>
#include "ini.h"
#include "ff.h"
#include <string.h>

#define MAC_ADDR_START 0x1FFF781A //MM:MM:MM:SS:SS:SS
#define MAC_ADDR_SIZE 6
#define MAX_INI_SIZE 100
#define IP4_ADDR_STR_SIZE 16

typedef enum {
    MI_RETURN,
    MI_SAVE,
    MI_LOAD,
} MI_t;

static char *plan_str = NULL;
static networkconfig_t config;
static const char ini_file_name[] = "/lan_settings.ini"; //change -> change msgboxes
static char ini_file_str[MAX_INI_SIZE];
extern bool media_is_inserted();
const menu_item_t _menu_lan_items[] = {
    { { "Save settings", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
    { { "Load settings", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN },
};

static void _screen_lan_settings_item(window_menu_t *pwindow_menu, uint16_t index,
    window_menu_item_t **ppitem, void *data) {
    screen_t *screen = (screen_t *)data;
    *ppitem = &(plsd->items[index].item);
}

static uint8_t _get_ip4_addrs(void) {
    if (netif_is_up(&eth0)) {
        if (dhcp_supplied_address(&eth0)) {
            config.lan_ip4_addr.addr = netif_ip4_addr(&eth0)->addr;
            config.lan_ip4_msk.addr = netif_ip4_netmask(&eth0)->addr;
            config.lan_ip4_gw.addr = netif_ip4_gw(&eth0)->addr;
        } else {
            return 1;
        }
    } else {
        return 1;
    }
    return 0;
}

static void _addrs_to_str(char *param_str, uint8_t flg) {
    static char ip4_addr_str[IP4_ADDR_STR_SIZE], ip4_msk_str[IP4_ADDR_STR_SIZE], ip4_gw_str[IP4_ADDR_STR_SIZE];
    strncpy(ip4_addr_str, ip4addr_ntoa(&(config.lan_ip4_addr)), IP4_ADDR_STR_SIZE);
    strncpy(ip4_msk_str, ip4addr_ntoa(&(config.lan_ip4_msk)), IP4_ADDR_STR_SIZE);
    strncpy(ip4_gw_str, ip4addr_ntoa(&(config.lan_ip4_gw)), IP4_ADDR_STR_SIZE);

    if (flg)
        snprintf(param_str, MAX_INI_SIZE, "[lan_ip4]\naddress=%s\nmask=%s\ngateway=%s", ip4_addr_str, ip4_msk_str, ip4_gw_str);
    else
        snprintf(plan_str, 150, "IPv4 Address:\n    %s\nIPv4 Netmask:\n    %s\nIPv4 Gateway:\n    %s\nMAC Address:\n    %s",
            ip4_addr_str, ip4_msk_str, ip4_gw_str, param_str);
}

static void _parse_MAC_addr(char *mac_addr_str) {
    uint8_t mac_addr[] = { 0, 0, 0, 0, 0, 0 };
    for (uint8_t i = 0; i < MAC_ADDR_SIZE; i++)
        mac_addr[i] = *(volatile uint8_t *)(MAC_ADDR_START + i);

    sprintf(mac_addr_str, "%x:%x:%x:%x:%x:%x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

static void screen_lan_settings_init(screen_t *screen) {
    //============= SCREEN INIT ===============

    size_t count = sizeof(_menu_lan_items) / sizeof(menu_item_t);

    plsd->items = (menu_item_t *)malloc(sizeof(menu_item_t) * (count + 1));
    memset(plsd->items, '\0', sizeof(menu_item_t) * (count + 1));

    rect_ui16_t menu_rect = rect_ui16(10, 32, 220, 90);

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

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(10, 130, 220, 190), &(plsd->text));
    plsd->text.font = resource_font(IDR_FNT_SPECIAL);

    plsd->items[0] = menu_item_return;
    memcpy(plsd->items + 1, _menu_lan_items, count * sizeof(menu_item_t));
    //============== DECLARE VARIABLES ================

    plan_str = (char *)gui_malloc(150 * sizeof(char));

    //============= FILL VARIABLES ============

    _parse_MAC_addr(plsd->mac_addr_str);

    if (_get_ip4_addrs()) {
        config.lan_ip4_addr.addr = config.lan_ip4_msk.addr = config.lan_ip4_gw.addr = 0;
        sprintf(plan_str, "IPv4 Address:\n    0.0.0.0\nIPv4 Netmask:\n    0.0.0.0\nIPv4 Gateway:\n    0.0.0.0\nMAC Address:\n    %s",
            plsd->mac_addr_str);
    } else {
        _addrs_to_str(plsd->mac_addr_str, 0); //0 means parsing to screen text
    }

    //============= SET TEXT ================

    plsd->text.text = plan_str;
}
static uint8_t _save_ini_file(void) {
    //======= CONFIG -> INI STR ==========

    _addrs_to_str(ini_file_str, 1); //1 means parsing to ini file format
    UINT ini_config_len = strlen(ini_file_str);
    UINT written_bytes = 0;
    FIL ini_file;

    //=========== FILE ACCESS =============

    f_unlink(ini_file_name);

    uint8_t i = f_open(&ini_file, ini_file_name, FA_WRITE | FA_CREATE_NEW);
    uint8_t w = f_write(&ini_file, ini_file_str, ini_config_len, &written_bytes);
    uint8_t c = f_close(&ini_file);

    if (i || w || c || written_bytes != ini_config_len)
        return 0;

    return 1;
}

static int handler(void *user, const char *section, const char *name, const char *value) {
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("lan_ip4", "address")) {
        config.lan_ip4_addr.addr = ipaddr_addr(value);
    } else if (MATCH("lan_ip4", "mask")) {
        config.lan_ip4_msk.addr = ipaddr_addr(value);
    } else if (MATCH("lan_ip4", "gateway")) {
        config.lan_ip4_gw.addr = ipaddr_addr(value);
    } else {
        return 0; /* unknown section/name, error */
    }
    return 1;
}

static uint8_t _load_ini_file(void) {
    UINT written_bytes = 0;
    FIL ini_file;

    //=========== FILE ACCESS =============

    uint8_t file_init = f_open(&ini_file, ini_file_name, FA_READ);
    uint8_t file_read = f_read(&ini_file, ini_file_str, MAX_INI_SIZE, &written_bytes);
    uint8_t file_close = f_close(&ini_file);

    if (file_init || file_read || file_close) {
        return 0;
    }

    //=========== INI FILE PARSING =============

    if (ini_parse_string(ini_file_str, handler, 0) < 0) {
        return 0;
    }

    //=========== SET ADDRESSES ================

    netifapi_netif_set_addr(&eth0,
        (const ip4_addr_t *)&config.lan_ip4_addr,
        (const ip4_addr_t *)&config.lan_ip4_msk,
        (const ip4_addr_t *)&config.lan_ip4_gw);

    return 1;
}

static int screen_lan_settings_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {

    window_header_events(&(plsd->header));

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    switch ((int)param) {
    case MI_RETURN:
        screen_close();
        return 1;
    case MI_SAVE:
        if (media_is_inserted() == false) {
            if (gui_msgbox("Please insert USB flash disk and try again.",
                    MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                == MSGBOX_RES_OK) {
            }
        } else {
            if (_save_ini_file()) { // !its possible to save empty configurations!
                if (gui_msgbox("Settings saved in the \"lan_settings.ini\" file.",
                        MSGBOX_BTN_OK | MSGBOX_ICO_INFO)
                    == MSGBOX_RES_OK) {
                }
            } else {
                if (gui_msgbox("File \"lan_settings.ini\" did not saved properly.",
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
            if (_load_ini_file()) {
                if (gui_msgbox("Settings successfully loaded", MSGBOX_BTN_OK | MSGBOX_ICO_INFO) == MSGBOX_RES_OK) {
                }
                _addrs_to_str(plsd->mac_addr_str, 0);
                plsd->text.win.flg |= WINDOW_FLG_INVALID;

            } else {
                if (gui_msgbox("File \"lan_settings.ini\" not found in the root directory of the USB flash disk.",
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
    0, //pdata
};

screen_t *const pscreen_lan_settings = &screen_lan_settings;
