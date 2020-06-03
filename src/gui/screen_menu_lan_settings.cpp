/*
 * screen_lan_settings.cpp
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
#include "WindowMenuItems.hpp"
#include "wui_api.h"
#include "config.h"

/*****************************************************************************/
//Eth static class used by menu and its items
//And NO David a do not want to use singleton here
#pragma pack(push, 1)
class Eth {
    static bool new_data_flg;
    static bool conn_flg; // wait for dhcp to supply addresses

public:
    static bool IsNewData(); //clear by read
    static uint8_t GetFlag();
    static uint8_t Save();
    static void On();
    static void Off();
    static bool IsStatic();
    static bool IsOn();
    static void Init();
    static bool IsUpdated();
};
#pragma pack(pop)

bool Eth::conn_flg = false;
bool Eth::new_data_flg = false;

/*****************************************************************************/
//Eth methods
uint8_t Eth::GetFlag() {
    ETH_config_t ethconfig;
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&ethconfig);

    return ethconfig.lan.flag;
}

uint8_t Eth::Save() {
    ETH_config_t ethconfig;
    ini_file_str_t ini_str;
    ethconfig.var_mask = ETHVAR_EEPROM_CONFIG;
    load_eth_params(&ethconfig);
    stringify_eth_for_ini(&ini_str, &ethconfig);
    return ini_save_file((const char *)&ini_str);
}

void Eth::Off() {
    ETH_config_t ethconfig;
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&ethconfig);
    turn_off_LAN(&ethconfig);
    save_eth_params(&ethconfig);
    new_data_flg = true;
}

void Eth::On() {
    ETH_config_t ethconfig;
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&ethconfig);
    turn_on_LAN(&ethconfig);
    save_eth_params(&ethconfig);
    new_data_flg = true;
    if (IS_LAN_DHCP(ethconfig.lan.flag)) {
        conn_flg = true;
    }
}

bool Eth::IsStatic() {
    return IS_LAN_STATIC(GetFlag());
}
bool Eth::IsOn() {
    return !IS_LAN_OFF(GetFlag());
}

void Eth::Init() {
    uint8_t flg = GetFlag();
    conn_flg = (IS_LAN_ON(flg) && IS_LAN_DHCP(flg) && dhcp_addrs_are_supplied());
}

bool Eth::IsUpdated() {
    if (conn_flg) {
        uint8_t eth_flag = GetFlag();
        if ((IS_LAN_DHCP(eth_flag) && dhcp_addrs_are_supplied()) || IS_LAN_STATIC(eth_flag)) {
            conn_flg = false;
            return true;
        }
    }
    return false;
}

/*****************************************************************************/
//ITEMS
#pragma pack(push, 1)
class MI_LAN_ONOFF : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = "LAN";

public:
    MI_LAN_ONOFF()
        : WI_SWITCH_OFF_ON_t(Eth::IsOn() ? 0 : 1, label, 0, true, false) {}
    virtual void OnChange(size_t old_index) {
        old_index == 0 ? Eth::Off() : Eth::On();
    }
};

class MI_LAN_IP_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = "LAN IP";

    constexpr static const char *str_static = "static";
    constexpr static const char *str_DHCP = "DHCP";

public:
    MI_LAN_IP_t()
        : WI_SWITCH_t<2>(Eth::IsStatic() ? 0 : 1, label, 0, true, false, str_static, str_DHCP) {}
    virtual void OnChange(size_t old_index) {
    }
};

class MI_SAVE : public WI_LABEL_t {
    constexpr static const char *const label = "Save settings";

public:
    MI_SAVE()
        : WI_LABEL_t(label, 0, true, false) {}
    virtual void click(Iwindow_menu_t &window_menu) {
    }
};

class MI_LOAD : public WI_LABEL_t {
    constexpr static const char *const label = "Load settings";

public:
    MI_LOAD()
        : WI_LABEL_t(label, 0, true, false) {}
    virtual void click(Iwindow_menu_t &window_menu) {
    }
};
#pragma pack(pop)

