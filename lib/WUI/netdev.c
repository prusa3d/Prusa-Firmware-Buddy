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

#include "netifapi.h"
#include "dns.h"
#include "netif_settings.h"
#include "wui_api.h"
#include "alsockets.h"
#include "espif.h"
#include "otp.h"

#define _dbg(...)

static uint32_t active_netdev_id = NETDEV_NODEV_ID;
/*
 * Desired configuration for each interface.
 */
#ifdef _DEBUG
static ETH_config_t wui_netdev_config[NETDEV_COUNT] = {
    { .hostname = "debug-eth-MINI" },
    { .hostname = "debug-esp-MINI" }
}; // the active WUI configuration for ethernet, connect and server
#else
static ETH_config_t wui_netdev_config[NETDEV_COUNT] = {};
#endif

struct netif eth0;  // network interface structure for ETH
struct netif wlan0; // network interface structure for ESP Wifi
static ap_entry_t ap = { "", "" };

extern osMessageQId networkMbox_id;
extern struct alsockets_s *alsockets_eth();

static struct alsockets_s *netdev_get_sockets(uint32_t);

#define ETH_CONFIG() wui_netdev_config[NETDEV_ETH_ID]
#define ESP_CONFIG() wui_netdev_config[NETDEV_ESP_ID]

#define DNS_1 0
#define DNS_2 1

static void netif_link_callback(struct netif *eth) {
    ethernetif_update_config(eth);
    eth->hostname = ETH_CONFIG().hostname;
    if (eth == &eth0) {
        osMessagePut(networkMbox_id, EVT_NETDEV_INIT_FINISHED(NETDEV_ETH_ID, 0), 0);
    }
    if (eth == &wlan0) {
        osMessagePut(networkMbox_id, EVT_NETDEV_INIT_FINISHED(NETDEV_ESP_ID, 0), 0);
    }
}

static void netif_status_callback(struct netif *eth) {
    ethernetif_update_config(eth);
}

static void tcpip_init_done_callback(void *arg) {
    if (netif_add_noaddr(&eth0, NULL, ethernetif_init, tcpip_input)) {
        netif_set_link_callback(&eth0, netif_link_callback);
        netif_set_status_callback(&eth0, netif_status_callback);
    }
    if (netif_add_noaddr(&wlan0, NULL, espif_init, tcpip_input)) {
        netif_set_link_callback(&wlan0, netif_link_callback);
        netif_set_status_callback(&wlan0, netif_status_callback);
    }
    wui_marlin_client_init();
    osMessagePut(networkMbox_id, EVT_TCPIP_INIT_FINISHED, 0);
}

static struct netif *get_netif_by_id(uint32_t netdev_id) {
    switch (netdev_id) {
    case NETDEV_ETH_ID:
        return &eth0;
    case NETDEV_ESP_ID:
        return &wlan0;
    default:
        return NULL;
    }
}

void netdev_get_ipv4_addresses(uint32_t netdev_id, lan_t *config) {
    struct netif *dev = get_netif_by_id(netdev_id);
    if (!dev) {
        config->addr_ip4.addr = 0;
        config->msk_ip4.addr = 0;
        config->gw_ip4.addr = 0;
        return;
    }
    config->addr_ip4.addr = netif_ip4_addr(dev)->addr;
    config->msk_ip4.addr = netif_ip4_netmask(dev)->addr;
    config->gw_ip4.addr = netif_ip4_gw(dev)->addr;
}

void netdev_get_MAC_address(uint32_t netdev_id, uint8_t mac[OTP_MAC_ADDRESS_SIZE]) {
    if (netdev_id == NETDEV_ETH_ID) {
        // TODO: Why not to copy address from netif?
        memcpy(mac, (void *)OTP_MAC_ADDRESS_ADDR, OTP_MAC_ADDRESS_SIZE);
    } else if (netdev_id == NETDEV_ESP_ID) {
        memcpy(mac, wlan0.hwaddr, wlan0.hwaddr_len);
    } else {
        memset(mac, 0, OTP_MAC_ADDRESS_SIZE);
    }
}

static void alsockets_adjust() {
    // TODO: Drop alternative sockets
    alsockets_funcs(netdev_get_sockets(active_netdev_id));
}

