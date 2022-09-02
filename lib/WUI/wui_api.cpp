/*
 * wui_api.c
 * \brief   interface functions for Web User Interface library
 *
 *  Created on: April 22, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 *  Modify on 09/17/2021
 *      Author: Marek Mosna <marek.mosna[at]prusa3d.cz>
 */

#include "wui_api.h"
#include "netdev.h"
#include "version.h"
#include "otp.h"
#include "ini_handler.h"
#include "eeprom.h"
#include "stm32f4xx_hal.h"
#include "print_utils.hpp"
#include "marlin_client.hpp"
#include "fsm_types.hpp"

#include <basename.h>
#include <lfn.h>
#include <ScreenHandler.hpp>
#include <screen_home.hpp>
#include <screen_print_preview.hpp>

#include <cassert>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <atomic>
#include "configuration_store.hpp"

#define USB_MOUNT_POINT        "/usb/"
#define USB_MOUNT_POINT_LENGTH 5

extern RTC_HandleTypeDef hrtc;

bool sntp_time_init = false;
static char wui_media_LFN[FILE_NAME_BUFFER_LEN]; // static buffer for gcode file name
static char wui_media_SFN_path[FILE_PATH_BUFFER_LEN];
static std::atomic<uint32_t> uploaded_gcodes;
static std::atomic<uint32_t> modified_gcodes;

// example of simple callback automatically sending print response (click on print button) in preview fsm
// it would be better to use queue to fet rid of no longer current commands
// see "void DialogHandler::Command(uint32_t u32, uint16_t u16) "
#if 0
static void fsm_cb(uint32_t u32, uint16_t u16) {
    fsm::variant_t variant(u32, u16);
    if (variant.GetType() == ClientFSM::PrintPreview) {
        PhasesPrintPreview phase;
        switch (variant.GetCommand()) {
        case ClientFSM_Command::change:
            phase = GetEnumFromPhaseIndex<PhasesPrintPreview>(variant.change.data.GetPhase());
            break;
        case ClientFSM_Command::create:
            phase = PhasesPrintPreview::_first;
            break;
        default:
            return;
        }

        if (phase == PhasesPrintPreview::main_dialog)
            marlin_FSM_response(phase, Response::Print);
    }
}

#else // !0

static void fsm_cb(uint32_t u32, uint16_t u16) {
}

#endif // 0

void wui_marlin_client_init(void) {
    marlin_vars_t *vars = marlin_client_init(); // init the client
    // force update variables when starts
    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF, NULL);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI, NULL);
    marlin_client_set_fsm_cb(fsm_cb);
    if (vars) {
        /*
         * Note: We currently have only a single marlin client for
         * WUI/networking. So we can use a single buffer there.
         */
        vars->media_LFN = wui_media_LFN;
        vars->media_SFN_path = wui_media_SFN_path;
    }
}

struct ini_load_def {
    // eth::ipv4 or wifi::ipv4
    const char *ip_section;
    // The config to store to (must not be NULL).
    ETH_config_t *config;
    // The wifi AP definition. May be NULL (in which case it isn't loaded).
    ap_entry_t *ap;
};

static bool ini_string_match(const char *section, const char *section_var, const char *name, const char *name_var) {
    return strcmp(section_var, section) == 0 && strcmp(name_var, name) == 0;
}

