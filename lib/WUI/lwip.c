/**
 ******************************************************************************
 * File Name          : LWIP.c
 * Description        : This file provides initialization code for LWIP
 *                      middleWare.
 ******************************************************************************
 * This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * Copyright (c) 2019 STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/*
 * file changes note
 *      20200122: Joshy
 *          Added Interface functions for LwIP customization
 *          code formatted
 *
 * */

#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "dns.h"
#include "netifapi.h"

#include <string.h>
#include "dbg.h"
#include "ethernetif.h"
#include "wui_api.h"
// global variables
WUI_ETH_LINK_STATUS_t link_status = WUI_ETH_LINK_DOWN;
WUI_ETH_NETIF_STATUS_t netif_status = WUI_ETH_NETIF_DOWN;
struct netif eth0;           // network interface structure for ETH
ETH_config_t wui_eth_config; // the active WUI configuration for ethernet, connect and server
// local static variables
static ip4_addr_t ipaddr;
static ip4_addr_t netmask;
static ip4_addr_t gw;
static uint32_t ip4_type = WUI_IP4_DHCP;
static char eth_hostname[ETH_HOSTNAME_LEN + 1] = { 0 };

static void wui_lwip_set_lan_static(ETH_config_t *ethconfig) {
    ip4_type = WUI_IP4_DHCP;
    ipaddr.addr = ethconfig->lan.addr_ip4.addr;
    netmask.addr = ethconfig->lan.msk_ip4.addr;
    gw.addr = ethconfig->lan.gw_ip4.addr;
    dns_setserver(DNS_1, &ethconfig->dns1_ip4);
    dns_setserver(DNS_2, &ethconfig->dns2_ip4);
    netifapi_netif_set_addr(&eth0, &ipaddr, &netmask, &gw);
    netifapi_dhcp_inform(&eth0);
}

static void wui_lwip_set_lan_dhcp() {
    ip4_type = WUI_IP4_DHCP;
    netifapi_dhcp_start(&eth0);
}

void netif_link_callback(struct netif *eth) {
    ethernetif_update_config(eth);
    if (netif_is_link_up(eth)) {
        if (!netif_is_up(eth)) {
            netif_set_up(eth);
        }
    } else {
        if (netif_is_up(eth)) {
            netif_set_down(eth);
        }
    }
}

void netif_status_callback(struct netif *eth) {
    netif_status = netif_is_up(eth) ? WUI_ETH_NETIF_UP : WUI_ETH_NETIF_DOWN;
}

void wui_lwip_link_status() {
    if (ethernetif_link(&eth0)) {
        link_status = WUI_ETH_LINK_UP;
        load_eth_params(&wui_eth_config);
        if (!netif_is_link_up(&eth0) && IS_LAN_ON(wui_eth_config.lan.flag)) {
            netifapi_netif_set_link_up(&eth0);
            // start the DHCP if needed!
            if (!IS_LAN_STATIC(wui_eth_config.lan.flag)) {
                dhcp_renew(&eth0);
            }
        }
    } else {
        link_status = WUI_ETH_LINK_DOWN;
        if (netif_is_link_up(&eth0)) {
            netifapi_netif_set_link_down(&eth0);
        }
    }
}

void wui_lwip_sync_gui_lan_settings() {
    uint32_t var_mask = get_eth_update_mask();
    if (var_mask) {
        wui_eth_config.var_mask = var_mask;
        load_eth_params(&wui_eth_config);
        clear_eth_update_mask();
        // LAN related changes
        if (wui_eth_config.var_mask & ETHVAR_MSK(ETHVAR_LAN_FLAGS)) {
            //// LAN ON/OFF
            if (IS_LAN_ON(wui_eth_config.lan.flag)) {
                if (!netif_is_up(&eth0)) {
                    netif_set_up(&eth0);
                    if (!IS_LAN_STATIC(wui_eth_config.lan.flag)) {
                        dhcp_start(&eth0);
                    }
                }
            } else {
                if (netif_is_up(&eth0)) {
                    netif_set_down(&eth0);
                }
            }
            //// DHCP/static ?
            if (IS_LAN_STATIC(wui_eth_config.lan.flag)) {
                if (WUI_IP4_DHCP == ip4_type)
                    wui_lwip_set_lan_static(&wui_eth_config);
            } else {
                if (WUI_IP4_STATIC == ip4_type)
                    wui_lwip_set_lan_dhcp();
            }
        }
    }
}

void MX_LWIP_Init(ETH_config_t *ethconfig) {
    /* Initilialize the LwIP stack with RTOS */
    tcpip_init(NULL, NULL);

    if (IS_LAN_STATIC(ethconfig->lan.flag)) {
        ip4_type = WUI_IP4_STATIC;
        ipaddr.addr = ethconfig->lan.addr_ip4.addr;
        netmask.addr = ethconfig->lan.msk_ip4.addr;
        gw.addr = ethconfig->lan.gw_ip4.addr;

        dns_setserver(DNS_1, &ethconfig->dns1_ip4);
        dns_setserver(DNS_2, &ethconfig->dns2_ip4);

        netif_set_addr(&eth0, &ipaddr, &netmask, &gw);
    } else {
        ip4_type = WUI_IP4_DHCP;
        ipaddr.addr = 0;
        netmask.addr = 0;
        gw.addr = 0;
    }
    /* add the network interface (IPv4/IPv6) with RTOS */
    netif_add(&eth0, &ipaddr, &netmask, &gw, NULL, &ethernetif_init,
        &tcpip_input);
    /* Registers the default network interface */
    netif_set_default(&eth0);
    // set the host name
    strlcpy(eth_hostname, ethconfig->hostname, ETH_HOSTNAME_LEN + 1);
    eth0.hostname = eth_hostname;
    /* Setting necessary callbacks after initial setup */
    netif_set_link_callback(&eth0, netif_link_callback);
    netif_set_status_callback(&eth0, netif_status_callback);

    if (netif_is_link_up(&eth0) && IS_LAN_ON(ethconfig->lan.flag)) {
        /* When the netif is fully configured this function must be called */
        netif_set_up(&eth0);
        // start the DHCP if needed!
        if (!IS_LAN_STATIC(ethconfig->lan.flag)) {
            dhcp_start(&eth0);
        }
    } else {
        /* When the netif link is down this function must be called */
        netif_set_down(&eth0);
    }
}
