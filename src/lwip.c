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

#include <string.h>

#include "dbg.h"
#include "ethernetif.h"

#define HOSTNAME_MAX_LEN 20
char interface_hostname[HOSTNAME_MAX_LEN + 1];
struct netif eth0;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;

void Error_Handler(void);

void netif_link_callback(struct netif *eth) {
    ethernetif_update_config(eth);
    uint8_t ee_flag = eeprom_get_var(EEVAR_LAN_FLAG).ui8;
    if (netif_is_link_up(eth)) {
        if (!(ee_flag & LAN_EEFLG_ONOFF)) {
            netif_set_up(eth);
        }
    } else {
        netif_set_down(eth);
    }
}

void netif_status_callback(struct netif *eth) {
    uint8_t ee_flag = eeprom_get_var(EEVAR_LAN_FLAG).ui8;
    if (netif_is_up(eth)) {
        if (!(ee_flag & LAN_EEFLG_TYPE)) {
            dhcp_start(eth);
        } else {
            dhcp_inform(eth);
        }
    } else {
        if (!(ee_flag & LAN_EEFLG_TYPE)) {
            dhcp_stop(eth);
        }
    }
}

void MX_LWIP_Init(void) {
    /* Initilialize the LwIP stack with RTOS */
    tcpip_init(NULL, NULL);

    /* IP addresses initialization with DHCP (IPv4) */
    ipaddr.addr = 0;
    netmask.addr = 0;
    gw.addr = 0;

    /* add the network interface (IPv4/IPv6) with RTOS */
    netif_add(&eth0, &ipaddr, &netmask, &gw, NULL, &ethernetif_init,
        &tcpip_input);

    /* Registers the default network interface */
    netif_set_default(&eth0);

    if (netif_is_link_up(&eth0)) {
        /* When the netif is fully configured this function must be called */
        netif_set_up(&eth0);
        /* Start DHCP negotiation for a network interface (IPv4) */
        dhcp_start(&eth0);
    } else {
        /* When the netif link is down this function must be called */
        netif_set_down(&eth0);
    }

    /* USER CODE BEGIN 3 */
    netif_set_link_callback(&eth0, netif_link_callback);
    netif_set_status_callback(&eth0, netif_status_callback);
    netif_set_down(&eth0);
    uint8_t ee_lan_flg = eeprom_get_var(EEVAR_LAN_FLAG).ui8;
    variant8_t hostname = eeprom_get_var(EEVAR_LAN_HOSTNAME);
    strcpy(interface_hostname, hostname.pch);
    variant8_done(&hostname);
    eth0.hostname = interface_hostname;
    if (ee_lan_flg & LAN_EEFLG_TYPE) {
        ipaddr.addr = eeprom_get_var(EEVAR_LAN_IP4_ADDR).ui32;
        netmask.addr = eeprom_get_var(EEVAR_LAN_IP4_MSK).ui32;
        gw.addr = eeprom_get_var(EEVAR_LAN_IP4_GW).ui32;
        netif_set_addr(&eth0, &ipaddr, &netmask, &gw);
    }
    if (!(ee_lan_flg & LAN_EEFLG_ONOFF)) {
        netif_set_up(&eth0);
    }
    /* USER CODE END 3 */
}

/* MINI LwIP interface functions --------------------------------------------*/

void http_server_init(void) {
    httpd_init();
}
