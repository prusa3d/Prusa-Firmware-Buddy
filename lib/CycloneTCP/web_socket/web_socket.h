/**
 * @file web_socket.h
 * @brief WebSocket API (client and server)
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

#ifndef _WEB_SOCKET_H
#define _WEB_SOCKET_H

//Dependencies
#include "core/net.h"
#include "core/socket.h"

//WebSocket support
#ifndef WEB_SOCKET_SUPPORT
   #define WEB_SOCKET_SUPPORT DISABLED
#elif (WEB_SOCKET_SUPPORT != ENABLED && WEB_SOCKET_SUPPORT != DISABLED)
   #error WEB_SOCKET_SUPPORT parameter is not valid
#endif

//Number of WebSockets that can be opened simultaneously
#ifndef WEB_SOCKET_MAX_COUNT
   #define WEB_SOCKET_MAX_COUNT 4
#elif (WEB_SOCKET_MAX_COUNT < 1)
   #error WEB_SOCKET_MAX_COUNT parameter is not valid
#endif

//Support for WebSocket connections over TLS
#ifndef WEB_SOCKET_TLS_SUPPORT
   #define WEB_SOCKET_TLS_SUPPORT DISABLED
#elif (WEB_SOCKET_TLS_SUPPORT != ENABLED && WEB_SOCKET_TLS_SUPPORT != DISABLED)
   #error WEB_SOCKET_TLS_SUPPORT parameter is not valid
#endif

//Basic access authentication support
#ifndef WEB_SOCKET_BASIC_AUTH_SUPPORT
   #define WEB_SOCKET_BASIC_AUTH_SUPPORT DISABLED
#elif (WEB_SOCKET_BASIC_AUTH_SUPPORT != ENABLED && WEB_SOCKET_BASIC_AUTH_SUPPORT != DISABLED)
   #error WEB_SOCKET_BASIC_AUTH_SUPPORT parameter is not valid
#endif

//Digest access authentication support
#ifndef WEB_SOCKET_DIGEST_AUTH_SUPPORT
   #define WEB_SOCKET_DIGEST_AUTH_SUPPORT DISABLED
#elif (WEB_SOCKET_DIGEST_AUTH_SUPPORT != ENABLED && WEB_SOCKET_DIGEST_AUTH_SUPPORT != DISABLED)
   #error WEB_SOCKET_DIGEST_AUTH_SUPPORT parameter is not valid
#endif

//Maximum number of connection attempts
#ifndef WEB_SOCKET_MAX_CONN_RETRIES
   #define WEB_SOCKET_MAX_CONN_RETRIES 3
#elif (WEB_SOCKET_MAX_CONN_RETRIES < 1)
   #error WEB_SOCKET_MAX_CONN_RETRIES parameter is not valid
#endif

//Size of the WebSocket buffer
#ifndef WEB_SOCKET_BUFFER_SIZE
   #define WEB_SOCKET_BUFFER_SIZE 1024
#elif (WEB_SOCKET_BUFFER_SIZE < 128)
   #error WEB_SOCKET_BUFFER_SIZE parameter is not valid
#endif

//Maximum length of the hostname
#ifndef WEB_SOCKET_HOST_MAX_LEN
   #define WEB_SOCKET_HOST_MAX_LEN 32
#elif (WEB_SOCKET_HOST_MAX_LEN < 1)
   #error WEB_SOCKET_HOST_MAX_LEN parameter is not valid
#endif

//Maximum length of the origin header field
#ifndef WEB_SOCKET_ORIGIN_MAX_LEN
   #define WEB_SOCKET_ORIGIN_MAX_LEN 16
#elif (WEB_SOCKET_ORIGIN_MAX_LEN < 1)
   #error WEB_SOCKET_ORIGIN_MAX_LEN parameter is not valid
#endif

//Maximum length of the sub-protocol
#ifndef WEB_SOCKET_SUB_PROTOCOL_MAX_LEN
   #define WEB_SOCKET_SUB_PROTOCOL_MAX_LEN 8
#elif (WEB_SOCKET_SUB_PROTOCOL_MAX_LEN < 1)
   #error WEB_SOCKET_SUB_PROTOCOL_MAX_LEN parameter is not valid
#endif

//Maximum length of the URI
#ifndef WEB_SOCKET_URI_MAX_LEN
   #define WEB_SOCKET_URI_MAX_LEN 32
#elif (WEB_SOCKET_URI_MAX_LEN < 1)
   #error WEB_SOCKET_URI_MAX_LEN parameter is not valid
#endif

//Maximum length of the query string
#ifndef WEB_SOCKET_QUERY_STRING_MAX_LEN
   #define WEB_SOCKET_QUERY_STRING_MAX_LEN 32
#elif (WEB_SOCKET_QUERY_STRING_MAX_LEN < 1)
   #error WEB_SOCKET_QUERY_STRING_MAX_LEN parameter is not valid
#endif

//Maximum length of the realm
#ifndef WEB_SOCKET_REALM_MAX_LEN
   #define WEB_SOCKET_REALM_MAX_LEN 32
#elif (WEB_SOCKET_REALM_MAX_LEN < 1)
   #error WEB_SOCKET_REALM_MAX_LEN parameter is not valid
#endif

//Maximum length of the user name
#ifndef WEB_SOCKET_USERNAME_MAX_LEN
   #define WEB_SOCKET_USERNAME_MAX_LEN 16
#elif (WEB_SOCKET_USERNAME_MAX_LEN < 1)
   #error WEB_SOCKET_USERNAME_MAX_LEN parameter is not valid
#endif

//Maximum length of the password
#ifndef WEB_SOCKET_PASSWORD_MAX_LEN
   #define WEB_SOCKET_PASSWORD_MAX_LEN 16
#elif (WEB_SOCKET_PASSWORD_MAX_LEN < 1)
   #error WEB_SOCKET_PASSWORD_MAX_LEN parameter is not valid
#endif

//Maximum length of the nonce
#ifndef WEB_SOCKET_NONCE_MAX_LEN
   #define WEB_SOCKET_NONCE_MAX_LEN 32
#elif (WEB_SOCKET_NONCE_MAX_LEN < 1)
   #error WEB_SOCKET_NONCE_MAX_LEN parameter is not valid
#endif

//Maximum length of the opaque parameter
#ifndef WEB_SOCKET_OPAQUE_MAX_LEN
   #define WEB_SOCKET_OPAQUE_MAX_LEN 32
#elif (WEB_SOCKET_OPAQUE_MAX_LEN < 1)
   #error WEB_SOCKET_OPAQUE_MAX_LEN parameter is not valid
#endif

//Cnonce size
#ifndef WEB_SOCKET_CNONCE_SIZE
   #define WEB_SOCKET_CNONCE_SIZE 16
#elif (WEB_SOCKET_CNONCE_SIZE < 1)
   #error WEB_SOCKET_CNONCE_SIZE parameter is not valid
#endif

//TLS supported?
#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//Client key size
#define WEB_SOCKET_CLIENT_KEY_SIZE 24
//Server key size
#define WEB_SOCKET_SERVER_KEY_SIZE 28

//Forward declaration of WebSocket structure
struct _WebSocket;
#define WebSocket struct _WebSocket

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief WebSocket endpoint types
 **/

