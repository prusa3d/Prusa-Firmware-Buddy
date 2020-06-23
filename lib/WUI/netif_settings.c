#include "lwip/netifapi.h"
#include "lwip/dhcp.h"
#include "wui_api.h"
#include "lwip.h"

static ETH_STATUS_t eth_status = ETH_NETIF_DOWN;
static bool dhcp_supplied = false;
struct netif eth0; // network interface for ETH
char eth_hostname[ETH_HOSTNAME_LEN + 1] = { 0 };

const ETH_STATUS_t get_eth_status(void) {
    return eth_status;
}

bool get_dhcp_supplied(void) {
    return dhcp_supplied;
}

void get_addrs_from_dhcp(ETH_config_t *config) {

    if (IS_LAN_ON(config->lan.flag)) {
        if (dhcp_supplied_address(&eth0)) {
            config->lan.addr_ip4.addr = netif_ip4_addr(&eth0)->addr;
            config->lan.msk_ip4.addr = netif_ip4_netmask(&eth0)->addr;
            config->lan.gw_ip4.addr = netif_ip4_gw(&eth0)->addr;
            return;
        }
    }
    config->lan.addr_ip4.addr = 0;
    config->lan.msk_ip4.addr = 0;
    config->lan.gw_ip4.addr = 0;
}

void eth_status_step(ETH_config_t *config, uint32_t eth_link) {
    ETH_STATUS_t old_status = eth_status;
    if (eth_link) {
        if (old_status == ETH_UNLINKED) {
            netifapi_netif_set_link_up(&eth0);
        }
        if (IS_LAN_STATIC(config->lan.flag)) {
            if (netif_is_up(&eth0)) {
                eth_status = ETH_NETIF_UP;
                dhcp_supplied = false;
            } else {
                eth_status = ETH_NETIF_DOWN;
                dhcp_supplied = false;
            }
        } else {
            if (dhcp_supplied_address(&eth0)) {
                dhcp_supplied = true;
                eth_status = ETH_NETIF_UP;
            } else {
                eth_status = ETH_NETIF_DOWN;
                dhcp_supplied = false;
            }
        }
    } else {
        eth_status = ETH_UNLINKED;
        dhcp_supplied = false;
        if (old_status != ETH_UNLINKED) {
            netifapi_netif_set_link_down(&eth0);
        }
    }
}

void turn_off_LAN(ETH_config_t *config) {
    TURN_LAN_OFF(config->lan.flag);
    save_eth_params(config);
    if (eth_status == ETH_NETIF_UP) {
        netifapi_netif_set_link_down(&eth0);
    }
}

void turn_on_LAN(ETH_config_t *config) {
    TURN_LAN_ON(config->lan.flag);
    save_eth_params(config);
    if (eth_status != ETH_UNLINKED) {
        netifapi_netif_set_link_up(&eth0);
    }
}

void set_LAN_to_static(ETH_config_t *config) {
    if (eth_status == ETH_NETIF_UP) {
        netifapi_netif_set_link_down(&eth0);
    }
    CHANGE_LAN_TO_STATIC(config->lan.flag);
    config->var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    save_eth_params(config);
    ipaddr.addr = config->lan.addr_ip4.addr;
    netmask.addr = config->lan.msk_ip4.addr;
    gw.addr = config->lan.gw_ip4.addr;
    netifapi_netif_set_addr(&eth0,
        (const ip4_addr_t *)&ipaddr,
        (const ip4_addr_t *)&netmask,
        (const ip4_addr_t *)&gw);
    if (eth_status != ETH_UNLINKED && IS_LAN_ON(config->lan.flag)) {
        netifapi_netif_set_link_up(&eth0);
    }
}

void set_LAN_to_dhcp(ETH_config_t *config) {
    if (eth_status == ETH_NETIF_UP) {
        netifapi_netif_set_link_down(&eth0);
    }
    CHANGE_LAN_TO_DHCP(config->lan.flag);
    config->var_mask = ETHVAR_MSK(ETHVAR_LAN_FLAGS);
    save_eth_params(config);
    if (eth_status != ETH_UNLINKED && IS_LAN_ON(config->lan.flag)) {
        netifapi_netif_set_link_up(&eth0);
    }
}