static int ini_handler_func(void *user, const char *section, const char *name, const char *value) {
    ini_load_def *def = (ini_load_def *)user;

    ETH_config_t *tmp_config = def->config;

    if (ini_string_match(section, def->ip_section, name, "type")) {
        if (strncasecmp(value, "DHCP", 4) == 0) {
            CHANGE_FLAG_TO_DHCP(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        } else if (strncasecmp(value, "STATIC", 6) == 0) {
            CHANGE_FLAG_TO_STATIC(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        }
    } else if (ini_string_match(section, "network", name, "hostname")) {
        strlcpy(tmp_config->hostname, value, ETH_HOSTNAME_LEN + 1);
        tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_HOSTNAME);
    } else if (ini_string_match(section, def->ip_section, name, "addr")) {
        if (ip4addr_aton(value, &tmp_config->lan.addr_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4);
        }
    } else if (ini_string_match(section, def->ip_section, name, "mask")) {
        if (ip4addr_aton(value, &tmp_config->lan.msk_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_MSK_IP4);
        }
    } else if (ini_string_match(section, def->ip_section, name, "gw")) {
        if (ip4addr_aton(value, &tmp_config->lan.gw_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_GW_IP4);
        }
    } else if (ini_string_match(section, "network", name, "dns4")) {

        if (NULL != strchr(value, ';')) {
            char *token;
            char *rest = (char *)value;
            for (int i = 0; i < 2; i++) {
                token = strtok_r(rest, ";", &rest);
                if (NULL != token) {
                    switch (i) {
                    case 0:
                        if (ip4addr_aton(token, &tmp_config->dns1_ip4)) {
                            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_DNS1_IP4);
                        }
                        break;
                    case 1:
                        if (ip4addr_aton(token, &tmp_config->dns2_ip4)) {
                            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_DNS2_IP4);
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        } else {
            if (ip4addr_aton(value, &tmp_config->dns1_ip4)) {
                tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_DNS1_IP4);
            }
        }
    }

    if (def->ap) {
        if (ini_string_match(section, "wifi", name, "key_mgmt")) {
            if (strcasecmp("WPA", value) == 0) {
                def->ap->security = AP_SEC_WPA;
            } else if (strcasecmp("WEP", value) == 0) {
                def->ap->security = AP_SEC_WEP;
            } else if (strcasecmp("NONE", value) == 0) {
                def->ap->security = AP_SEC_NONE;
            }
            // TODO: else -> ??? Any way to tell the user?
            tmp_config->var_mask |= ETHVAR_MSK(APVAR_SECURITY);
        } else if (ini_string_match(section, "wifi", name, "ssid")) {
            strlcpy(def->ap->ssid, value, SSID_MAX_LEN + 1);
            tmp_config->var_mask |= ETHVAR_MSK(APVAR_SSID);
        } else if (ini_string_match(section, "wifi", name, "psk")) {
            strlcpy(def->ap->pass, value, WIFI_PSK_MAX + 1);
            tmp_config->var_mask |= ETHVAR_MSK(APVAR_PASS);
        }
    }

    return 1;
}

uint32_t load_ini_file_eth(ETH_config_t *config) {
    ini_load_def def = {};
    def.config = config;
    def.ip_section = "eth::ipv4";
    return ini_load_file(ini_handler_func, &def);
}

uint32_t load_ini_file_wifi(ETH_config_t *config, ap_entry_t *ap) {
    ini_load_def def = {};
    def.config = config;
    def.ip_section = "wifi::ipv4";
    def.ap = ap;
    return ini_load_file(ini_handler_func, &def);
}
// Pick the right variable depending on the net device we use.
template <class T>
static T &vid(uint32_t net_id, T &wifi, T &lan) {
    switch (net_id) {
    case NETDEV_ETH_ID:
        return lan;
    case NETDEV_ESP_ID:
        return wifi;
    default:
        assert(0 /* Unknown net device */);
    }
    // should not happen
    return lan;
}

void save_net_params(ETH_config_t *ethconfig, ap_entry_t *ap, uint32_t netdev_id) {
    if (ethconfig->var_mask & (ETHVAR_MSK(ETHVAR_LAN_FLAGS) | ETHVAR_MSK(APVAR_SECURITY))) {
        uint8_t flags = ethconfig->lan.flag;
        if (ap != NULL) {
            flags |= ap->security;
        }
        vid(netdev_id, config_store().lan_flag, config_store().wifi_flag).set(flags);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4)) {
        vid(netdev_id, config_store().lan_ip4_addr, config_store().wifi_ip4_addr).set(ethconfig->lan.addr_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_DNS1_IP4)) {
        vid(netdev_id, config_store().lan_ip4_dns1, config_store().wifi_ip4_dns1).set(ethconfig->dns1_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_DNS2_IP4)) {
        vid(netdev_id, config_store().lan_ip4_dns2, config_store().wifi_ip4_dns2).set(ethconfig->dns2_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_MSK_IP4)) {
        vid(netdev_id, config_store().lan_ip4_msk, config_store().wifi_ip4_msk).set(ethconfig->lan.msk_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_GW_IP4)) {
        vid(netdev_id, config_store().lan_ip4_gw, config_store().wifi_ip4_gw).set(ethconfig->lan.gw_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_HOSTNAME)) {
        vid(netdev_id, config_store().lan_hostname, config_store().wifi_hostname).set(ethconfig->hostname);
    }

    if (ap != NULL) {
        assert(netdev_id == NETDEV_ESP_ID);
        // +1 to account for null byte
        static_assert(decltype(config_store().wifi_ap_ssid)::size == WIFI_MAX_SSID_LEN + 1);
        static_assert(decltype(config_store().wifi_ap_passwd)::size == WIFI_MAX_PASSWD_LEN + 1);

        if (ethconfig->var_mask & ETHVAR_MSK(APVAR_SSID)) {
            config_store().wifi_ap_ssid.set(ap->ssid);
        }
        if (ethconfig->var_mask & ETHVAR_MSK(APVAR_PASS)) {
            config_store().wifi_ap_passwd.set(ap->pass);
        }
    }
}

void load_net_params(ETH_config_t *ethconfig, ap_entry_t *ap, uint32_t netdev_id) {
    // Just the flags, without (possibly) the wifi secutiry
    ethconfig->lan.flag = vid(netdev_id, config_store().lan_flag, config_store().wifi_flag).get() & ~APSEC_MASK;
    ethconfig->lan.addr_ip4.addr = vid(netdev_id, config_store().lan_ip4_addr, config_store().wifi_ip4_addr).get();
    ethconfig->dns1_ip4.addr = vid(netdev_id, config_store().lan_ip4_dns1, config_store().wifi_ip4_dns1).get();
    ethconfig->dns2_ip4.addr = vid(netdev_id, config_store().lan_ip4_dns2, config_store().wifi_ip4_dns2).get();
    ethconfig->lan.msk_ip4.addr = vid(netdev_id, config_store().lan_ip4_msk, config_store().wifi_ip4_msk).get();
    ethconfig->lan.gw_ip4.addr = vid(netdev_id, config_store().lan_ip4_gw, config_store().wifi_ip4_gw).get();

    auto hostname = vid(netdev_id, config_store().lan_hostname, config_store().wifi_hostname).get();
    strcpy(ethconfig->hostname, hostname.data());

    if (ap != NULL) {
        assert(netdev_id == NETDEV_ESP_ID);

        ap->security = static_cast<ap_sec_t>(config_store().wifi_flag.get() & APSEC_MASK);
        strncpy(ap->ssid, config_store().wifi_ap_ssid.get().data(), config_store().wifi_ap_ssid.size);
        strncpy(ap->pass, config_store().wifi_ap_passwd.get().data(), config_store().wifi_ap_passwd.size);
    }
}

void get_MAC_address(mac_address_t *dest, uint32_t netdev_id) {
    uint8_t mac[6 /*sizeof(otp_get_mac_address()->mac)*/]; // TODO
    if (netdev_get_MAC_address(netdev_id, mac)) {
        snprintf(*dest, MAC_ADDR_STR_LEN, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        **dest = '\0';
    }
}

void sntp_set_system_time(uint32_t sec) {

    // RTC_TimeTypeDef has attributes like TimeFormat (AM/PM) and DayLightSaving, which we don't use
    RTC_TimeTypeDef currTime = { 0 };
    RTC_DateTypeDef currDate = { 0 };

    struct tm current_time_val;
    time_t current_time = (time_t)sec;

    localtime_r(&current_time, &current_time_val);

    currTime.Seconds = current_time_val.tm_sec;
    currTime.Minutes = current_time_val.tm_min;
    currTime.Hours = current_time_val.tm_hour;
    currDate.Date = current_time_val.tm_mday;
    currDate.Month = current_time_val.tm_mon;
    currDate.Year = current_time_val.tm_year;
    currDate.WeekDay = current_time_val.tm_wday;

    HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &currDate, RTC_FORMAT_BIN);

    sntp_time_init = true;
}

void add_time_to_timestamp(int32_t secs_to_add, struct tm *timestamp) {

    if (secs_to_add == 0) {
        return;
    }
    time_t secs_from_epoch_start = mktime(timestamp);
    time_t current_time = secs_from_epoch_start + secs_to_add;
    localtime_r(&current_time, timestamp);
}

uint32_t wui_gcodes_uploaded() {
    return uploaded_gcodes;
}

uint32_t wui_gcodes_mods() {
    return modified_gcodes;
}

bool wui_start_print(char *filename, bool autostart_if_able) {
    marlin_update_vars(MARLIN_VAR_MSK2(MARLIN_VAR_PRNSTATE, MARLIN_VAR_FILENAME));
    const bool printer_can_print = marlin_remote_print_ready(!autostart_if_able);

    strlcpy(marlin_vars()->media_LFN, basename_b(filename), FILE_NAME_BUFFER_LEN);
    // Turn it into the short name, to improve buffer length, avoid strange
    // chars like spaces in it, etc.
    get_SFN_path(filename);
    if (autostart_if_able) {
        if (printer_can_print) {
            print_begin(filename, true);
            return marlin_print_started();
        } else {
            return false;
        }
    } else {
        // The conditional here is just an optimization, to save bothering
        // marlin. If it couldn't print/initialte the preview, it would just do
        // nothing anyway.
        if (printer_can_print) {
            print_begin(filename, false);
        }
        // We were not asked to print. Showing the preview is "best effort",
        // but not reported to the user.
        return true;
    }
}

bool wui_uploaded_gcode(char *filename, bool start_print) {
    modified_gcodes++;
    uploaded_gcodes++;

    return wui_start_print(filename, start_print);
}

void wui_gcode_modified() {
    modified_gcodes++;
}

bool wui_is_file_being_printed(const char *filename) {
    marlin_update_vars(MARLIN_VAR_MSK2(MARLIN_VAR_PRNSTATE, MARLIN_VAR_FILENAME));
    if (!marlin_is_printing()) {
        return false;
    }

    char sfn[FILE_PATH_BUFFER_LEN];
    strlcpy(sfn, filename, sizeof(sfn));
    get_SFN_path(sfn);
    return strcasecmp(sfn, marlin_vars()->media_SFN_path) == 0;
}
