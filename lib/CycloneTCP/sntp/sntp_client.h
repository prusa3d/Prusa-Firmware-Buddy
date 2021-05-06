/**
 * @file sntp_client.h
 * @brief SNTP client (Simple Network Time Protocol)
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _SNTP_CLIENT_H
#define _SNTP_CLIENT_H

//Dependencies
#include "core/net.h"
#include "sntp/ntp_common.h"

//SNTP client support
#ifndef SNTP_CLIENT_SUPPORT
   #define SNTP_CLIENT_SUPPORT ENABLED
#elif (SNTP_CLIENT_SUPPORT != ENABLED && SNTP_CLIENT_SUPPORT != DISABLED)
   #error SNTP_CLIENT_SUPPORT parameter is not valid
#endif

//Default timeout
#ifndef SNTP_CLIENT_DEFAULT_TIMEOUT
   #define SNTP_CLIENT_DEFAULT_TIMEOUT 30000
#elif (SNTP_CLIENT_DEFAULT_TIMEOUT < 1000)
   #error SNTP_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//Initial retransmission timeout
#ifndef SNTP_CLIENT_INIT_RETRANSMIT_TIMEOUT
   #define SNTP_CLIENT_INIT_RETRANSMIT_TIMEOUT 2000
#elif (SNTP_CLIENT_INIT_RETRANSMIT_TIMEOUT < 1000)
   #error SNTP_CLIENT_INIT_RETRANSMIT_TIMEOUT parameter is not valid
#endif

//Maximum retransmission timeout
#ifndef SNTP_CLIENT_MAX_RETRANSMIT_TIMEOUT
   #define SNTP_CLIENT_MAX_RETRANSMIT_TIMEOUT 15000
#elif (SNTP_CLIENT_MAX_RETRANSMIT_TIMEOUT < 1000)
   #error SNTP_CLIENT_MAX_RETRANSMIT_TIMEOUT parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief SNTP client states
 **/

typedef enum
{
   SNTP_CLIENT_STATE_INIT      = 0,
   SNTP_CLIENT_STATE_SENDING   = 2,
   SNTP_CLIENT_STATE_RECEIVING = 3,
   SNTP_CLIENT_STATE_COMPLETE  = 4
} SntpClientState;


/**
 * @brief SNTP client context
 **/

typedef struct
{
   SntpClientState state;             ///<SNTP client state
   NetInterface *interface;           ///<Underlying network interface
   IpAddr serverIpAddr;               ///<NTP server address
   uint16_t serverPort;               ///<NTP server port
   systime_t timeout;                 ///<Timeout value
   Socket *socket;                    ///<Underlying socket
   systime_t startTime;               ///<Request start time
   systime_t retransmitStartTime;     ///<Time at which the last request was sent
   systime_t retransmitTimeout;       ///<Retransmission timeout
   uint8_t message[NTP_MAX_MSG_SIZE]; ///<Buffer that holds the NTP request/response
   size_t messageLen;                 ///<Length of the NTP message, in bytes
   uint32_t kissCode;                 ///<Kiss code
} SntpClientContext;


//SNTP client related functions
error_t sntpClientInit(SntpClientContext *context);

error_t sntpClientSetTimeout(SntpClientContext *context, systime_t timeout);

error_t sntpClientBindToInterface(SntpClientContext *context,
   NetInterface *interface);

error_t sntpClientSetServerAddr(SntpClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort);

error_t sntpClientGetTimestamp(SntpClientContext *context,
   NtpTimestamp *timestamp);

uint32_t sntpClientGetKissCode(SntpClientContext *context);

void sntpClientDeinit(SntpClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
