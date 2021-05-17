/**
 * @file coap_client.h
 * @brief CoAP client
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

#ifndef _COAP_CLIENT_H
#define _COAP_CLIENT_H

//Dependencies
#include "core/net.h"
#include "coap/coap_common.h"
#include "coap/coap_message.h"
#include "coap/coap_option.h"

//CoAP client support
#ifndef COAP_CLIENT_SUPPORT
   #define COAP_CLIENT_SUPPORT ENABLED
#elif (COAP_CLIENT_SUPPORT != ENABLED && COAP_CLIENT_SUPPORT != DISABLED)
   #error COAP_CLIENT_SUPPORT parameter is not valid
#endif

//DTLS-secured CoAP support
#ifndef COAP_CLIENT_DTLS_SUPPORT
   #define COAP_CLIENT_DTLS_SUPPORT DISABLED
#elif (COAP_CLIENT_DTLS_SUPPORT != ENABLED && COAP_CLIENT_DTLS_SUPPORT != DISABLED)
   #error COAP_CLIENT_DTLS_SUPPORT parameter is not valid
#endif

//CoAP observe support
#ifndef COAP_CLIENT_OBSERVE_SUPPORT
   #define COAP_CLIENT_OBSERVE_SUPPORT ENABLED
#elif (COAP_CLIENT_OBSERVE_SUPPORT != ENABLED && COAP_CLIENT_OBSERVE_SUPPORT != DISABLED)
   #error COAP_CLIENT_OBSERVE_SUPPORT parameter is not valid
#endif

//CoAP block-wise transfer support
#ifndef COAP_CLIENT_BLOCK_SUPPORT
   #define COAP_CLIENT_BLOCK_SUPPORT ENABLED
#elif (COAP_CLIENT_BLOCK_SUPPORT != ENABLED && COAP_CLIENT_BLOCK_SUPPORT != DISABLED)
   #error COAP_CLIENT_BLOCK_SUPPORT parameter is not valid
#endif

//CoAP client tick interval
#ifndef COAP_CLIENT_TICK_INTERVAL
   #define COAP_CLIENT_TICK_INTERVAL 100
#elif (COAP_CLIENT_TICK_INTERVAL < 10)
   #error COAP_CLIENT_TICK_INTERVAL parameter is not valid
#endif

//Default timeout
#ifndef COAP_CLIENT_DEFAULT_TIMEOUT
   #define COAP_CLIENT_DEFAULT_TIMEOUT 20000
#elif (COAP_CLIENT_DEFAULT_TIMEOUT < 0)
   #error COAP_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//Maximum number of simultaneous outstanding requests
#ifndef COAP_CLIENT_NSTART
   #define COAP_CLIENT_NSTART 1
#elif (COAP_CLIENT_NSTART < 1)
   #error COAP_CLIENT_NSTART parameter is not valid
#endif

//Maximum number of retransmissions
#ifndef COAP_CLIENT_MAX_RETRANSMIT
   #define COAP_CLIENT_MAX_RETRANSMIT 4
#elif (COAP_CLIENT_MAX_RETRANSMIT < 1)
   #error COAP_CLIENT_MAX_RETRANSMIT parameter is not valid
#endif

//Initial retransmission timeout (minimum)
#ifndef COAP_CLIENT_ACK_TIMEOUT_MIN
   #define COAP_CLIENT_ACK_TIMEOUT_MIN 2000
#elif (COAP_CLIENT_ACK_TIMEOUT_MIN < 1000)
   #error COAP_CLIENT_ACK_TIMEOUT_MIN parameter is not valid
#endif

//Initial retransmission timeout (maximum)
#ifndef COAP_CLIENT_ACK_TIMEOUT_MAX
   #define COAP_CLIENT_ACK_TIMEOUT_MAX 3000
#elif (COAP_CLIENT_ACK_TIMEOUT_MAX < COAP_CLIENT_ACK_TIMEOUT_MIN)
   #error COAP_CLIENT_ACK_TIMEOUT_MAX parameter is not valid
#endif

//Random delay after Max-Age has expired (minimum)
#ifndef COAP_CLIENT_RAND_DELAY_MIN
   #define COAP_CLIENT_RAND_DELAY_MIN 5000
#elif (COAP_CLIENT_RAND_DELAY_MIN < 1000)
   #error COAP_CLIENT_RAND_DELAY_MIN parameter is not valid
#endif

//Random delay after Max-Age has expired (maximum)
#ifndef COAP_CLIENT_RAND_DELAY_MAX
   #define COAP_CLIENT_RAND_DELAY_MAX 15000
#elif (COAP_CLIENT_RAND_DELAY_MAX < COAP_CLIENT_RAND_DELAY_MIN)
   #error COAP_CLIENT_RAND_DELAY_MAX parameter is not valid
#endif

//Default token length
#ifndef COAP_CLIENT_DEFAULT_TOKEN_LEN
   #define COAP_CLIENT_DEFAULT_TOKEN_LEN 4
#elif (COAP_CLIENT_DEFAULT_TOKEN_LEN < 0 || COAP_CLIENT_DEFAULT_TOKEN_LEN > 8)
   #error COAP_CLIENT_DEFAULT_TOKEN_LEN parameter is not valid
#endif

//DTLS supported?
#if (COAP_CLIENT_DTLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//Forward declaration of CoapClientContext structure
struct _CoapClientContext;
#define CoapClientContext struct _CoapClientContext

//Forward declaration of CoapClientRequest structure
struct _CoapClientRequest;
#define CoapClientRequest struct _CoapClientRequest

//Dependencies
#include "coap_client_request.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief CoAP client states
 */

