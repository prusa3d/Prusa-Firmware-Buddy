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
#include "netdev.h"

#include "eeprom.h"
#include "variant8.h"
#include "netifapi.h"
#include "ethernetif.h"

#include "esp/esp.h"
#include "netifapi.h"
#include "dns.h"
#include "netif_settings.h"
#include "dbg.h"
#include "wui_api.h"
#include "alsockets.h"
#include "lwesp_ll_buddy.h"

typedef struct netdev_device_s {
    ETH_config_t config;
    netdev_status_t status;
} netdev_device_t;

static const uint32_t esp_target_baudrate = 4600000;
static uint32_t active_netdev_id = NETDEV_NODEV_ID;
static netdev_device_t netdev_devices[NETDEV_COUNT];

struct netif eth0; // network interface structure for ETH
static ap_entry_t ap = { "", "" };

extern osMessageQId networkMbox_id;
extern struct alsockets_s *alsockets_eth();
extern struct alsockets_s *alsockets_esp();

static struct alsockets_s *netdev_get_sockets(uint32_t);

#define ETH_DEV (netdev_devices[NETDEV_ETH_ID])
#define ESP_DEV (netdev_devices[NETDEV_ESP_ID])

#define DNS_1 0
#define DNS_2 1

static void netif_link_callback(struct netif *eth) {
    ethernetif_update_config(eth);
    eth->hostname = ETH_DEV.config.hostname;
    osMessagePut(networkMbox_id, EVT_NETDEV_INIT_FINISHED(NETDEV_ETH_ID, 0), 0);
}

static void netif_status_callback(struct netif *eth) {
    ethernetif_update_config(eth);
    ETH_DEV.status = !ethernetif_link(&eth0) ? NETDEV_UNLINKED : NETDEV_NETIF_UP;
}

static void tcpip_init_done_callback(void *arg) {
    if (netif_add_noaddr(&eth0, NULL, ethernetif_init, tcpip_input)) {
        netif_set_link_callback(&eth0, netif_link_callback);
        netif_set_status_callback(&eth0, netif_status_callback);
    }
    wui_marlin_client_init();
    osMessagePut(networkMbox_id, EVT_TCPIP_INIT_FINISHED, 0);
}

void get_eth_address(uint32_t netdev_id, ETH_config_t *config) {
    if (netdev_id == NETDEV_ETH_ID) {
        config->lan.addr_ip4.addr = netif_ip4_addr(&eth0)->addr;
        config->lan.msk_ip4.addr = netif_ip4_netmask(&eth0)->addr;
        config->lan.gw_ip4.addr = netif_ip4_gw(&eth0)->addr;
    } else if (netdev_id == NETDEV_ESP_ID) {
        esp_ip_t ip, mask, gw;
        esp_sta_getip(&ip, &gw, &mask, NULL, NULL, 1);
        config->lan.addr_ip4.addr = *(uint32_t *)ip.ip;
        config->lan.msk_ip4.addr = *(uint32_t *)mask.ip;
        config->lan.gw_ip4.addr = *(uint32_t *)gw.ip;
    } else {
        config->lan.addr_ip4.addr = 0;
        config->lan.msk_ip4.addr = 0;
        config->lan.gw_ip4.addr = 0;
    }
}

/**
 * \brief         Event callback function for ESP baurate change
 * \param[in]     res: Baudrate change result
 * \param[in]     arg: event data, new baudarate
 */
static void esp_baudrate_changed(espr_t res, void *arg) {
    if (res != espOK) {
        _dbg("ESP baudrate change failed !!!");
        return;
    }

    _dbg("ESP baudrate change success, reconfiguring UART for %" PRIu32, esp_target_baudrate);
    esp_reconfigure_uart(esp_target_baudrate);
}

