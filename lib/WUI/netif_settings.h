#pragma once

#include "lwip/ip_addr.h"

#define LAN_FLAG_ONOFF_POS (1 << 0) // position of ONOFF switch in lan.flag
#define LAN_FLAG_TYPE_POS  (1 << 1) // position of DHCP/STATIC switch in lan.flag

#define IS_LAN_OFF(flg)    (flg & LAN_FLAG_ONOFF_POS) // returns true if flag is set to OFF
#define IS_LAN_ON(flg)     ((flg & LAN_FLAG_ONOFF_POS) == 0) // returns true if flag is set to ON
#define IS_LAN_STATIC(flg) (flg & LAN_FLAG_TYPE_POS) // returns true if flag is set to STATIC
#define IS_LAN_DHCP(flg)   ((flg & LAN_FLAG_TYPE_POS) == 0) // returns true if flag is set to DHCP

#define CHANGE_FLAG_TO_STATIC(flg) (flg |= LAN_FLAG_TYPE_POS) // flip lan type flg to STATIC
#define CHANGE_FLAG_TO_DHCP(flg)   (flg &= ~LAN_FLAG_TYPE_POS) // flip lan type flg to DHCP
#define TURN_FLAG_ON(flg)          (flg &= ~LAN_FLAG_ONOFF_POS) // flip lan switch flg to ON
#define TURN_FLAG_OFF(flg)         (flg |= LAN_FLAG_ONOFF_POS) // flip lan switch flg to OFF

#define HOSTNAME_LEN            20 // ethernet hostname MAX length
#define SSID_MAX_LEN            32 // https://en.wikipedia.org/wiki/Service_set_(802.11_network)#SSID
#define WIFI_PSK_MAX            64
#define DNS_NTP_MAX_NAME_LENGTH 61

typedef struct {
    uint8_t flag; // lan flags: pos0 = switch(ON=0, OFF=1), pos1 = type(DHCP=0, STATIC=1)
                  // pos2 = type(ETH=1, WIFI=0)
    ip4_addr_t addr_ip4; // user defined static ip4 address
    ip4_addr_t msk_ip4; // user defined ip4 netmask
    ip4_addr_t gw_ip4; // user define ip4 default gateway
} lan_t;

typedef struct {
    char hostname[HOSTNAME_LEN + 1]; // ETH hostname: MAX 20 chars
    ip_addr_t dns1_ip4; // user defined DNS #1
    ip_addr_t dns2_ip4; // user defined DNS #2
    lan_t lan; // user defined LAN configurations
    uint32_t var_mask; // mask for setting ethvars
    char ntp[DNS_NTP_MAX_NAME_LENGTH]; // user defined NTP hostname
} netif_config_t;

// those bits were previously assigned to distinguish WPA/WEP/none
// this is no longer used, but don't use them since some long-ago-inited
// eeproms may contain them
#define RESERVED_MASK 0b1100

typedef struct {
    char ssid[SSID_MAX_LEN + 1];
    char pass[WIFI_PSK_MAX + 1];
} ap_entry_t;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
