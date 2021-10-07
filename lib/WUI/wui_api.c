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
#include "version.h"
#include "otp.h"
#include "ini_handler.h"
#include "eeprom.h"
#include "stm32f4xx_hal.h"
#include "print_utils.hpp"
#include "marlin_client.h"

#include <time.h>
#include <string.h>
#include <stdio.h>

#define USB_MOUNT_POINT        "/usb/"
#define USB_MOUNT_POINT_LENGTH 5

extern RTC_HandleTypeDef hrtc;

uint32_t start_print = 0;
char filename[FILE_NAME_MAX_LEN];

static FILE *upload_file = NULL;
static char tmp_filename[FILE_NAME_MAX_LEN];
static bool sntp_time_init = false;
static char wui_media_LFN[FILE_NAME_MAX_LEN + 1]; // static buffer for gcode file name

void wui_marlin_client_init(void) {
    marlin_vars_t *vars = marlin_client_init(); // init the client
    // force update variables when starts
    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF - MARLIN_EVT_MSK_FSM, NULL);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI, NULL);
    if (vars) {
        vars->media_LFN = wui_media_LFN;
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
    struct ini_load_def *def = user;

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
        // FIXME: Do we follow the ticket or the doc/network.ini?
        if (ini_string_match(section, "wifi", name, "key_mgmt")) {
            if (strcasecmp("WPA", value) == 0) {
                def->ap->security = AP_SEC_WPA;
            } else if (strcasecmp("WEP", value) == 0) {
                def->ap->security = AP_SEC_WEP;
            } else if (strcasecmp("NONE", value) == 0) {
                def->ap->security = AP_SEC_NONE;
            }
            // TODO: else -> ??? Any way to tell the user?
        } else if (ini_string_match(section, "wifi", name, "ssid")) {
            strlcpy(def->ap->ssid, value, SSID_MAX_LEN + 1);
        } else if (ini_string_match(section, "wifi", name, "psk")) {
            strlcpy(def->ap->pass, value, WIFI_PSK_MAX + 1);
        }
    }

    return 1;
}

uint32_t load_ini_file_eth(ETH_config_t *config) {
    return ini_load_file(ini_handler_func, &(struct ini_load_def) {
                                               .config = config,
                                               .ip_section = "eth::ipv4",
                                           });
}

uint32_t load_ini_file_wifi(ETH_config_t *config, ap_entry_t *ap) {
    return ini_load_file(ini_handler_func, &(struct ini_load_def) {
                                               .config = config,
                                               .ip_section = "wifi::ipv4",
                                               .ap = ap,
                                           });
}

uint32_t save_eth_params(ETH_config_t *ethconfig) {

    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_FLAGS)) {
        eeprom_set_var(EEVAR_LAN_FLAG, variant8_ui8(ethconfig->lan.flag));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4)) {
        eeprom_set_var(EEVAR_LAN_IP4_ADDR, variant8_ui32(ethconfig->lan.addr_ip4.addr));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_DNS1_IP4)) {
        eeprom_set_var(EEVAR_LAN_IP4_DNS1, variant8_ui32(ethconfig->dns1_ip4.addr));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_DNS2_IP4)) {
        eeprom_set_var(EEVAR_LAN_IP4_DNS2, variant8_ui32(ethconfig->dns2_ip4.addr));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_MSK_IP4)) {
        eeprom_set_var(EEVAR_LAN_IP4_MSK, variant8_ui32(ethconfig->lan.msk_ip4.addr));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_GW_IP4)) {
        eeprom_set_var(EEVAR_LAN_IP4_GW, variant8_ui32(ethconfig->lan.gw_ip4.addr));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_HOSTNAME)) {
        variant8_t hostname = variant8_pchar(ethconfig->hostname, 0, 0);
        eeprom_set_var(EEVAR_LAN_HOSTNAME, hostname);
        //variant8_done() is not called, variant_pchar with init flag 0 doesnt hold its memory
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_TIMEZONE)) {
        eeprom_set_var(EEVAR_TIMEZONE, variant8_i8(ethconfig->timezone));
    }

    return 0;
}

uint32_t load_eth_params(ETH_config_t *ethconfig) {

    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_FLAGS)) {
        ethconfig->lan.flag = variant8_get_ui8(eeprom_get_var(EEVAR_LAN_FLAG));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4)) {
        ethconfig->lan.addr_ip4.addr = variant8_get_ui32(eeprom_get_var(EEVAR_LAN_IP4_ADDR));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_DNS1_IP4)) {
        ethconfig->dns1_ip4.addr = variant8_get_ui32(eeprom_get_var(EEVAR_LAN_IP4_DNS1));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_DNS2_IP4)) {
        ethconfig->dns2_ip4.addr = variant8_get_ui32(eeprom_get_var(EEVAR_LAN_IP4_DNS2));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_MSK_IP4)) {
        ethconfig->lan.msk_ip4.addr = variant8_get_ui32(eeprom_get_var(EEVAR_LAN_IP4_MSK));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_LAN_GW_IP4)) {
        ethconfig->lan.gw_ip4.addr = variant8_get_ui32(eeprom_get_var(EEVAR_LAN_IP4_GW));
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_HOSTNAME)) {
        variant8_t hostname = eeprom_get_var(EEVAR_LAN_HOSTNAME);
        variant8_t *pvar = &hostname;
        strlcpy(ethconfig->hostname, variant8_get_pch(hostname), ETH_HOSTNAME_LEN + 1);
        variant8_done(&pvar);
    }
    if (ethconfig->var_mask & ETHVAR_MSK(ETHVAR_TIMEZONE)) {
        ethconfig->timezone = variant8_get_i8(eeprom_get_var(EEVAR_TIMEZONE));
    }

    return 0;
}

