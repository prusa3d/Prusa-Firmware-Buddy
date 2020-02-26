/**
 ******************************************************************************
 * File Name          : LWIP.h
 * Description        : This file provides code for the configuration
 *                      of the LWIP.
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
 *************************************************************************

 */

/*
 * file changes note
 * 		20200122: Joshy
 * 			Added Interface functions for LwIP customization
 * 			code formatted
 *
 * */

#ifndef _MINI_LWIP_H
#define _MINI_LWIP_H
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "ethernetif.h"
/* Includes for RTOS ---------------------------------------------------------*/
#if WITH_RTOS
    #include "lwip/tcpip.h"
#endif /* WITH_RTOS */
/* chip specific includes ----------------------------------------------------*/
#include "stm32f4xx_hal.h"
/* lwip customization includes -----------------------------------------------*/

#include "lwip/apps/httpd.h"
#include "eeprom.h"
/* Global Variables ---------------------------------------------------------*/
extern struct netif eth0;
extern char interface_hostname[LAN_HOSTNAME_MAX_LEN + 1];
extern ETH_HandleTypeDef heth;

/*!****************************************************************************
* \brief Main LwIP stack initialization function
*
* \param    void
*
* \return	void
*
*****************************************************************************/
void MX_LWIP_Init(void);

/* MINI LwIP interface functions --------------------------------------------*/

/*!****************************************************************************
* \brief Interface function for initializing HTTP server.
*
* \param    void
*
* \return	void
*
*****************************************************************************/
void http_server_init(void);

#ifdef __cplusplus
}
#endif
#endif /*_MINI_LWIP_H */
