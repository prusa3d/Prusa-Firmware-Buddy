/*
 * screen_lan_settings.c
 *
 *  Created on: Nov 27, 2019
 *      Author: Migi
 */

#include "screen_lan_settings.h"
#include "marlin_client.h"
#include "ini_handler.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "screens.h"
#include "screen_menu.hpp"
#include "wui_api.h"
#include "config.h"

typedef enum {
    MI_RETURN,
    MI_SWITCH,
    MI_TYPE,
    MI_SAVE,
    MI_LOAD,
    MI_COUNT
} MI_t;

static bool conn_flg = false; // wait for dhcp to supply addresses
static const char *LAN_switch_opt[] = { "On", "Off", NULL };
static const char *LAN_type_opt[] = { "DHCP", "static", NULL };

typedef struct {
    window_frame_t root;
    window_header_t header;
    window_menu_t menu;
    window_text_t text;
    menu_item_t items[MI_COUNT];
    lan_descp_str_t plan_str;

} screen_lan_settings_data_t;

static void _screen_lan_settings_item(window_menu_t *pwindow_menu, uint16_t index,
    window_menu_item_t **ppitem, void *data) {
    screen_t *screen = (screen_t *)data;
    *ppitem = &(plsd->items[index].item);
}

static void refresh_addresses(screen_t *screen) {
    ETH_config_t ethconfig;
    update_eth_addrs(&ethconfig);
    stringify_eth_for_screen(&plsd->plan_str, &ethconfig);
    plsd->text.text = (char *)plsd->plan_str;
    plsd->text.win.flg |= WINDOW_FLG_INVALID;
    gui_invalidate();
}

static void screen_lan_settings_init(screen_t *screen) {
    //============= LOAD CONFIG ===============
    ETH_config_t ethconfig;
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&ethconfig);

    conn_flg = (IS_LAN_ON(ethconfig.lan.flag) && IS_LAN_DHCP(ethconfig.lan.flag) && dhcp_addrs_are_supplied());

    //============= SCREEN INIT ===============
    rect_ui16_t menu_rect = rect_ui16(10, 32, 220, 150);

    int16_t id;
    int16_t root = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(plsd->root));
    window_disable(root);

    id = window_create_ptr(WINDOW_CLS_HEADER, root, rect_ui16(0, 0, 240, 31), &(plsd->header));
    p_window_header_set_text(&(plsd->header), "LAN SETTINGS");

    id = window_create_ptr(WINDOW_CLS_MENU, root, menu_rect, &(plsd->menu));
    plsd->menu.padding = padding_ui8(20, 6, 2, 6);
    plsd->menu.icon_rect = rect_ui16(0, 0, 16, 30);
    plsd->menu.count = MI_COUNT;
    plsd->menu.menu_items = _screen_lan_settings_item;
    plsd->menu.data = (void *)screen;

    window_set_capture(id); // set capture to list
    window_set_focus(id);

    id = window_create_ptr(WINDOW_CLS_TEXT, root, rect_ui16(10, 183, 230, 137), &(plsd->text));
    plsd->text.font = resource_font(IDR_FNT_SPECIAL);

    plsd->items[MI_RETURN] = menu_item_return;
    plsd->items[MI_SWITCH] = (menu_item_t) { { "LAN", 0, WI_SWITCH, 0 }, SCREEN_MENU_NO_SCREEN };
    plsd->items[MI_SWITCH].item.wi_switch_select.index = IS_LAN_OFF(ethconfig.lan.flag) ? 1 : 0;
    plsd->items[MI_SWITCH].item.wi_switch_select.strings = LAN_switch_opt;
    plsd->items[MI_TYPE] = (menu_item_t) { { "LAN IP", 0, WI_SWITCH, 0 }, SCREEN_MENU_NO_SCREEN };
    plsd->items[MI_TYPE].item.wi_switch_select.index = IS_LAN_STATIC(ethconfig.lan.flag) ? 1 : 0;
    plsd->items[MI_TYPE].item.wi_switch_select.strings = LAN_type_opt;
    plsd->items[MI_SAVE] = (menu_item_t) { { "Save settings", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };
    plsd->items[MI_LOAD] = (menu_item_t) { { "Load settings", 0, WI_LABEL }, SCREEN_MENU_NO_SCREEN };

    refresh_addresses(screen);
}
static uint8_t save_config(void) {
    ETH_config_t ethconfig;
    ini_file_str_t ini_str;
    ethconfig.var_mask = ETHVAR_EEPROM_CONFIG;
    load_eth_params(&ethconfig);
    stringify_eth_for_ini(&ini_str, &ethconfig);
    return ini_save_file((const char *)&ini_str);
}