uint32_t netdev_init() {
    ETH_CONFIG().var_mask = ETHVAR_EEPROM_CONFIG;
    load_net_params(&ETH_CONFIG(), NULL, NETDEV_ETH_ID);
    ESP_CONFIG().var_mask = ETHVAR_EEPROM_CONFIG | APVAR_EEPROM_CONFIG;
    load_net_params(&ESP_CONFIG(), &ap, NETDEV_ESP_ID);
    active_netdev_id = variant8_get_ui8(eeprom_get_var(EEVAR_ACTIVE_NETDEV));

    tcpip_init(tcpip_init_done_callback, NULL);

    alsockets_adjust();
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

bool netdev_get_current_ipv4(uint8_t *dest) {
    uint32_t id = netdev_get_active_id();
    switch (id) {
    case NETDEV_ETH_ID:
    case NETDEV_ESP_ID: {
        lan_t result = {};
        netdev_get_ipv4_addresses(id, &result);
        memcpy(dest, &result.addr_ip4, 4);
        return true;
    }
    case NETDEV_NODEV_ID:
        return false;
    default:
        assert(0 /* Unhandled/invalid active_netdev_id */);
        return false;
    }
}

static uint32_t netif_link(struct netif *dev) {
    if (dev == &eth0) {
        return ethernetif_link(dev);
    }
    if (dev == &wlan0) {
        return espif_link(dev);
    }
    return 0;
}

netdev_status_t netdev_get_status(uint32_t netdev_id) {
    struct netif *dev = get_netif_by_id(netdev_id);
    if (!dev || !netif_is_link_up(dev)) {
        return NETDEV_NETIF_DOWN;
    }
    return netif_link(dev) ? NETDEV_NETIF_UP : NETDEV_UNLINKED;
}

netdev_ip_obtained_t netdev_get_ip_obtained_type(uint32_t netdev_id) {
    if (netdev_id < NETDEV_COUNT) {
        return IS_LAN_STATIC(wui_netdev_config[netdev_id].lan.flag) ? NETDEV_STATIC : NETDEV_DHCP;
    } else {
        return NETDEV_DHCP;
    }
}

uint32_t netdev_set_dhcp(uint32_t netdev_id) {
    ETH_config_t *pConfig = NULL;
    err_t res = ERR_OK;

    struct netif *dev = get_netif_by_id(netdev_id);
    if (dev) {
        res = netifapi_dhcp_start(dev);
        pConfig = &wui_netdev_config[netdev_id];
    }
    if (pConfig != NULL) {
        CHANGE_FLAG_TO_DHCP(pConfig->lan.flag);
        pConfig->var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        save_net_params(pConfig, netdev_id == NETDEV_ESP_ID ? &ap : NULL, netdev_id);
        pConfig->var_mask = 0;
        return res;
    } else {
        res = ERR_IF;
    }

    return res;
}

uint32_t netdev_set_up(uint32_t netdev_id) {
    struct netif *dev = get_netif_by_id(netdev_id);
    if (!dev) {
        return ERR_IF;
    }
    netifapi_netif_set_default(dev);
    netifapi_netif_set_link_up(dev);
    return netifapi_netif_set_up(dev);
}

bool netdev_load_ini_to_eeprom() {
    if ((load_ini_file_eth(&wui_netdev_config[NETDEV_ETH_ID]) != 1) || (load_ini_file_wifi(&wui_netdev_config[NETDEV_ESP_ID], &ap) != 1)) {
        return false;
    }

    // Yes, indeed, the load functions return 1 on success, these save return 0 on success...
    if ((save_net_params(&wui_netdev_config[NETDEV_ETH_ID], NULL, NETDEV_ETH_ID) != 0) || (save_net_params(&wui_netdev_config[NETDEV_ESP_ID], &ap, NETDEV_ESP_ID))) {
        return false;
    }

    return true;
}

uint32_t netdev_set_down(uint32_t netdev_id) {
    struct netif *dev = get_netif_by_id(netdev_id);
    if (!dev) {
        return ERR_IF;
    }
    netifapi_netif_set_link_down(dev);
    return netifapi_netif_set_down(dev);
}

uint32_t netdev_set_static(uint32_t netdev_id) {
    ETH_config_t *pConfig = NULL;
    err_t res = ERR_OK;

    struct netif *dev = get_netif_by_id(netdev_id);
    if (dev) {
        pConfig = &wui_netdev_config[netdev_id];
        netifapi_netif_set_up(dev);
        dns_setserver(DNS_1, &pConfig->dns1_ip4);
        dns_setserver(DNS_2, &pConfig->dns2_ip4);
        res = netifapi_netif_set_addr(dev, &pConfig->lan.addr_ip4, &pConfig->lan.msk_ip4, &pConfig->lan.gw_ip4);
        res = netifapi_dhcp_inform(dev);
    }

    if (pConfig != NULL) {
        CHANGE_FLAG_TO_STATIC(pConfig->lan.flag);
        pConfig->var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        save_net_params(pConfig, netdev_id == NETDEV_ESP_ID ? &ap : NULL, netdev_id);
        pConfig->var_mask = 0;
        return res;
    } else {
        res = ERR_IF;
    }

    return res;
}

uint32_t netdev_check_link(uint32_t dev_id) {
    // TODO: This needs some implementation or removal
    return 0;
}

static struct alsockets_s *netdev_get_sockets(uint32_t active_id) {
    // TODO: Drop alternative sockets
    return alsockets_eth();
}

const char *netdev_get_hostname(uint32_t active_id) {
    if (active_id < NETDEV_COUNT) {
        return wui_netdev_config[active_id].hostname;
    } else {
        return "";
    }
}

void netdev_join_ap() {
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
        return;
    }
    espif_join_ap(ap.ssid, passwd);
}
