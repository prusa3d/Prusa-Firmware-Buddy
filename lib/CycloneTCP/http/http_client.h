/**
 * @file http_client.h
 * @brief HTTP client (HyperText Transfer Protocol)
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

#ifndef _HTTP_CLIENT_H
#define _HTTP_CLIENT_H

//Dependencies
#include "core/net.h"
#include "http/http_common.h"

//HTTP client support
#ifndef HTTP_CLIENT_SUPPORT
   #define HTTP_CLIENT_SUPPORT ENABLED
#elif (HTTP_CLIENT_SUPPORT != ENABLED && HTTP_CLIENT_SUPPORT != DISABLED)
   #error HTTP_CLIENT_SUPPORT parameter is not valid
#endif

//HTTP over TLS
#ifndef HTTP_CLIENT_TLS_SUPPORT
   #define HTTP_CLIENT_TLS_SUPPORT DISABLED
#elif (HTTP_CLIENT_TLS_SUPPORT != ENABLED && HTTP_CLIENT_TLS_SUPPORT != DISABLED)
   #error HTTP_CLIENT_TLS_SUPPORT parameter is not valid
#endif

//Basic access authentication support
#ifndef HTTP_CLIENT_BASIC_AUTH_SUPPORT
   #define HTTP_CLIENT_BASIC_AUTH_SUPPORT DISABLED
#elif (HTTP_CLIENT_BASIC_AUTH_SUPPORT != ENABLED && HTTP_CLIENT_BASIC_AUTH_SUPPORT != DISABLED)
   #error HTTP_CLIENT_BASIC_AUTH_SUPPORT parameter is not valid
#endif

//Digest access authentication support
#ifndef HTTP_CLIENT_DIGEST_AUTH_SUPPORT
   #define HTTP_CLIENT_DIGEST_AUTH_SUPPORT DISABLED
#elif (HTTP_CLIENT_DIGEST_AUTH_SUPPORT != ENABLED && HTTP_CLIENT_DIGEST_AUTH_SUPPORT != DISABLED)
   #error HTTP_CLIENT_DIGEST_AUTH_SUPPORT parameter is not valid
#endif

//MD5 digest support
#ifndef HTTP_CLIENT_MD5_SUPPORT
   #define HTTP_CLIENT_MD5_SUPPORT ENABLED
#elif (HTTP_CLIENT_MD5_SUPPORT != ENABLED && HTTP_CLIENT_MD5_SUPPORT != DISABLED)
   #error HTTP_CLIENT_MD5_SUPPORT parameter is not valid
#endif

//SHA-256 digest support
#ifndef HTTP_CLIENT_SHA256_SUPPORT
   #define HTTP_CLIENT_SHA256_SUPPORT DISABLED
#elif (HTTP_CLIENT_SHA256_SUPPORT != ENABLED && HTTP_CLIENT_SHA256_SUPPORT != DISABLED)
   #error HTTP_CLIENT_SHA256_SUPPORT parameter is not valid
#endif

//SHA-512/256 digest support
#ifndef HTTP_CLIENT_SHA512_256_SUPPORT
   #define HTTP_CLIENT_SHA512_256_SUPPORT DISABLED
#elif (HTTP_CLIENT_SHA512_256_SUPPORT != ENABLED && HTTP_CLIENT_SHA512_256_SUPPORT != DISABLED)
   #error HTTP_CLIENT_SHA512_256_SUPPORT parameter is not valid
#endif

//Default timeout
#ifndef HTTP_CLIENT_DEFAULT_TIMEOUT
   #define HTTP_CLIENT_DEFAULT_TIMEOUT 20000
#elif (HTTP_CLIENT_DEFAULT_TIMEOUT < 1000)
   #error HTTP_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//Size of the buffer for input/output operations
#ifndef HTTP_CLIENT_BUFFER_SIZE
   #define HTTP_CLIENT_BUFFER_SIZE 2048
#elif (HTTP_CLIENT_BUFFER_SIZE < 256)
   #error HTTP_CLIENT_BUFFER_SIZE parameter is not valid
#endif

//TX buffer size for TLS connections
#ifndef HTTP_CLIENT_TLS_TX_BUFFER_SIZE
   #define HTTP_CLIENT_TLS_TX_BUFFER_SIZE 2048
#elif (HTTP_CLIENT_TLS_TX_BUFFER_SIZE < 512)
   #error HTTP_CLIENT_TLS_TX_BUFFER_SIZE parameter is not valid
#endif

//RX buffer size for TLS connections
#ifndef HTTP_CLIENT_TLS_RX_BUFFER_SIZE
   #define HTTP_CLIENT_TLS_RX_BUFFER_SIZE 16384
#elif (HTTP_CLIENT_TLS_RX_BUFFER_SIZE < 512)
   #error HTTP_CLIENT_TLS_RX_BUFFER_SIZE parameter is not valid
#endif

//Maximum length of HTTP method
#ifndef HTTP_CLIENT_MAX_METHOD_LEN
   #define HTTP_CLIENT_MAX_METHOD_LEN 8
#elif (HTTP_CLIENT_MAX_METHOD_LEN < 1)
   #error HTTP_CLIENT_MAX_METHOD_LEN parameter is not valid
#endif

//Maximum length of the user name
#ifndef HTTP_CLIENT_MAX_USERNAME_LEN
   #define HTTP_CLIENT_MAX_USERNAME_LEN 32
#elif (HTTP_CLIENT_MAX_USERNAME_LEN < 0)
   #error HTTP_CLIENT_MAX_USERNAME_LEN parameter is not valid
#endif

//Maximum length of the password
#ifndef HTTP_CLIENT_MAX_PASSWORD_LEN
   #define HTTP_CLIENT_MAX_PASSWORD_LEN 32
#elif (HTTP_CLIENT_MAX_PASSWORD_LEN < 0)
   #error HTTP_CLIENT_MAX_PASSWORD_LEN parameter is not valid
#endif

//Maximum length of the realm
#ifndef HTTP_CLIENT_MAX_REALM_LEN
   #define HTTP_CLIENT_MAX_REALM_LEN 32
#elif (HTTP_CLIENT_MAX_REALM_LEN < 1)
   #error HTTP_CLIENT_MAX_REALM_LEN parameter is not valid
#endif

//Maximum length of the nonce
#ifndef HTTP_CLIENT_MAX_NONCE_LEN
   #define HTTP_CLIENT_MAX_NONCE_LEN 64
#elif (HTTP_CLIENT_MAX_NONCE_LEN < 1)
   #error HTTP_CLIENT_MAX_NONCE_LEN parameter is not valid
#endif

//Cnonce size
#ifndef HTTP_CLIENT_CNONCE_SIZE
   #define HTTP_CLIENT_CNONCE_SIZE 16
#elif (HTTP_CLIENT_CNONCE_SIZE < 1)
   #error HTTP_CLIENT_CNONCE_SIZE parameter is not valid
#endif

//Maximum length of the opaque parameter
#ifndef HTTP_CLIENT_MAX_OPAQUE_LEN
   #define HTTP_CLIENT_MAX_OPAQUE_LEN 64
#elif (HTTP_CLIENT_MAX_OPAQUE_LEN < 1)
   #error HTTP_CLIENT_MAX_OPAQUE_LEN parameter is not valid
#endif

//HTTP authentication supported?
#if (HTTP_CLIENT_BASIC_AUTH_SUPPORT == ENABLED)
   #define HTTP_CLIENT_AUTH_SUPPORT ENABLED
#elif (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   #define HTTP_CLIENT_AUTH_SUPPORT ENABLED
#else
   #define HTTP_CLIENT_AUTH_SUPPORT DISABLED
#endif

//TLS supported?
#if (HTTP_CLIENT_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//Basic authentication supported?
#if (HTTP_CLIENT_BASIC_AUTH_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "encoding/base64.h"
#endif

//MD5 digest authentication supported?
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED && HTTP_CLIENT_MD5_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "hash/md5.h"
#endif

//SHA-256 digest authentication supported?
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED && HTTP_CLIENT_SHA256_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "hash/sha256.h"
#endif

//SHA-512/256 digest authentication supported?
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED && HTTP_CLIENT_SHA512_256_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "hash/sha512_256.h"
#endif

//Forward declaration of HttpClientContext structure
struct _HttpClientContext;
#define HttpClientContext struct _HttpClientContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief HTTP client states
 */