typedef enum
{
   WS_ENDPOINT_CLIENT = 0,
   WS_ENDPOINT_SERVER = 1
} WebSocketEndpoint;


/**
 * @brief HTTP version numbers
 **/

typedef enum
{
   WS_HTTP_VERSION_0_9 = 0x0009,
   WS_HTTP_VERSION_1_0 = 0x0100,
   WS_HTTP_VERSION_1_1 = 0x0101
} WebSocketHttpVersion;


/**
 * @brief Authentication schemes
 **/

typedef enum
{
   WS_AUTH_MODE_NONE   = 0x00,
   WS_AUTH_MODE_BASIC  = 0x01,
   WS_AUTH_MODE_DIGEST = 0x02
} WebSocketAuthMode;


/**
 * @brief WebSocket states
 **/

typedef enum
{
   WS_STATE_UNUSED             = 0,
   WS_STATE_CLOSED             = 1,
   WS_STATE_INIT               = 2,
   WS_STATE_CONNECTING         = 3,
   WS_STATE_CLIENT_HANDSHAKE   = 4,
   WS_STATE_SERVER_HANDSHAKE   = 5,
   WS_STATE_SERVER_RESP_BODY   = 6,
   WS_STATE_OPEN               = 7,
   WS_STATE_CLOSING_TX         = 8,
   WS_STATE_CLOSING_RX         = 9,
   WS_STATE_SHUTDOWN           = 10,
} WebSocketState;


/**
 * @brief WebSocket sub-states
 **/