/*****************************************************************************/
//parent alias
using parent = screen_menu_data_t<EHeader::On, EFooter::Off, EHelp::On,
    MI_RETURN, MI_LAN_ONOFF, MI_LAN_IP_t, MI_SAVE, MI_LOAD>;

#pragma pack(push, 1)
class ScreenMenuLanSettings : public parent {
    lan_descp_str_t plan_str;
    void refresh_addresses();

public:
    constexpr static const char *label = "LAN SETTINGS";
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
};

#pragma pack(pop)

/*****************************************************************************/
//non static member function definition
void ScreenMenuLanSettings::refresh_addresses() {
    ETH_config_t ethconfig;
    update_eth_addrs(&ethconfig);
    stringify_eth_for_screen(&plan_str, &ethconfig);
    help.text = (char *)plan_str;
    help.win.flg |= WINDOW_FLG_INVALID;
    gui_invalidate();
}

/*****************************************************************************/
//static member function definition
void ScreenMenuLanSettings::Init(screen_t *screen) {
    marlin_update_vars(MARLIN_VAR_MSK_TEMP_TARG | MARLIN_VAR_MSK(MARLIN_VAR_Z_OFFSET) | MARLIN_VAR_MSK(MARLIN_VAR_FANSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_PRNSPEED) | MARLIN_VAR_MSK(MARLIN_VAR_FLOWFACT));
    Create(screen, label);

    //============= LOAD CONFIG ===============
    Eth::Init();

    //============= SCREEN INIT ===============
    ScreenMenuLanSettings *const ths = reinterpret_cast<ScreenMenuLanSettings *>(screen->pdata);

    ths->help.font = resource_font(IDR_FNT_SPECIAL);
    ths->refresh_addresses();
}

int ScreenMenuLanSettings::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuLanSettings *const ths = reinterpret_cast<ScreenMenuLanSettings *>(screen->pdata);

    //window_header_events(&(ths->header)); //dodo check if needed
    if (Eth::IsUpdated())
        ths->refresh_addresses();

    if (event != WINDOW_EVENT_CLICK) {
        return 0;
    }

    /*  switch ((int)param) {
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
                    ths->items[MI_TYPE].item.wi_switch_select.index = 0;
                }
                return 0;
            }
            ethconfig.var_mask = ETHVAR_STATIC_LAN_ADDRS;
            load_eth_params(&ethconfig);
            set_LAN_to_static(&ethconfig);
            ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
            save_eth_params(&ethconfig);
            stringify_eth_for_screen(&ths->plan_str, &ethconfig);
            ths->text.text = (char *)ths->plan_str;
            ths->text.win.flg |= WINDOW_FLG_INVALID;
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
                    ths->items[MI_TYPE].item.wi_switch_select.index = Eth::IsStatic() ? 1 : 0;
                    window_invalidate(ths->menu.win.id);
                    if (IS_LAN_DHCP(ethconfig.lan.flag)) {
                        refresh_addresses(screen);
                        conn_flg = true;
                    } else {
                        ethconfig.var_mask = ETHVAR_STATIC_LAN_ADDRS;
                        load_eth_params(&ethconfig);
                        stringify_eth_for_screen(&ths->plan_str, &ethconfig);
                        ths->text.text = (char *)ths->plan_str;
                        ths->text.win.flg |= WINDOW_FLG_INVALID;
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
    return 0;*/

    return ths->Event(window, event, param);
}

screen_t screen_lan_settings = {
    0,
    0,
    ScreenMenuLanSettings::Init,
    ScreenMenuLanSettings::CDone,
    ScreenMenuLanSettings::CDraw,
    ScreenMenuLanSettings::CEvent,
    sizeof(ScreenMenuLanSettings), //data_size
    0,                             //pdata
};

extern "C" screen_t *const get_scr_lan_settings() { return &screen_lan_settings; }