/**
 * \brief           Event callback function for ESP stack
 * \param[in]       evt: Event information with data
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_callback_func(esp_evt_t *evt) {
    switch (esp_evt_get_type(evt)) {
    case ESP_EVT_AT_VERSION_NOT_SUPPORTED: {
        esp_sw_version_t v_min, v_curr;

        esp_get_min_at_fw_version(&v_min);
        esp_get_current_at_fw_version(&v_curr);

        _dbg("Current ESP8266 AT version is not supported by library!");
        _dbg("Minimum required AT version is: %d.%d.%d", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
        _dbg("Current AT version is: %d.%d.%d", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
        break;
    }
    case ESP_EVT_INIT_FINISH: {
        esp_set_wifi_mode(ESP_MODE_STA, NULL, NULL, 1);
        ESP_DEV.status = NETDEV_UNLINKED;
        osMessagePut(networkMbox_id, EVT_LWESP_INIT_FINISHED, 0);
        _dbg("ESP_EVT_INIT_FINISH");
        break;
    }
    case ESP_EVT_RESET: {
        esp_set_at_baudrate(esp_target_baudrate, esp_baudrate_changed, NULL, 0);
        _dbg("ESP_EVT_RESET");
        break;
    }
    case ESP_EVT_RESET_DETECTED: {
        esp_reconfigure_uart(ESP_CFG_AT_PORT_BAUDRATE);
        _dbg("ESP_EVT_RESET_DETECTED");
        break;
    }
    case ESP_EVT_WIFI_GOT_IP: {
        _dbg("ESP_EVT_WIFI_GOT_IP");
        ESP_DEV.status = NETDEV_NETIF_UP;
        break;
    }
    case ESP_EVT_WIFI_CONNECTED: {
        uint32_t message = EVT_NETDEV_INIT_FINISHED(NETDEV_ESP_ID, 0);
        _dbg("ESP_EVT_WIFI_CONNECTED");
        osMessagePut(networkMbox_id, message, 0);
        break;
    }
    case ESP_EVT_WIFI_DISCONNECTED: {
        _dbg("ESP_EVT_WIFI_DISCONNECTED");
        ESP_DEV.status = NETDEV_UNLINKED;
        break;
    }
    case ESP_EVT_SERVER: {
        _dbg("ESP_EVT_SERVER");
        break;
    }
    case ESP_EVT_WIFI_IP_ACQUIRED: {
        _dbg("ESP_EVT_WIFI_IP_ACQUIRED");
        break;
    }
    case ESP_EVT_STA_JOIN_AP: {
        _dbg("ESP_EVT_STA_JOIN_AP");
        break;
    }
    case ESP_EVT_CMD_TIMEOUT: {
        _dbg("ESP_EVT_CMD_TIMEOUT");
        break;
    }
    default: {
        _dbg("Unknown ESP message: %d", (int)esp_evt_get_type(evt));
        break;
    }
    }
    return espOK;
}

static void alsockets_adjust() {
    alsockets_funcs(netdev_get_sockets(active_netdev_id));
}

uint32_t netdev_init() {
    ETH_DEV.config.var_mask = ETHVAR_EEPROM_CONFIG;
    load_net_params(&ETH_DEV.config, NULL, NETDEV_ETH_ID);
    ESP_DEV.config.var_mask = ETHVAR_EEPROM_CONFIG | APVAR_EEPROM_CONFIG;
    load_net_params(&ESP_DEV.config, &ap, NETDEV_ESP_ID);
    active_netdev_id = variant8_get_ui8(eeprom_get_var(EEVAR_ACTIVE_NETDEV));

    tcpip_init(tcpip_init_done_callback, NULL);
    netdev_init_esp();

    alsockets_adjust();
    return 0;
}

uint32_t netdev_init_esp() {
    esp_init(esp_callback_func, 0);
    return 0;
}

uint32_t netdev_get_active_id() {
    return active_netdev_id;
}

uint32_t netdev_set_active_id(uint32_t netdev_id) {
    if (netdev_get_active_id() == netdev_id) {
        return 0xFFFFFFFF;
    }

    if (netdev_id > NETDEV_COUNT) {
        netdev_id = NETDEV_NODEV_ID;
    }

    active_netdev_id = netdev_id;
    eeprom_set_var(EEVAR_ACTIVE_NETDEV, variant8_ui8((uint8_t)(netdev_id & 0xFF)));

    alsockets_adjust();
    return 0;
}

bool get_current_ipv4(uint8_t *dest) {
    /*
     * FIXME: We currently don't have synchronization of network-related
     * variables solved. This probably goes to
     * https://dev.prusa3d.com/browse/BFW-2198.
     *
     * Technically, this is UB (a data race), but on the current architecture,
     * this will likely not end up in anything worse than outdated info in some
     * rare cases. So we dare to postpone this for the above ticket.
     */
    uint32_t id = netdev_get_active_id();
    switch (id) {
    case NETDEV_ETH_ID:
    case NETDEV_ESP_ID:
        memcpy(dest, &netdev_devices[id].config.lan.addr_ip4, 4);
        return true;
    case NETDEV_NODEV_ID:
        return false;
    default:
        assert(0 /* Unhandled/invalid active_netdev_id */);
        return false;
    }
}

netdev_status_t netdev_get_status(uint32_t netdev_id) {
    if (netdev_id < NETDEV_NODEV_ID) {
        if (netdev_id == NETDEV_ETH_ID) {
            ETH_DEV.status = !ethernetif_link(&eth0) ? NETDEV_UNLINKED : NETDEV_NETIF_UP;
        }
        return netdev_devices[netdev_id].status;
    } else {
        return NETDEV_NETIF_DOWN;
    }
}

netdev_ip_obtained_t netdev_get_ip_obtained_type(uint32_t netdev_id) {
    if (netdev_id < NETDEV_COUNT) {
        return IS_LAN_STATIC(netdev_devices[netdev_id].config.lan.flag) ? NETDEV_STATIC : NETDEV_DHCP;
    } else {
        return NETDEV_DHCP;
    }
}