typedef enum
{
   WS_SUB_STATE_INIT                   = 0,
   //Handshake decoding
   WS_SUB_STATE_HANDSHAKE_LEADING_LINE = 1,
   WS_SUB_STATE_HANDSHAKE_HEADER_FIELD = 2,
   WS_SUB_STATE_HANDSHAKE_LWSP         = 3,
   //WebSocket frame decoding
   WS_SUB_STATE_FRAME_HEADER           = 4,
   WS_SUB_STATE_FRAME_EXT_HEADER       = 5,
   WS_SUB_STATE_FRAME_PAYLOAD          = 6
} WebSocketSubState;


/**
 * @brief WebSocket frame types
 **/

typedef enum
{
   WS_FRAME_TYPE_CONTINUATION = 0x00,
   WS_FRAME_TYPE_TEXT         = 0x01,
   WS_FRAME_TYPE_BINARY       = 0x02,
   WS_FRAME_TYPE_CLOSE        = 0x08,
   WS_FRAME_TYPE_PING         = 0x09,
   WS_FRAME_TYPE_PONG         = 0x0A
} WebSocketFrameType;


/**
 * @brief WebSocket status codes
 **/

typedef enum
{
   WS_STATUS_CODE_NORMAL_CLOSURE       = 1000,
   WS_STATUS_CODE_GOING_AWAY           = 1001,
   WS_STATUS_CODE_PROTOCOL_ERROR       = 1002,
   WS_STATUS_CODE_UNSUPPORTED_DATA     = 1003,
   WS_STATUS_CODE_NO_STATUS_RCVD       = 1005,
   WS_STATUS_CODE_ABNORMAL_CLOSURE     = 1006,
   WS_STATUS_CODE_INVALID_PAYLOAD_DATA = 1007,
   WS_STATUS_CODE_POLICY_VIOLATION     = 1008,
   WS_STATUS_CODE_MESSAGE_TOO_BIG      = 1009,
   WS_STATUS_CODE_MANDATORY_EXT        = 1010,
   WS_STATUS_CODE_INTERNAL_ERROR       = 1011,
   WS_STATUS_CODE_TLS_HANDSHAKE        = 1015
} WebSocketStatusCode;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief WebSocket frame
 **/

typedef __start_packed struct
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t fin : 1;         //0
   uint8_t reserved : 3;
   uint8_t opcode : 4;
   uint8_t mask : 1;        //1
   uint8_t payloadLen : 7;
#else
   uint8_t opcode : 4;      //0
   uint8_t reserved : 3;
   uint8_t fin : 1;
   uint8_t payloadLen : 7;  //1
   uint8_t mask : 1;
#endif
   uint8_t extPayloadLen[]; //2
} __end_packed WebSocketFrame;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief Random data generation callback function
 **/

typedef error_t (*WebSocketRandCallback)(uint8_t *data, size_t length);


//TLS supported?
#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)

/**
 * @brief TLS initialization callback function
 **/

typedef error_t (*WebSocketTlsInitCallback)(WebSocket *webSocket,
   TlsContext *tlsContext);

#endif


/**
 * @brief Authentication context
 **/

typedef struct
{
   uint_t allowedAuthModes;
   WebSocketAuthMode requiredAuthMode;
   WebSocketAuthMode selectedAuthMode;
   char_t username[WEB_SOCKET_USERNAME_MAX_LEN + 1];
   char_t password[WEB_SOCKET_PASSWORD_MAX_LEN + 1];
   char_t realm[WEB_SOCKET_REALM_MAX_LEN + 1];
#if (WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
   uint32_t nc;
   char_t nonce[WEB_SOCKET_NONCE_MAX_LEN + 1];
   char_t cnonce[WEB_SOCKET_CNONCE_SIZE * 2 + 1];
   char_t opaque[WEB_SOCKET_OPAQUE_MAX_LEN + 1];
   bool_t stale;
#endif
} WebSocketAuthContext;


/**
 * @brief Handshake context
 **/

typedef struct
{
   uint_t version;
   uint_t statusCode;
   bool_t upgradeWebSocket;
   bool_t connectionUpgrade;
   bool_t connectionClose;
   size_t contentLength;
   char_t clientKey[WEB_SOCKET_CLIENT_KEY_SIZE + 1];
   char_t serverKey[WEB_SOCKET_SERVER_KEY_SIZE + 1];
   bool_t closingFrameSent;
   bool_t closingFrameReceived;
} WebSocketHandshakeContext;


/**
 * @brief Frame encoding/decoding context
 **/