typedef enum
{
   HTTP_CLIENT_STATE_DISCONNECTED  = 0,
   HTTP_CLIENT_STATE_CONNECTING    = 1,
   HTTP_CLIENT_STATE_CONNECTED     = 2,
   HTTP_CLIENT_STATE_DISCONNECTING = 3
} HttpClientState;


//TLS supported?
#if (HTTP_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief TLS initialization callback function
 **/

typedef error_t (*HttpClientTlsInitCallback)(HttpClientContext *context,
   TlsContext *tlsContext);

#endif


/**
 * @brief Random data generation callback function
 **/

typedef error_t (*HttpClientRandCallback)(uint8_t *data, size_t length);


/**
 * @brief HTTP authentication parameters
 **/

typedef struct
{
   HttpAuthMode mode;                                 ///<HTTP authentication mode
   char_t username[HTTP_CLIENT_MAX_USERNAME_LEN + 1]; ///<User name
   char_t password[HTTP_CLIENT_MAX_PASSWORD_LEN + 1]; ///<Password
   char_t realm[HTTP_CLIENT_MAX_REALM_LEN + 1];       ///<Realm
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   HttpAuthQop qop;                                   ///<Quality of protection
   const HashAlgo *algorithm;                         ///<Digest algorithm
   uint32_t nc;                                       ///<Nonce count
   char_t nonce[HTTP_CLIENT_MAX_NONCE_LEN + 1];       ///<Nonce value
   char_t cnonce[HTTP_CLIENT_CNONCE_SIZE * 2 + 1];    ///<Cnonce value
   char_t opaque[HTTP_CLIENT_MAX_OPAQUE_LEN + 1];     ///<Opaque parameter
   bool_t stale;                                      ///<Stale flag
#endif
} HttpClientAuthParams;


