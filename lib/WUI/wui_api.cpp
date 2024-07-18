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
#include "ini_handler.h"
#include "stm32f4xx_hal.h"
#include "print_utils.hpp"
#include "marlin_client.hpp"

#include <lfn.h>
#include <state/printer_state.hpp>

#include <cassert>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <atomic>

#include <lwip/netif.h>
#include <config_store/store_instance.hpp>

#define USB_MOUNT_POINT        "/usb/"
#define USB_MOUNT_POINT_LENGTH 5

extern RTC_HandleTypeDef hrtc;

static bool sntp_time_init = false;

void wui_marlin_client_init(void) {
    marlin_client::init(); // init the client
    // force update variables when starts
    marlin_client::set_event_notify(marlin_server::EVENT_MSK_DEF);
}

struct ini_load_def {
    // eth::ipv4 or wifi::ipv4
    const char *ip_section;
    // The config to store to (must not be NULL).
    netif_config_t *config;
    // The wifi AP definition. May be NULL (in which case it isn't loaded).
    ap_entry_t *ap;
};

static bool ini_string_match(const char *section, const char *section_var, const char *name, const char *name_var) {
    return strcmp(section_var, section) == 0 && strcmp(name_var, name) == 0;
}

static int ini_handler_func(void *user, const char *section, const char *name, const char *value) {
    ini_load_def *def = (ini_load_def *)user;

    netif_config_t *tmp_config = def->config;

    if (ini_string_match(section, def->ip_section, name, "type")) {
        if (strncasecmp(value, "DHCP", 4) == 0) {
            CHANGE_FLAG_TO_DHCP(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        } else if (strncasecmp(value, "STATIC", 6) == 0) {
            CHANGE_FLAG_TO_STATIC(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        }
    } else if (ini_string_match(section, "network", name, "hostname")) {
        strlcpy(tmp_config->hostname, value, HOSTNAME_LEN + 1);
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
        if (ini_string_match(section, "wifi", name, "ssid")) {
            strlcpy(def->ap->ssid, value, SSID_MAX_LEN + 1);
            tmp_config->var_mask |= ETHVAR_MSK(APVAR_SSID);
        } else if (ini_string_match(section, "wifi", name, "psk")) {
            strlcpy(def->ap->pass, value, WIFI_PSK_MAX + 1);
            tmp_config->var_mask |= ETHVAR_MSK(APVAR_PASS);
        }
    }

    return 1;
}

uint32_t load_ini_file_eth(netif_config_t *config) {
    ini_load_def def = {};
    def.config = config;
    def.ip_section = "eth::ipv4";
    return ini_load_file(ini_handler_func, &def);
}

uint32_t load_ini_file_wifi(netif_config_t *config, ap_entry_t *ap) {
    ini_load_def def = {};
    def.config = config;
    def.ip_section = "wifi::ipv4";
    def.ap = ap;
    return ini_load_file(ini_handler_func, &def);
}

void save_net_params(netif_config_t *ethconfig, ap_entry_t *ap, uint32_t netdev_id) {
    assert(netdev_id == NETDEV_ETH_ID || netdev_id == NETDEV_ESP_ID);

    auto &store = config_store();
    auto transaction = store.get_backend().transaction_guard();

    if (ethconfig->var_mask & (ETHVAR_MSK(ETHVAR_LAN_FLAGS))) {
        uint8_t flags = ethconfig->lan.flag;
        netdev_id == NETDEV_ETH_ID ? store.lan_flag.set(flags) : store.wifi_flag.set(flags);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4)) {
        netdev_id == NETDEV_ETH_ID ? store.lan_ip4_addr.set(ethconfig->lan.addr_ip4.addr)
                                   : store.wifi_ip4_addr.set(ethconfig->lan.addr_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_DNS1_IP4)) {
        netdev_id == NETDEV_ETH_ID ? store.lan_ip4_dns1.set(ethconfig->dns1_ip4.addr)
                                   : store.wifi_ip4_dns1.set(ethconfig->dns1_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_DNS2_IP4)) {
        netdev_id == NETDEV_ETH_ID ? store.lan_ip4_dns2.set(ethconfig->dns2_ip4.addr)
                                   : store.wifi_ip4_dns2.set(ethconfig->dns2_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_MSK_IP4)) {
        netdev_id == NETDEV_ETH_ID ? store.lan_ip4_mask.set(ethconfig->lan.msk_ip4.addr)
                                   : store.wifi_ip4_mask.set(ethconfig->lan.msk_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_GW_IP4)) {
        netdev_id == NETDEV_ETH_ID ? store.lan_ip4_gateway.set(ethconfig->lan.gw_ip4.addr)
                                   : store.wifi_ip4_gateway.set(ethconfig->lan.gw_ip4.addr);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_HOSTNAME)) {
        store.hostname.set(ethconfig->hostname);
    }

    if (ap != NULL) {
        assert(netdev_id == NETDEV_ESP_ID);
        static_assert(SSID_MAX_LEN == config_store_ns::wifi_max_ssid_len);
        static_assert(WIFI_PSK_MAX == config_store_ns::wifi_max_passwd_len);

        if (ethconfig->var_mask & ETHVAR_MSK(APVAR_SSID)) {
            store.wifi_ap_ssid.set(ap->ssid);
        }
        if (ethconfig->var_mask & ETHVAR_MSK(APVAR_PASS)) {
            store.wifi_ap_password.set(ap->pass);
        }
    }
}

