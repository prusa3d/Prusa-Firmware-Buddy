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
#include "RAII.hpp"
#include "../lang/i18n.h"

/*****************************************************************************/
//Eth static class used by menu and its items
//And NO David a do not want to use singleton here
class Eth {
public:
    enum class Msg : uint8_t { NoMsg,
        StaicAddrErr,
        NoUSB,
        SaveOK,
        SaveNOK,
        LoadOK,
        LoadNOK };
    static Msg msg;
    static bool new_data_flg;
    static bool conn_flg; // wait for dhcp to supply addresses
    static bool reinit_flg;
    static uint8_t save();

public:
    static uint8_t GetFlag();
    static void Save();
    static void Load();
    static void On();
    static void Off();
    static bool IsStatic();
    static bool IsDHCP();
    static bool IsOn();
    static void Init();
    static bool IsUpdated();
    static bool SetStatic();
    static bool SetDHCP();
    static Msg ConsumeMsg();
    static bool ConsumeReinit();
};

bool Eth::reinit_flg = false; //default state is "does not need reinit"
bool Eth::conn_flg = false;
bool Eth::new_data_flg = false;
Eth::Msg Eth::msg = Eth::Msg::NoMsg;
/*****************************************************************************/
//Eth methods
uint8_t Eth::GetFlag() {
    ETH_config_t ethconfig;
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&ethconfig);

    return ethconfig.lan.flag;
}

uint8_t Eth::save() {
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
    new_data_flg = true;
}

void Eth::On() {
    ETH_config_t ethconfig;
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&ethconfig);
    turn_on_LAN(&ethconfig);
    new_data_flg = true;
    if (IS_LAN_DHCP(ethconfig.lan.flag)) {
        conn_flg = true;
    }
}

bool Eth::IsStatic() {
    return IS_LAN_STATIC(GetFlag());
}

bool Eth::IsDHCP() {
    return IS_LAN_DHCP(GetFlag());
}

bool Eth::IsOn() {
    return !IS_LAN_OFF(GetFlag());
}

void Eth::Init() {
    uint8_t flg = GetFlag();
    conn_flg = (IS_LAN_ON(flg) && IS_LAN_DHCP(flg) && get_dhcp_supplied());
}

bool Eth::IsUpdated() {
    bool ret = false;
    if (conn_flg) {
        uint8_t eth_flag = GetFlag();
        if ((IS_LAN_DHCP(eth_flag) && get_dhcp_supplied()) || IS_LAN_STATIC(eth_flag)) {
            conn_flg = false;
            ret = true;
        }
    }
    ret |= new_data_flg;
    new_data_flg = false;
    return ret;
}

bool Eth::SetStatic() {
    ETH_config_t ethconfig;
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS) | ETHVAR_STATIC_LAN_ADDRS;
    load_eth_params(&ethconfig);

    if (ethconfig.lan.addr_ip4.addr == 0) {
        msg = Msg::StaicAddrErr;
        return false;
    }
    set_LAN_to_static(&ethconfig);
    new_data_flg = true;
    return true;
}

bool Eth::SetDHCP() {
    ETH_config_t ethconfig;
    ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    load_eth_params(&ethconfig);
    set_LAN_to_dhcp(&ethconfig);
    new_data_flg = true;
    conn_flg = true;
    return true;
}

Eth::Msg Eth::ConsumeMsg() {
    Msg ret = msg;
    msg = Msg::NoMsg;
    return ret;
}

void Eth::Save() {
    if (!(marlin_vars()->media_inserted)) {
        msg = Msg::NoUSB;
    } else {
        if (save()) { // !its possible to save empty configurations!
            msg = Msg::SaveOK;
        } else {
            msg = Msg::SaveNOK;
        }
    }
}

void Eth::Load() {
    if (!(marlin_vars()->media_inserted)) {
        msg = Msg::NoUSB;
    } else {
        ETH_config_t ethconfig;
        if (load_ini_params(&ethconfig)) {
            ethconfig.var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
            load_eth_params(&ethconfig);

            new_data_flg = true;
            conn_flg = true;
            reinit_flg = true;
            msg = Msg::LoadOK;
        } else {
            msg = Msg::LoadNOK;
        }
    }
}

bool Eth::ConsumeReinit() {
    bool ret = reinit_flg;
    reinit_flg = false;
    return ret;
}
/*****************************************************************************/
//ITEMS
class MI_LAN_ONOFF : public WI_SWITCH_OFF_ON_t {
    constexpr static const char *const label = "LAN";

public:
    MI_LAN_ONOFF()
        : WI_SWITCH_OFF_ON_t(Eth::IsOn() ? 1 : 0, label, 0, true, false) {}
    virtual void OnChange(size_t old_index) override {
        old_index == 0 ? Eth::On() : Eth::Off();
    }
};