typedef enum
{
   COAP_CLIENT_STATE_DISCONNECTED = 0,
   COAP_CLIENT_STATE_CONNECTING   = 1,
   COAP_CLIENT_STATE_CONNECTED    = 2
} CoapClientState;


//DTLS supported?
#if (COAP_CLIENT_DTLS_SUPPORT == ENABLED)

/**
 * @brief DTLS initialization callback
 **/

typedef error_t (*CoapClientDtlsInitCallback)(CoapClientContext *context,
   TlsContext *dtlsContext);

#endif


/**
 * @brief CoAP client context
 **/

struct _CoapClientContext
{
   OsMutex mutex;                                 ///<Mutex preventing simultaneous access to the context
   OsEvent event;                                 ///<Event object used to receive notifications
   CoapClientState state;                         ///<CoAP client state
   CoapTransportProtocol transportProtocol;       ///<Transport protocol (UDP or DTLS)
   NetInterface *interface;                       ///<Underlying network interface
   Socket *socket;                                ///<Underlying UDP socket
#if (COAP_CLIENT_DTLS_SUPPORT == ENABLED)
   TlsContext *dtlsContext;                       ///<DTLS context
   TlsSessionState dtlsSession;                   ///<DTLS session state
   CoapClientDtlsInitCallback dtlsInitCallback;   ///<DTLS initialization callback
#endif
   systime_t startTime;                           ///<Start time
   systime_t timeout;                             ///<Timeout value
   uint16_t mid;                                  ///<Message identifier
   size_t tokenLen;                               ///<Token length
   CoapClientRequest request[COAP_CLIENT_NSTART]; ///<Outstanding CoAP requests
   CoapMessage response;                          ///<CoAP response message
};


//CoAP client related functions
error_t coapClientInit(CoapClientContext *context);

error_t coapClientSetTransportProtocol(CoapClientContext *context,
   CoapTransportProtocol transportProtocol);

#if (COAP_CLIENT_DTLS_SUPPORT == ENABLED)

error_t coapClientRegisterDtlsInitCallback(CoapClientContext *context,
   CoapClientDtlsInitCallback callback);

#endif

error_t coapClientSetTimeout(CoapClientContext *context, systime_t timeout);
error_t coapClientSetTokenLength(CoapClientContext *context, size_t length);

error_t coapClientBindToInterface(CoapClientContext *context,
   NetInterface *interface);

error_t coapClientConnect(CoapClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort);

error_t coapClientTask(CoapClientContext *context, systime_t timeout);

error_t coapClientDisconnect(CoapClientContext *context);

void coapClientDeinit(CoapClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