void load_net_params(netif_config_t *ethconfig, ap_entry_t *ap, uint32_t netdev_id) {
    assert(netdev_id == NETDEV_ETH_ID || netdev_id == NETDEV_ESP_ID);

    auto &store = config_store();

    // Just the flags, without (possibly) the wifi security
    if (netdev_id == NETDEV_ETH_ID) {
        ethconfig->lan.flag = store.lan_flag.get() & ~RESERVED_MASK;
        ethconfig->lan.addr_ip4.addr = store.lan_ip4_addr.get();
        ethconfig->dns1_ip4.addr = store.lan_ip4_dns1.get();
        ethconfig->dns2_ip4.addr = store.lan_ip4_dns2.get();
        ethconfig->lan.msk_ip4.addr = store.lan_ip4_mask.get();
        ethconfig->lan.gw_ip4.addr = store.lan_ip4_gateway.get();
    } else {
        ethconfig->lan.flag = store.wifi_flag.get() & ~RESERVED_MASK;
        ethconfig->lan.addr_ip4.addr = store.wifi_ip4_addr.get();
        ethconfig->dns1_ip4.addr = store.wifi_ip4_dns1.get();
        ethconfig->dns2_ip4.addr = store.wifi_ip4_dns2.get();
        ethconfig->lan.msk_ip4.addr = store.wifi_ip4_mask.get();
        ethconfig->lan.gw_ip4.addr = store.wifi_ip4_gateway.get();
    }

    strlcpy(ethconfig->hostname, store.hostname.get_c_str(), HOSTNAME_LEN + 1);

    if (ap != NULL) {
        assert(netdev_id == NETDEV_ESP_ID);

        strlcpy(ap->ssid, store.wifi_ap_ssid.get_c_str(), SSID_MAX_LEN + 1);
        strlcpy(ap->pass, store.wifi_ap_password.get_c_str(), WIFI_PSK_MAX + 1);
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
    RTC_TimeTypeDef currTime {};
    RTC_DateTypeDef currDate {};

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

StartPrintResult wui_start_print(char *filename, bool autostart_if_able) {

    const bool printer_can_print = printer_state::remote_print_ready(!autostart_if_able);

    // Turn it into the short name, to improve buffer length, avoid strange
    // chars like spaces in it, etc.
    get_SFN_path(filename);

    if (autostart_if_able) {
        if (printer_can_print) {
            print_begin(filename, marlin_server::PreviewSkipIfAble::all);
            return marlin_client::is_print_started() ? StartPrintResult::PrintStarted : StartPrintResult::Failed;
        } else {
            return StartPrintResult::Failed;
        }
    } else {
        // The conditional here is just an optimization, to save bothering
        // marlin. If it couldn't print/initialte the preview, it would just do
        // nothing anyway.
        if (printer_can_print) {
            print_begin(filename);
        }
        // We were not asked to print. Showing the preview is "best effort",
        // but not reported to the user.
        return StartPrintResult::Uploaded;
    }
}

bool wui_uploaded_gcode(char *filename, bool start_print) {
    StartPrintResult res = wui_start_print(filename, start_print);

    return (res != StartPrintResult::Failed);
}

bool wui_is_file_being_printed(const char *filename) {

    if (!marlin_client::is_printing()) {
        return false;
    }

    char sfn[FILE_PATH_BUFFER_LEN];
    strlcpy(sfn, filename, sizeof(sfn));
    get_SFN_path(sfn);
    return marlin_vars().media_SFN_path.equals(sfn);
}

bool wui_media_inserted() {
    return marlin_vars().media_inserted;
}