class MI_LAN_IP_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = "LAN IP";

    constexpr static const char *str_static = "static";
    constexpr static const char *str_DHCP = "DHCP";

public:
    MI_LAN_IP_t()
        : WI_SWITCH_t<2>(Eth::IsStatic() ? 1 : 0, label, 0, true, false, str_DHCP, str_static) {}
    virtual void OnChange(size_t old_index) override {
        bool success = old_index == 0 ? Eth::SetStatic() : Eth::SetDHCP();
        if (!success)
            this->SetIndex(old_index);
    }
    void ReInit() {
        index = Eth::IsStatic() ? 1 : 0;
    }
};

class MI_LAN_SAVE : public WI_LABEL_t {
    constexpr static const char *const label = N_("Save settings");

public:
    MI_LAN_SAVE()
        : WI_LABEL_t(label, 0, true, false) {}
    virtual void click(IWindowMenu & /*window_menu*/) override {
        Eth::Save();
    }
};

class MI_LAN_LOAD : public WI_LABEL_t {
    constexpr static const char *const label = N_("Load settings");

public:
    MI_LAN_LOAD()
        : WI_LABEL_t(label, 0, true, false) {}
    virtual void click(IWindowMenu & /*window_menu*/) override {
        Eth::Load();
    }
};

/*****************************************************************************/
//parent alias
constexpr static const HelperConfig helper_lines = { 8, IDR_FNT_SPECIAL };
using parent = ScreenMenu<EHeader::On, EFooter::Off, helper_lines,
    MI_RETURN, MI_LAN_ONOFF, MI_LAN_IP_t, MI_LAN_SAVE, MI_LAN_LOAD>;

class ScreenMenuLanSettings : public parent {
    lan_descp_str_t plan_str; //todo not initialized in constructor
    bool msg_shown;           //todo not initialized in constructor
    void refresh_addresses();
    void show_msg(Eth::Msg msg);

public:
    constexpr static const char *label = N_("LAN SETTINGS");
    static void Init(screen_t *screen);
    static int CEvent(screen_t *screen, window_t *window, uint8_t event, void *param);
};

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

void ScreenMenuLanSettings::show_msg(Eth::Msg msg) {
    if (msg_shown)
        return;
    AutoRestore<bool> AR(msg_shown);
    msg_shown = true;
    switch (msg) {
    case Eth::Msg::StaicAddrErr:
        gui_msgbox(_("Static IPv4 addresses were not set."), MSGBOX_BTN_OK | MSGBOX_ICO_ERROR);
        break;
    case Eth::Msg::NoUSB:
        gui_msgbox(_("Please insert a USB drive and try again."), MSGBOX_BTN_OK | MSGBOX_ICO_ERROR);
        break;
    case Eth::Msg::SaveOK:
        gui_msgbox(_("The settings have been saved successfully in the \"lan_settings.ini\" file."), MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
        break;
    case Eth::Msg::SaveNOK:
        gui_msgbox(_("There was an error saving the settings in the \"lan_settings.ini\" file."), MSGBOX_BTN_OK | MSGBOX_ICO_ERROR);
        break;
    case Eth::Msg::LoadOK:
        gui_msgbox(_("Settings successfully loaded"), MSGBOX_BTN_OK | MSGBOX_ICO_INFO);
        break;
    case Eth::Msg::LoadNOK:
        gui_msgbox(_("IP addresses or parameters are not valid or the file \"lan_settings.ini\" is not in the root directory of the USB drive."), MSGBOX_BTN_OK | MSGBOX_ICO_ERROR);
        break;
    default:
        break;
    }
}

/*****************************************************************************/
//static member function definition
void ScreenMenuLanSettings::Init(screen_t *screen) {
    Create(screen, label);
    Eth::Init();

    ScreenMenuLanSettings *const ths = reinterpret_cast<ScreenMenuLanSettings *>(screen->pdata);

    ths->help.font = resource_font(IDR_FNT_SPECIAL);
    ths->refresh_addresses();
    ths->msg_shown = false;
}

int ScreenMenuLanSettings::CEvent(screen_t *screen, window_t *window, uint8_t event, void *param) {
    ScreenMenuLanSettings *const ths = reinterpret_cast<ScreenMenuLanSettings *>(screen->pdata);
    if (Eth::ConsumeReinit()) {
        MI_LAN_IP_t *item = &ths->Item<MI_LAN_IP_t>();
        item->ReInit();
    }

    //window_header_events(&(ths->header)); //dodo check if needed
    if (Eth::IsUpdated())
        ths->refresh_addresses();

    ths->show_msg(Eth::ConsumeMsg());

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
    nullptr,                       //pdata
};

screen_t *const get_scr_lan_settings() { return &screen_lan_settings; }
