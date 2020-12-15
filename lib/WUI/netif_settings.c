#include "netifapi.h"
#include "netif.h"
#include "dhcp.h"
#include "dns.h"
#include "wui_api.h"
#include "lwip.h"

static uint32_t wui_eth_config_update = 0;

const ETH_STATUS_t get_eth_status(void) {
    if (WUI_ETH_NETIF_UP == netif_status && link_status == WUI_ETH_LINK_UP) {
        return ETH_NETIF_UP;
    } else if (WUI_ETH_LINK_UP == link_status && WUI_ETH_NETIF_DOWN == netif_status) {
        return ETH_NETIF_DOWN;
    }
    return ETH_UNLINKED;
}

uint8_t get_lan_flag(void) {
    return wui_eth_config.lan.flag;
}

void get_eth_address(ETH_config_t *config) {
    config->lan.addr_ip4.addr = netif_ip4_addr(&eth0)->addr;
    config->lan.msk_ip4.addr = netif_ip4_netmask(&eth0)->addr;
    config->lan.gw_ip4.addr = netif_ip4_gw(&eth0)->addr;
}

void set_eth_update_mask(uint32_t var_mask) {
    wui_eth_config_update |= var_mask;
}

void clear_eth_update_mask() {
    wui_eth_config_update = 0;
}

uint32_t get_eth_update_mask() {
    return wui_eth_config_update;
}
