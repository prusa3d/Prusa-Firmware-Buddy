#pragma once

#include "lwip/ip_addr.h"

#define LAN_FLAG_ONOFF_POS     (1 << 0) // position of ONOFF switch in lan.flag
#define LAN_FLAG_TYPE_POS      (1 << 1) // position of DHCP/STATIC switch in lan.flag
#define LAN_FLAG_INTERFACE_POS (1 << 2) // position of WIFI/ETH switch in lan.flag

#define IS_LAN_OFF(flg)    (flg & LAN_FLAG_ONOFF_POS)        // returns true if flag is set to OFF
#define IS_LAN_ON(flg)     ((flg & LAN_FLAG_ONOFF_POS) == 0) // returns true if flag is set to ON
#define IS_LAN_STATIC(flg) (flg & LAN_FLAG_TYPE_POS)         // returns true if flag is set to STATIC
#define IS_LAN_DHCP(flg)   ((flg & LAN_FLAG_TYPE_POS) == 0)  // returns true if flag is set to DHCP

#define CHANGE_FLAG_TO_STATIC(flg) (flg |= LAN_FLAG_TYPE_POS)   // flip lan type flg to STATIC
#define CHANGE_FLAG_TO_DHCP(flg)   (flg &= ~LAN_FLAG_TYPE_POS)  // flip lan type flg to DHCP
#define TURN_FLAG_ON(flg)          (flg &= ~LAN_FLAG_ONOFF_POS) // flip lan switch flg to ON
#define TURN_FLAG_OFF(flg)         (flg |= LAN_FLAG_ONOFF_POS)  // flip lan switch flg to OFF

#define CHANGE_LAN_TO_STATIC()  eth_change_setting(LAN_FLAG_TYPE_POS, 1)  // flip lan type flg to STATIC
#define CHANGE_LAN_TO_DHCP()    eth_change_setting(LAN_FLAG_TYPE_POS, 0)  // flip lan type flg to DHCP
#define TURN_LAN_ON()           eth_change_setting(LAN_FLAG_ONOFF_POS, 0) // flip lan switch flg to ON
#define TURN_LAN_OFF()          eth_change_setting(LAN_FLAG_ONOFF_POS, 1) // flip lan switch flg to OFF
#define LAN_INTERFACE_ETH(flg)  (flg |= LAN_FLAG_INTERFACE_POS)           // flip lan interface to ETH
#define LAN_INTERFACE_WIFI(flg) (flg &= ~LAN_FLAG_INTERFACE_POS)          // flip lan interface to WIFI

#define ETH_HOSTNAME_LEN 20 // ethernet hostname MAX length

typedef enum {
    ETH_UNLINKED,   // ETH cabel is unlinked
    ETH_NETIF_DOWN, // ETH interface is DOWN
    ETH_NETIF_UP,   // ETH interface is UP
} ETH_STATUS_t;

typedef enum {
    WUI_ETH_LINK_DOWN, // ETH cable is unplugged
    WUI_ETH_LINK_UP,   // ETH cable is plugged
} WUI_ETH_LINK_STATUS_t;

typedef enum {
    WUI_ETH_NETIF_DOWN, // ETH interface is down
    WUI_ETH_NETIF_UP,   // ETH interface is up
} WUI_ETH_NETIF_STATUS_t;

typedef struct {
    uint8_t flag;        // lan flags: pos0 = switch(ON=0, OFF=1), pos1 = type(DHCP=0, STATIC=1)
                         // pos2 = type(ETH=1, WIFI=0)
    ip4_addr_t addr_ip4; // user defined static ip4 address
    ip4_addr_t msk_ip4;  // user defined ip4 netmask
    ip4_addr_t gw_ip4;   // user define ip4 default gateway
} lan_t;

typedef struct {
    char hostname[ETH_HOSTNAME_LEN + 1]; // ETH hostname: MAX 20 chars
    int8_t timezone;                     // user defined difference to UTC
    ip_addr_t dns1_ip4;                  // user defined DNS #1
    ip_addr_t dns2_ip4;                  // user defined DNS #2
    lan_t lan;                           // user defined LAN configurations
    uint32_t var_mask;                   // mask for setting ethvars
} ETH_config_t;

#ifdef __cplusplus
extern "C" {
#endif

void eth_change_setting(uint16_t flag, uint16_t value);

#ifdef __cplusplus
}
#endif