static int screen_lan_settings_event(screen_t *screen, window_t *window,
    uint8_t event, void *param) {

    window_header_events(&(plsd->header));

    if (conn_flg) {
        ETH_config_t ethconfig;
        ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        load_eth_params(&ethconfig);
        if ((IS_LAN_DHCP(ethconfig.lan.flag) && dhcp_addrs_are_supplied()) || IS_LAN_STATIC(ethconfig.lan.flag)) {
            conn_flg = false;
            refresh_addresses(screen);
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
        ETH_config_t ethconfig;
        ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        load_eth_params(&ethconfig);
        if (IS_LAN_ON(ethconfig.lan.flag)) {
            turn_off_LAN(&ethconfig);
            save_eth_params(&ethconfig);
            refresh_addresses(screen);
        } else {
            turn_on_LAN(&ethconfig);
            save_eth_params(&ethconfig);
            refresh_addresses(screen);
            if (IS_LAN_DHCP(ethconfig.lan.flag)) {
                conn_flg = true;
            }
        }
        break;
    }
    case MI_TYPE: {
        ETH_config_t ethconfig;
        ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS) | ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4);
        load_eth_params(&ethconfig);
        if (IS_LAN_DHCP(ethconfig.lan.flag)) {
            if (ethconfig.lan.addr_ip4.addr == 0) {
                if (gui_msgbox("Static IPv4 addresses were not set.",
                        MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                    == MSGBOX_RES_OK) {
                    plsd->items[MI_TYPE].item.wi_switch_select.index = 0;
                }
                return 0;
            }
            ethconfig.var_mask = ETHVAR_STATIC_LAN_ADDRS;
            load_eth_params(&ethconfig);
            set_LAN_to_static(&ethconfig);
            ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
            save_eth_params(&ethconfig);
            stringify_eth_for_screen(&plsd->plan_str, &ethconfig);
            plsd->text.text = (char *)plsd->plan_str;
            plsd->text.win.flg |= WINDOW_FLG_INVALID;
            gui_invalidate();
        } else {
            set_LAN_to_dhcp(&ethconfig);
            ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
            save_eth_params(&ethconfig);
            refresh_addresses(screen);
            conn_flg = true;
        }
        break;
    }
    case MI_SAVE:
        if (!(marlin_vars()->media_inserted)) {
            if (gui_msgbox("Please insert a USB drive and try again.",
                    MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                == MSGBOX_RES_OK) {
            }
        } else {
            if (save_config()) { // !its possible to save empty configurations!
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
        if (!(marlin_vars()->media_inserted)) {
            if (gui_msgbox("Please insert USB flash disk and try again.",
                    MSGBOX_BTN_OK | MSGBOX_ICO_ERROR)
                == MSGBOX_RES_OK) {
            }
        } else {
            ETH_config_t ethconfig;
            if (load_ini_params(&ethconfig)) {
                if (gui_msgbox("Settings successfully loaded", MSGBOX_BTN_OK | MSGBOX_ICO_INFO) == MSGBOX_RES_OK) {

                    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
                    load_eth_params(&ethconfig);
                    plsd->items[MI_TYPE].item.wi_switch_select.index = IS_LAN_STATIC(ethconfig.lan.flag) ? 1 : 0;
                    window_invalidate(plsd->menu.win.id);
                    if (IS_LAN_DHCP(ethconfig.lan.flag)) {
                        refresh_addresses(screen);
                        conn_flg = true;
                    } else {
                        ethconfig.var_mask = ETHVAR_STATIC_LAN_ADDRS;
                        load_eth_params(&ethconfig);
                        stringify_eth_for_screen(&plsd->plan_str, &ethconfig);
                        plsd->text.text = (char *)plsd->plan_str;
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

extern "C" screen_t *const get_scr_lan_settings() { return &screen_lan_settings; }