typedef struct
{
   WebSocketSubState state;                ///FSM state
   WebSocketFrameType dataFrameType;       ///<Data frame type
   WebSocketFrameType controlFrameType;    ///<Control frame type
   bool_t fin;                             ///<Final fragment in a message
   bool_t mask;                            ///<Defines whether the payload data is masked
   uint8_t maskingKey[4];                  ///<Masking key
   size_t payloadLen;                      ///<Payload length
   size_t payloadPos;                      ///<Current position
   uint8_t buffer[WEB_SOCKET_BUFFER_SIZE]; ///<Data buffer
   size_t bufferLen;                       ///<Length of the data buffer
   size_t bufferPos;                       ///<Current position
} WebSocketFrameContext;


/**
 * @brief UTF-8 decoding context
 **/

typedef struct
{
   uint_t utf8CharSize;
   uint_t utf8CharIndex;
   uint32_t utf8CodePoint;
} WebSocketUtf8Context;


/**
 * @brief Structure describing a WebSocket
 **/

struct _WebSocket
{
   WebSocketEndpoint endpoint;               ///<Endpoint type (client or server)
   WebSocketState state;                     ///<WebSocket connection state
   uint16_t statusCode;
   systime_t timestamp;
   uint_t retryCount;
   char_t host[WEB_SOCKET_HOST_MAX_LEN + 1]; ///<Domain name of the server (for virtual hosting)
   char_t origin[WEB_SOCKET_ORIGIN_MAX_LEN + 1];
   char_t subProtocol[WEB_SOCKET_SUB_PROTOCOL_MAX_LEN + 1];
   char_t uri[WEB_SOCKET_URI_MAX_LEN + 1];
   char_t queryString[WEB_SOCKET_QUERY_STRING_MAX_LEN + 1];
   systime_t timeout;                        ///<timeout value for blocking operations
   NetInterface *interface;                  ///<Underlying network interface
   Socket *socket;                           ///<Underlying TCP socket
#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   TlsContext *tlsContext;                   ///<TLS context
   TlsSessionState tlsSession;               ///<TLS session state
   WebSocketTlsInitCallback tlsInitCallback; ///<TLS initialization callback function
#endif
#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
   WebSocketAuthContext authContext;
#endif
   WebSocketHandshakeContext handshakeContext;
   WebSocketFrameContext txContext;
   WebSocketFrameContext rxContext;
   WebSocketUtf8Context utf8Context;
};


//Random data generation callback function
extern WebSocketRandCallback webSockRandCallback;

//WebSocket related functions
error_t webSocketInit(void);

error_t webSocketRegisterRandCallback(WebSocketRandCallback callback);

WebSocket *webSocketOpen(void);
WebSocket *webSocketUpgradeSocket(Socket *socket);

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)

WebSocket *webSocketUpgradeSecureSocket(Socket *socket, TlsContext *tlsContext);

error_t webSocketRegisterTlsInitCallback(WebSocket *webSocket,
   WebSocketTlsInitCallback callback);

#endif

error_t webSocketSetTimeout(WebSocket *webSocket, systime_t timeout);

error_t webSocketSetHost(WebSocket *webSocket, const char_t *host);
error_t webSocketSetOrigin(WebSocket *webSocket, const char_t *origin);
error_t webSocketSetSubProtocol(WebSocket *webSocket, const char_t *subProtocol);

error_t webSocketSetAuthInfo(WebSocket *webSocket, const char_t *username,
   const char_t *password, uint_t allowedAuthModes);

error_t webSocketBindToInterface(WebSocket *webSocket, NetInterface *interface);

error_t webSocketConnect(WebSocket *webSocket, const IpAddr *serverIpAddr,
   uint16_t serverPort, const char_t *uri);

error_t webSocketSetClientKey(WebSocket *webSocket, const char_t *clientKey);
error_t webSocketParseClientHandshake(WebSocket *webSocket);
error_t webSocketSendServerHandshake(WebSocket *webSocket);

error_t webSocketSendErrorResponse(WebSocket *webSocket,
   uint_t statusCode, const char_t *message);

error_t webSocketSend(WebSocket *webSocket, const void *data,
   size_t length, WebSocketFrameType type, size_t *written);

error_t webSocketSendEx(WebSocket *webSocket, const void *data, size_t length,
   WebSocketFrameType type, size_t *written, bool_t firstFrag, bool_t lastFrag);

error_t webSocketReceive(WebSocket *webSocket, void *data,
   size_t size, WebSocketFrameType *type, size_t *received);

error_t webSocketReceiveEx(WebSocket *webSocket, void *data, size_t size,
   WebSocketFrameType *type, size_t *received, bool_t *firstFrag, bool_t *lastFrag);

bool_t webSocketIsRxReady(WebSocket *webSocket);
error_t webSocketShutdown(WebSocket *webSocket);
void webSocketClose(WebSocket *webSocket);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
