/*
 * wui_api.c
 * \brief   interface functions for Web User Interface library
 *
 *  Created on: April 22, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */
#include "wui_api.h"
#include "wui.h"
#include "version.h"
#include "otp.h"
#include <string.h>
#include <stdio.h>
#include "ini_handler.h"
#include "eeprom.h"
#include "string.h"
#include <stdbool.h>
#include <time.h>
#include "main.h"
#include "stm32f4xx_hal.h"

// static const uint16_t MAX_UINT16 = 65535;
static const uint32_t PRINTER_TYPE_ADDR = 0x0802002F;    // 1 B
static const uint32_t PRINTER_VERSION_ADDR = 0x08020030; // 1 B

static bool sntp_time_init = false;

uint32_t load_ini_file(ETH_config_t *config) {
    return ini_load_file(config);
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

void get_printer_info(printer_info_t *printer_info) {
    // FIRMWARE VERSION
    strlcpy(printer_info->firmware_version, project_version_full, FW_VER_STR_LEN);
    // PRINTER TYPE
    printer_info->printer_type = *(volatile uint8_t *)PRINTER_TYPE_ADDR;
    // PRINTER_VERSION
    printer_info->printer_version = *(volatile uint8_t *)PRINTER_VERSION_ADDR;
    // MAC ADDRESS
    parse_MAC_address(&printer_info->mac_address);
    // SERIAL NUMBER
    for (int i = 0; i < OTP_SERIAL_NUMBER_SIZE; i++) {
        printer_info->serial_number[i] = *(volatile char *)(OTP_SERIAL_NUMBER_ADDR + i);
    }
    // UUID - 96 bits
    volatile uint32_t *uuid_ptr = (volatile uint32_t *)OTP_STM32_UUID_ADDR;
    snprintf(printer_info->mcu_uuid, UUID_STR_LEN, "%08lx-%08lx-%08lx", *uuid_ptr, *(uuid_ptr + 1), *(uuid_ptr + 2));
}

void parse_MAC_address(mac_address_t *dest) {
    volatile uint8_t *mac_ptr = (volatile uint8_t *)OTP_MAC_ADDRESS_ADDR;
    snprintf(*dest, MAC_ADDR_STR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
        *mac_ptr, *(mac_ptr + 1), *(mac_ptr + 2), *(mac_ptr + 3), *(mac_ptr + 4), *(mac_ptr + 5));
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

void stringify_eth_for_screen(lan_descp_str_t *dest, ETH_config_t *config) {
    char addr[IP4_ADDR_STR_SIZE], msk[IP4_ADDR_STR_SIZE], gw[IP4_ADDR_STR_SIZE];
    mac_address_t mac;
    parse_MAC_address(&mac);

    ip4addr_ntoa_r(&(config->lan.addr_ip4), addr, IP4_ADDR_STR_SIZE);
    ip4addr_ntoa_r(&(config->lan.msk_ip4), msk, IP4_ADDR_STR_SIZE);
    ip4addr_ntoa_r(&(config->lan.gw_ip4), gw, IP4_ADDR_STR_SIZE);

    snprintf(*dest, LAN_DESCP_SIZE, "IPv4 Address:\n%s\nIPv4 Netmask:\n%s\nIPv4 Gateway:\n%s\nMAC Address:\n%s",
        addr, msk, gw, mac);
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
