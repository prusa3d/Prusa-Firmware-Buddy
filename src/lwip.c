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

/* Includes ------------------------------------------------------------------*/
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#if defined(__CC_ARM) /* MDK ARM Compiler */
    #include "lwip/sio.h"
#endif /* MDK ARM Compiler */

/* USER CODE BEGIN 0 */

#include "eeprom.h"
#include "dbg.h"

/* USER CODE END 0 */
/* Private function prototypes -----------------------------------------------*/
/* ETH Variables initialization ----------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Variables Initialization */
struct netif eth0;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;

/* USER CODE BEGIN 2 */

void netif_link_callback(struct netif *eth) {
    ethernetif_update_config(eth);

    if (netif_is_link_up(eth)) {
        if(eeprom_get_var(EEVAR_LAN_TYPE).ui8 == 1){
            //if(eeprom_get_var(EEVAR_LAN_IP4_ADDR).ui32 == 0)
            ip4_addr_t ip4_addr, ip4_msk, ip4_gw; //, ip4_dns1, ip4_dns2;
            ip4_addr.addr = eeprom_get_var(EEVAR_LAN_IP4_ADDR).ui32;
            ip4_msk.addr = eeprom_get_var(EEVAR_LAN_IP4_MSK).ui32;
            ip4_gw.addr = eeprom_get_var(EEVAR_LAN_IP4_GW).ui32;
            //ip4_dns1.addr = eeprom_get_var(EEVAR_LAN_IP4_DNS1).ui32;
            //ip4_dns2.addr = eeprom_get_var(EEVAR_LAN_IP4_DNS2).ui32;

            netif_set_addr(eth,
                (const ip4_addr_t *)&ip4_addr,
                (const ip4_addr_t *)&ip4_msk,
                (const ip4_addr_t *)&ip4_gw);

            //dns_setserver(0, (const ip4_addr_t *)&ip4_dns1);
            //dns_setserver(1, (const ip4_addr_t *)&ip4_dns2);

            netif_set_up(eth);
            dhcp_inform(eth);   //inform dhcp server about fixed ip addr config

        } else {
            netif_set_up(eth);
            dhcp_start(eth);
        }
    } else {
        netif_set_down(eth);
    }
}

/* USER CODE END 2 */

/**
  * LwIP initialization function
  */
void MX_LWIP_Init(void) {
    /* Initilialize the LwIP stack with RTOS */
    tcpip_init(NULL, NULL);

    /* IP addresses initialization with DHCP (IPv4) */
    ipaddr.addr = 0;
    netmask.addr = 0;
    gw.addr = 0;

    /* add the network interface (IPv4/IPv6) with RTOS */
    netif_add(&eth0, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

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
    /* USER CODE END 3 */
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

#if defined(__CC_ARM) /* MDK ARM Compiler */
/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum) {
    sio_fd_t sd;

    /* USER CODE BEGIN 7 */
    sd = 0; // dummy code
    /* USER CODE END 7 */

    return sd;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd) {
    /* USER CODE BEGIN 8 */
    /* USER CODE END 8 */
}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len) {
    u32_t recved_bytes;

    /* USER CODE BEGIN 9 */
    recved_bytes = 0; // dummy code
    /* USER CODE END 9 */
    return recved_bytes;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len) {
    u32_t recved_bytes;

    /* USER CODE BEGIN 10 */
    recved_bytes = 0; // dummy code
    /* USER CODE END 10 */
    return recved_bytes;
}
#endif /* MDK ARM Compiler */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
