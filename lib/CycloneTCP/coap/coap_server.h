/**
 * @file coap_server.h
 * @brief CoAP server
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

#ifndef _COAP_SERVER_H
#define _COAP_SERVER_H

//Dependencies
#include "core/net.h"
#include "coap/coap_common.h"
#include "coap/coap_message.h"
#include "coap/coap_option.h"

//CoAP server support
#ifndef COAP_SERVER_SUPPORT
   #define COAP_SERVER_SUPPORT ENABLED
#elif (COAP_SERVER_SUPPORT != ENABLED && COAP_SERVER_SUPPORT != DISABLED)
   #error COAP_SERVER_SUPPORT parameter is not valid
#endif

//DTLS-secured CoAP support
#ifndef COAP_SERVER_DTLS_SUPPORT
   #define COAP_SERVER_DTLS_SUPPORT DISABLED
#elif (COAP_SERVER_DTLS_SUPPORT != ENABLED && COAP_SERVER_DTLS_SUPPORT != DISABLED)
   #error COAP_SERVER_DTLS_SUPPORT parameter is not valid
#endif

//Stack size required to run the CoAP server
#ifndef COAP_SERVER_STACK_SIZE
   #define COAP_SERVER_STACK_SIZE 650
#elif (COAP_SERVER_STACK_SIZE < 1)
   #error COAP_SERVER_STACK_SIZE parameter is not valid
#endif

//Maximum number of simultaneous DTLS sessions
#ifndef COAP_SERVER_MAX_SESSIONS
   #define COAP_SERVER_MAX_SESSIONS 4
#elif (COAP_SERVER_MAX_SESSIONS < 1)
   #error COAP_SERVER_MAX_SESSIONS parameter is not valid
#endif

//DTLS server tick interval
#ifndef COAP_SERVER_TICK_INTERVAL
   #define COAP_SERVER_TICK_INTERVAL 500
#elif (COAP_SERVER_TICK_INTERVAL < 100)
   #error COAP_SERVER_TICK_INTERVAL parameter is not valid
#endif

//DTLS session timeout
#ifndef COAP_SERVER_SESSION_TIMEOUT
   #define COAP_SERVER_SESSION_TIMEOUT 60000
#elif (COAP_SERVER_SESSION_TIMEOUT < 0)
   #error COAP_SERVER_SESSION_TIMEOUT parameter is not valid
#endif

//Size of buffer used for input/output operations
#ifndef COAP_SERVER_BUFFER_SIZE
   #define COAP_SERVER_BUFFER_SIZE 2048
#elif (COAP_SERVER_BUFFER_SIZE < 1)
   #error COAP_SERVER_BUFFER_SIZE parameter is not valid
#endif

//Maximum size of the cookie secret
#ifndef COAP_SERVER_MAX_COOKIE_SECRET_SIZE
   #define COAP_SERVER_MAX_COOKIE_SECRET_SIZE 32
#elif (COAP_SERVER_MAX_COOKIE_SECRET_SIZE < 1)
   #error COAP_SERVER_MAX_COOKIE_SECRET_SIZE parameter is not valid
#endif

//Maximum length of URI
#ifndef COAP_SERVER_MAX_URI_LEN
   #define COAP_SERVER_MAX_URI_LEN 128
#elif (COAP_SERVER_MAX_URI_LEN < 1)
   #error COAP_SERVER_MAX_URI_LEN parameter is not valid
#endif

//Priority at which the CoAP server should run
#ifndef COAP_SERVER_PRIORITY
   #define COAP_SERVER_PRIORITY OS_TASK_PRIORITY_NORMAL
#endif

//DTLS supported?
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//Forward declaration of CoapServerContext structure
struct _CoapServerContext;
#define CoapServerContext struct _CoapServerContext

//Forward declaration of CoapDtlsSession structure
struct _CoapDtlsSession;
#define CoapDtlsSession struct _CoapDtlsSession

//C++ guard
#ifdef __cplusplus
   extern "C" {
#endif


//DTLS supported?
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)

/**
 * @brief DTLS initialization callback
 **/

typedef error_t (*CoapServerDtlsInitCallback)(CoapServerContext *context,
   TlsContext *dtlsContext);

#endif


/**
 * @brief CoAP request callback function
 **/

typedef error_t (*CoapServerRequestCallback)(CoapServerContext *context,
   CoapCode method, const char_t *uri);


/**
 * @brief CoAP server settings
 **/

typedef struct
{
   NetInterface *interface;                     ///<Underlying network interface
   uint16_t port;                               ///<CoAP port number
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
   CoapServerDtlsInitCallback dtlsInitCallback; ///<DTLS initialization callback
#endif
   CoapServerRequestCallback requestCallback;   ///<CoAP request callback
} CoapServerSettings;


/**
 * @brief DTLS session
 **/

struct _CoapDtlsSession
{
   CoapServerContext *context;
   IpAddr serverIpAddr;
   IpAddr clientIpAddr;
   uint16_t clientPort;
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
   TlsContext *dtlsContext;
#endif
   systime_t timestamp;
};


/**
 * @brief CoAP server context
 **/

struct _CoapServerContext
{
   CoapServerSettings settings;                              ///<User settings
   Socket *socket;                                           ///<Underlying socket
   IpAddr serverIpAddr;                                      ///<Server's IP address
   IpAddr clientIpAddr;                                      ///<Client's IP address
   uint16_t clientPort;                                      ///<Client's port
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
   uint8_t cookieSecret[COAP_SERVER_MAX_COOKIE_SECRET_SIZE]; ///<Cookie secret
   size_t cookieSecretLen;                                   ///<Length of the cookie secret, in bytes
   CoapDtlsSession session[COAP_SERVER_MAX_SESSIONS];        ///<DTLS sessions
#endif
   uint8_t buffer[COAP_SERVER_BUFFER_SIZE];                  ///<Memory buffer for input/output operations
   size_t bufferLen;                                         ///<Length of the buffer, in bytes
   char_t uri[COAP_SERVER_MAX_URI_LEN + 1];                  ///<Resource identifier
   CoapMessage request;                                      ///<CoAP request message
   CoapMessage response;                                     ///<CoAP response message
};


//CoAP server related functions
void coapServerGetDefaultSettings(CoapServerSettings *settings);

error_t coapServerInit(CoapServerContext *context,
   const CoapServerSettings *settings);

error_t coapServerSetCookieSecret(CoapServerContext *context,
   const uint8_t *cookieSecret, size_t cookieSecretLen);

error_t coapServerStart(CoapServerContext *context);

void coapServerTask(CoapServerContext *context);

void coapServerDeinit(CoapServerContext *context);

//C++ guard
#ifdef __cplusplus
   }
#endif

#endif
