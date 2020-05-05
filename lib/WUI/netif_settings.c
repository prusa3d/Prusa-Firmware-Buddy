#include "netifapi.h"
#include "dhcp.h"
#include "wui_api.h"
#include "lwip.h"

struct netif eth0;          // network interface for ETH
char eth_hostname[ETH_HOSTNAME_LEN + 1] = { 0 };
void get_addrs_from_dhcp(ETH_config_t * config){
    
    if (IS_LAN_ON(config->lan.flag)) {
        if(dhcp_supplied_address(&eth0)){
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

ETH_STATUS_t eth_status(ETH_config_t *config){
    ETH_STATUS_t ret;
    if (netif_is_link_up(&eth0)) {
        if (IS_LAN_STATIC(config->lan.flag)) {
            if (netif_is_up(&eth0)) {
                ret = ETH_NETIF_UP;
            } else {
                ret = ETH_NETIF_DOWN;
            }
        } else {
            if (dhcp_supplied_address(&eth0)) {
                ret = ETH_NETIF_UP;
            } else {
                ret = ETH_NETIF_DOWN;
            }
        }
    } else {
        ret = ETH_UNLINKED;
    }
    return ret;
}

void turn_off_LAN(ETH_config_t *config){
    if (netif_is_up(&eth0)) {
        netifapi_netif_set_down(&eth0);
    }
    TURN_LAN_OFF(config->lan.flag);
}

void turn_on_LAN(ETH_config_t *config){
    TURN_LAN_ON(config->lan.flag);
    if (netif_is_link_up(&eth0)) {
        netifapi_netif_set_up(&eth0);
    }
}

void set_LAN_to_static(ETH_config_t *config) {
    if (netif_is_up(&eth0)) {
        netifapi_netif_set_down(&eth0);
    }
    CHANGE_LAN_TO_STATIC(config->lan.flag);
    ipaddr.addr = config->lan.addr_ip4.addr;
    netmask.addr = config->lan.msk_ip4.addr;
    gw.addr = config->lan.gw_ip4.addr;
    netifapi_netif_set_addr(&eth0,
        (const ip4_addr_t *)&ipaddr,
        (const ip4_addr_t *)&netmask,
        (const ip4_addr_t *)&gw
        );
    if (netif_is_link_up(&eth0) && IS_LAN_ON(config->lan.flag)) {
        netifapi_netif_set_up(&eth0);
    }
}

void set_LAN_to_dhcp(ETH_config_t *config) {
    if (netif_is_up(&eth0)) {        
        netifapi_netif_set_down(&eth0);
    }
    CHANGE_LAN_TO_DHCP(config->lan.flag);
    if (netif_is_link_up(&eth0) && IS_LAN_ON(config->lan.flag)) {
        netifapi_netif_set_up(&eth0);
    }
}

uint8_t dhcp_addrs_are_supplied(void){
    return dhcp_supplied_address(&eth0);
}