/**
 * @brief HTTP client context
 **/

struct _HttpClientContext
{
   HttpClientState state;                         ///<HTTP client state
   HttpVersion version;                           ///<HTTP protocol version
   NetInterface *interface;                       ///<Underlying network interface
   systime_t timeout;                             ///<Timeout value
   systime_t timestamp;                           ///<Timestamp to manage timeout
   Socket *socket;                                ///<Underlying socket
#if (HTTP_CLIENT_TLS_SUPPORT == ENABLED)
   TlsContext *tlsContext;                        ///<TLS context
   TlsSessionState tlsSession;                    ///<TLS session state
   HttpClientTlsInitCallback tlsInitCallback;     ///<TLS initialization callback function
#endif
   IpAddr serverIpAddr;                           ///<IP address of the HTTP server
   uint16_t serverPort;                           ///<TCP port number
#if (HTTP_CLIENT_AUTH_SUPPORT == ENABLED)
   HttpClientAuthParams authParams;               ///<HTTP authentication parameters
#endif
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   HttpClientRandCallback randCallback;           ///<Random data generation callback function
#endif
   HttpRequestState requestState;                 ///<HTTP request state
   char_t method[HTTP_CLIENT_MAX_METHOD_LEN + 1]; ///<HTTP request method
   bool_t keepAlive;                              ///<HTTP persistent connection
   bool_t chunkedEncoding;                        ///<Chunked transfer encoding
   char_t buffer[HTTP_CLIENT_BUFFER_SIZE + 1];    ///<Memory buffer for input/output operations
   size_t bufferLen;                              ///<Length of the buffer, in bytes
   size_t bufferPos;                              ///<Current position in the buffer
   size_t bodyLen;                                ///<Length of the body, in bytes
   size_t bodyPos;                                ///<Current position in the body
   uint_t statusCode;                             ///<HTTP status code
};


//HTTP related functions
error_t httpClientInit(HttpClientContext *context);

#if (HTTP_CLIENT_TLS_SUPPORT == ENABLED)

error_t httpClientRegisterTlsInitCallback(HttpClientContext *context,
   HttpClientTlsInitCallback callback);

#endif

error_t httpClientRegisterRandCallback(HttpClientContext *context,
   HttpClientRandCallback callback);

error_t httpClientSetVersion(HttpClientContext *context, HttpVersion version);
error_t httpClientSetTimeout(HttpClientContext *context, systime_t timeout);

error_t httpClientSetAuthInfo(HttpClientContext *context,
   const char_t *username, const char_t *password);

error_t httpClientBindToInterface(HttpClientContext *context,
   NetInterface *interface);

error_t httpClientConnect(HttpClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort);

error_t httpClientCreateRequest(HttpClientContext *context);
error_t httpClientSetMethod(HttpClientContext *context, const char_t *method);
error_t httpClientSetUri(HttpClientContext *context, const char_t *uri);

error_t httpClientSetHost(HttpClientContext *context, const char_t *host,
   uint16_t port);

error_t httpClientSetQueryString(HttpClientContext *context,
   const char_t *queryString);

error_t httpClientAddQueryParam(HttpClientContext *context,
   const char_t *name, const char_t *value);

error_t httpClientAddHeaderField(HttpClientContext *context,
   const char_t *name, const char_t *value);

error_t httpClientFormatHeaderField(HttpClientContext *context,
   const char_t *name, const char_t *format, ...);

error_t httpClientSetContentLength(HttpClientContext *context, size_t length);
error_t httpClientWriteHeader(HttpClientContext *context);

error_t httpClientWriteBody(HttpClientContext *context, const void *data,
   size_t length, size_t *written, uint_t flags);

error_t httpClientWriteTrailer(HttpClientContext *context);

error_t httpClientReadHeader(HttpClientContext *context);
uint_t httpClientGetStatus(HttpClientContext *context);

const char_t *httpClientGetHeaderField(HttpClientContext *context,
   const char_t *name);

error_t httpClientGetNextHeaderField(HttpClientContext *context,
   const char_t **name, const char_t **value);

error_t httpClientReadBody(HttpClientContext *context, void *data,
   size_t size, size_t *received, uint_t flags);

error_t httpClientReadTrailer(HttpClientContext *context);
error_t httpClientCloseBody(HttpClientContext *context);

error_t httpClientDisconnect(HttpClientContext *context);
error_t httpClientClose(HttpClientContext *context);

void httpClientDeinit(HttpClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