uint32_t netdev_set_dhcp(uint32_t netdev_id) {
    ETH_config_t *pConfig = NULL;
    err_t res = ERR_OK;

    if (netdev_id == NETDEV_ETH_ID) {
        netdev_devices[netdev_id].status = NETDEV_UNLINKED;
        res = netifapi_dhcp_start(&eth0);
        pConfig = &netdev_devices[netdev_id].config;
    } else if (netdev_id == NETDEV_ESP_ID) {
        // ESP automaticaly obtain IP address from DHCP server after it joins
        // to the network therefore we use such a feature during the switch between
        // static and dynamic IP because there is no API call to invoke DHCP client.
        esp_sta_quit(NULL, NULL, 1);
        res = netdev_set_up(NETDEV_ESP_ID);
        pConfig = &netdev_devices[netdev_id].config;
    }

    if (pConfig != NULL) {
        CHANGE_FLAG_TO_DHCP(pConfig->lan.flag);
        pConfig->var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        save_net_params(pConfig, NULL, netdev_id);
        pConfig->var_mask = 0;
        return res;
    } else {
        res = ERR_IF;
    }

    return res;
}

uint32_t netdev_set_up(uint32_t netdev_id) {

    if (netdev_id == NETDEV_ETH_ID) {
        netifapi_netif_set_default(&eth0);
        netifapi_netif_set_link_up(&eth0);
        return netifapi_netif_set_up(&eth0);
    } else if (netdev_id == NETDEV_ESP_ID) {
        const char *passwd;
        switch (ap.security) {
        case AP_SEC_NONE:
            passwd = NULL;
            break;
        case AP_SEC_WEP:
        case AP_SEC_WPA:
            passwd = ap.pass;
            break;
        default:
            assert(0 /* Unhandled AP_SEC_* value*/);
            return ERR_ARG;
        }
        esp_sta_join(ap.ssid, passwd, NULL, NULL, NULL, 0);
        return ERR_OK;
    } else {
        return ERR_IF;
    }
}

bool netdev_load_ini_to_eeprom() {
    if ((load_ini_file_eth(&netdev_devices[NETDEV_ETH_ID].config) != 1) || (load_ini_file_wifi(&netdev_devices[NETDEV_ESP_ID].config, &ap) != 1)) {
        return false;
    }

    // Yes, indeed, the load functions return 1 on success, these save return 0 on success...
    if ((save_net_params(&netdev_devices[NETDEV_ETH_ID].config, NULL, NETDEV_ETH_ID) != 0) || (save_net_params(&netdev_devices[NETDEV_ESP_ID].config, &ap, NETDEV_ESP_ID))) {
        return false;
    }

    return true;
}

uint32_t netdev_set_down(uint32_t netdev_id) {

    if (netdev_id == NETDEV_ETH_ID) {
        netifapi_netif_set_link_down(&eth0);
        return netifapi_netif_set_down(&eth0);
    } else if (netdev_id == NETDEV_ESP_ID) {
        esp_sta_quit(NULL, NULL, 0);
        return ERR_OK;
    } else {
        return ERR_IF;
    }
}

uint32_t netdev_set_static(uint32_t netdev_id) {
    ETH_config_t *pConfig = NULL;
    err_t res = ERR_OK;

    if (netdev_id == NETDEV_ETH_ID) {
        pConfig = &netdev_devices[netdev_id].config;
        netifapi_netif_set_up(&eth0);
        dns_setserver(DNS_1, &pConfig->dns1_ip4);
        dns_setserver(DNS_2, &pConfig->dns2_ip4);
        res = netifapi_netif_set_addr(&eth0, &pConfig->lan.addr_ip4, &pConfig->lan.msk_ip4, &pConfig->lan.gw_ip4);
        res = netifapi_dhcp_inform(&eth0);
    } else if (netdev_id == NETDEV_ESP_ID) {
        pConfig = &netdev_devices[netdev_id].config;
        esp_sta_setip((esp_ip_t *)&pConfig->lan.addr_ip4, (esp_ip_t *)&pConfig->lan.gw_ip4, (esp_ip_t *)&pConfig->lan.msk_ip4, NULL, NULL, 1);
        res = ERR_OK;
    }

    if (pConfig != NULL) {
        CHANGE_FLAG_TO_STATIC(pConfig->lan.flag);
        pConfig->var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        save_net_params(pConfig, NULL, netdev_id);
        pConfig->var_mask = 0;
        return res;
    } else {
        res = ERR_IF;
    }

    return res;
}

uint32_t netdev_check_link(uint32_t dev_id) {
    if (dev_id == NETDEV_ESP_ID) {
        esp_sw_version_t version;
        if (esp_get_current_at_fw_version(&version)) {
            ESP_DEV.status = NETDEV_UNLINKED;
        }
    }
    return 0;
}

static struct alsockets_s *netdev_get_sockets(uint32_t active_id) {
    if (active_id == NETDEV_ETH_ID)
        return alsockets_eth();
    else if (active_id == NETDEV_ESP_ID) {
        return alsockets_esp();
    } else {
        return NULL;
    }
}

const char *netdev_get_hostname(uint32_t active_id) {
    if (active_id < NETDEV_COUNT) {
        return netdev_devices[active_id].config.hostname;
    } else {
        return "";
    }
}