void stringify_eth_for_ini(ini_file_str_t *dest, ETH_config_t *config) {
    char addr[IP4_ADDR_STR_SIZE], msk[IP4_ADDR_STR_SIZE], gw[IP4_ADDR_STR_SIZE];
    char dns1[IP4_ADDR_STR_SIZE], dns2[IP4_ADDR_STR_SIZE];

    ip4addr_ntoa_r(&(config->lan.addr_ip4), addr, IP4_ADDR_STR_SIZE);
    ip4addr_ntoa_r(&(config->lan.msk_ip4), msk, IP4_ADDR_STR_SIZE);
    ip4addr_ntoa_r(&(config->lan.gw_ip4), gw, IP4_ADDR_STR_SIZE);
    ip4addr_ntoa_r(&(config->dns1_ip4), dns1, IP4_ADDR_STR_SIZE);
    ip4addr_ntoa_r(&(config->dns2_ip4), dns2, IP4_ADDR_STR_SIZE);

    snprintf(*dest, MAX_INI_SIZE,
        "[eth::ipv4]\ntype=%s\naddr=%s\nmask=%s\ngw=%s\n\n"
        "[network]\nhostname=%s\ndns4=%s;%s",
        IS_LAN_STATIC(config->lan.flag) ? "STATIC" : "DHCP", addr, msk, gw,
        config->hostname, dns1, dns2);
}

time_t sntp_get_system_time(void) {

    if (sntp_time_init) {
        RTC_TimeTypeDef currTime;
        RTC_DateTypeDef currDate;
        HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &currDate, RTC_FORMAT_BIN);
        time_t secs;
        struct tm system_time;
        system_time.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown
        system_time.tm_hour = currTime.Hours;
        system_time.tm_min = currTime.Minutes;
        system_time.tm_sec = currTime.Seconds;
        system_time.tm_mday = currDate.Date;
        system_time.tm_mon = currDate.Month;
        system_time.tm_year = currDate.Year;
        system_time.tm_wday = currDate.WeekDay;
        secs = mktime(&system_time);
        return secs;
    } else {
        return 0;
    }
}

void sntp_set_system_time(uint32_t sec, int8_t last_timezone) {
    ETH_config_t config = {};
    config.var_mask = ETHVAR_MSK(ETHVAR_TIMEZONE);
    load_eth_params(&config);

    RTC_TimeTypeDef currTime;
    RTC_DateTypeDef currDate;

    struct tm current_time_val;
    int8_t diff = config.timezone - last_timezone;
    time_t current_time = (time_t)sec + (diff * 3600);

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

uint32_t wui_upload_begin(const char *fname) {
    uint32_t fname_length = strlen(fname);
    if ((fname_length + USB_MOUNT_POINT_LENGTH) < FILE_NAME_MAX_LEN) {
        strcpy(tmp_filename, USB_MOUNT_POINT);
        strcpy(tmp_filename + USB_MOUNT_POINT_LENGTH, fname);
        upload_file = fopen(tmp_filename, "w");
    }
    return upload_file == NULL;
}

uint32_t wui_upload_data(const char *data, uint32_t length) {
    return fwrite(data, sizeof(char), length, upload_file);
}

uint32_t wui_upload_finish(const char *old_filename, const char *new_filename, uint32_t start) {
    uint32_t fname_length = strlen(new_filename);
    uint32_t error_code = 200;
    int result = 0;

    fclose(upload_file);

    if (!strstr(new_filename, "gcode")) {
        error_code = 415;
        goto clean_temp_file;
    }

    if ((fname_length + USB_MOUNT_POINT_LENGTH) >= FILE_NAME_MAX_LEN) {
        error_code = 409;
        goto clean_temp_file;
    } else {
        strlcpy(filename, USB_MOUNT_POINT, USB_MOUNT_POINT_LENGTH + 1);
        strlcat(filename, new_filename, FILE_PATH_MAX_LEN - USB_MOUNT_POINT_LENGTH);
    }

    result = rename(tmp_filename, filename);
    if (result != 0) {
        error_code = 409;
        goto clean_temp_file;
    }

    if (marlin_vars()->sd_printing && start) {
        error_code = 409;
        goto return_error_code;
    } else {
        if (start) {
            strlcpy(marlin_vars()->media_LFN, new_filename, FILE_PATH_MAX_LEN);
            print_begin(filename);
        }
        start_print = start;
        goto return_error_code;
    }

clean_temp_file:
    remove(tmp_filename);
return_error_code:
    return error_code;
}
