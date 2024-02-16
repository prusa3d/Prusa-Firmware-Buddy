/*
 * This file is part of the Prusa Firmware Buddy distribution (https://github.com/prusa3d/Prusa-Firmware-Buddy ).
 * Copyright (c) 2021 Marek Mosna.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <string.h>
#include "netdev.h"

#include "netifapi.h"
#include "ethernetif.h"

#include "netifapi.h"
#include "dns.h"
#include "netif_settings.h"
#include "wui_api.h"
#include "espif.h"

bool netdev_load_ini_to_eeprom() {
    ETH_config_t config[NETDEV_COUNT] = {};
    ap_entry_t ap = {};
    /*
     * Load current values, so the things that are not present in the ini are
     * left in the original form.
     */
    load_net_params(&config[NETDEV_ETH_ID], NULL, NETDEV_ETH_ID);
    load_net_params(&config[NETDEV_ESP_ID], &ap, NETDEV_ESP_ID);
    if ((load_ini_file_eth(&config[NETDEV_ETH_ID]) != 1) || (load_ini_file_wifi(&config[NETDEV_ESP_ID], &ap) != 1)) {
        return false;
    }

    save_net_params(&config[NETDEV_ETH_ID], NULL, NETDEV_ETH_ID);
    save_net_params(&config[NETDEV_ESP_ID], &ap, NETDEV_ESP_ID);

    return true;
}

bool netdev_load_esp_credentials_eeprom() {
    ETH_config_t cnf = {}; // to store current config, to be able to set it back
    ETH_config_t cnf_dummy = {}; // just to read config from ini to something and discard it
    ap_entry_t ap = {};
    /*
     * Load current values, so the things that are not present in the ini are
     * left in the original form.
     */
    load_net_params(&cnf, &ap, NETDEV_ESP_ID);
    if (load_ini_file_wifi(&cnf_dummy, &ap) != 1) { // cnf will be discarded
        return false;
    }

    strncpy(cnf.hostname, cnf_dummy.hostname, sizeof(cnf.hostname));
    cnf.var_mask = ETHVAR_MSK(APVAR_PASS) + ETHVAR_MSK(APVAR_SSID) + ETHVAR_MSK(ETHVAR_LAN_FLAGS) + ETHVAR_MSK(ETHVAR_HOSTNAME);
    cnf.lan.flag = cnf_dummy.lan.flag; // should be 0 == ON, DHCP, WIFI

    save_net_params(&cnf, &ap, NETDEV_ESP_ID);

    return ap.ssid[0] != '\0';
}

ap_entry_t netdev_read_esp_credentials_eeprom() {
    ETH_config_t cnf = {};
    ap_entry_t ap = {};

    load_net_params(&cnf, &ap, NETDEV_ESP_ID);

    return ap;
}
