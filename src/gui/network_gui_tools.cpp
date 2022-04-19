#include "network_gui_tools.hpp"
#include "wui_api.h"

void stringify_address_for_screen(char *dest, size_t dest_len, const lan_t config, uint32_t eth_var) {
    if (eth_var & ETHVAR_STATIC_LAN_ADDRS) {
        if (eth_var == ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4)) {
            ip4addr_ntoa_r(&config.addr_ip4, dest, dest_len);
        } else if (eth_var == ETHVAR_MSK(ETHVAR_LAN_MSK_IP4)) {
            ip4addr_ntoa_r(&config.msk_ip4, dest, dest_len);
        } else if (eth_var == ETHVAR_MSK(ETHVAR_LAN_GW_IP4)) {
            ip4addr_ntoa_r(&config.gw_ip4, dest, dest_len);
        }
    }
}
