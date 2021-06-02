/**
 * @file http_server.h
 * @brief HTTP server (HyperText Transfer Protocol)
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

#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

//Dependencies
#include "os_port.h"
#include "buddy_socket.h"
//#include "web_socket/web_socket.h"
#include "http/http_common.h"

//HTTP server support
#ifndef HTTP_SERVER_SUPPORT
   #define HTTP_SERVER_SUPPORT ENABLED
#elif (HTTP_SERVER_SUPPORT != ENABLED && HTTP_SERVER_SUPPORT != DISABLED)
   #error HTTP_SERVER_SUPPORT parameter is not valid
#endif

//Support for persistent connections
#ifndef HTTP_SERVER_PERSISTENT_CONN_SUPPORT
   #define HTTP_SERVER_PERSISTENT_CONN_SUPPORT DISABLED
#elif (HTTP_SERVER_PERSISTENT_CONN_SUPPORT != ENABLED && HTTP_SERVER_PERSISTENT_CONN_SUPPORT != DISABLED)
   #error HTTP_SERVER_PERSISTENT_CONN_SUPPORT parameter is not valid
#endif

//File system support
#ifndef HTTP_SERVER_FS_SUPPORT
   #define HTTP_SERVER_FS_SUPPORT DISABLED
#elif (HTTP_SERVER_FS_SUPPORT != ENABLED && HTTP_SERVER_FS_SUPPORT != DISABLED)
   #error HTTP_SERVER_FS_SUPPORT parameter is not valid
#endif

//Server Side Includes support
#ifndef HTTP_SERVER_SSI_SUPPORT
   #define HTTP_SERVER_SSI_SUPPORT DISABLED
#elif (HTTP_SERVER_SSI_SUPPORT != ENABLED && HTTP_SERVER_SSI_SUPPORT != DISABLED)
   #error HTTP_SERVER_SSI_SUPPORT parameter is not valid
#endif

//HTTP over TLS
#ifndef HTTP_SERVER_TLS_SUPPORT
   #define HTTP_SERVER_TLS_SUPPORT DISABLED
#elif (HTTP_SERVER_TLS_SUPPORT != ENABLED && HTTP_SERVER_TLS_SUPPORT != DISABLED)
   #error HTTP_SERVER_TLS_SUPPORT parameter is not valid
#endif

//HTTP Strict Transport Security support
#ifndef HTTP_SERVER_HSTS_SUPPORT
   #define HTTP_SERVER_HSTS_SUPPORT DISABLED
#elif (HTTP_SERVER_HSTS_SUPPORT != ENABLED && HTTP_SERVER_HSTS_SUPPORT != DISABLED)
   #error HTTP_SERVER_HSTS_SUPPORT parameter is not valid
#endif

//Basic access authentication support
#ifndef HTTP_SERVER_BASIC_AUTH_SUPPORT
   #define HTTP_SERVER_BASIC_AUTH_SUPPORT DISABLED
#elif (HTTP_SERVER_BASIC_AUTH_SUPPORT != ENABLED && HTTP_SERVER_BASIC_AUTH_SUPPORT != DISABLED)
   #error HTTP_SERVER_BASIC_AUTH_SUPPORT parameter is not valid
#endif

//Digest access authentication support
#ifndef HTTP_SERVER_DIGEST_AUTH_SUPPORT
   #define HTTP_SERVER_DIGEST_AUTH_SUPPORT DISABLED
#elif (HTTP_SERVER_DIGEST_AUTH_SUPPORT != ENABLED && HTTP_SERVER_DIGEST_AUTH_SUPPORT != DISABLED)
   #error HTTP_SERVER_DIGEST_AUTH_SUPPORT parameter is not valid
#endif

//WebSocket support
#ifndef HTTP_SERVER_WEB_SOCKET_SUPPORT
   #define HTTP_SERVER_WEB_SOCKET_SUPPORT DISABLED
#elif (HTTP_SERVER_WEB_SOCKET_SUPPORT != ENABLED && HTTP_SERVER_WEB_SOCKET_SUPPORT != DISABLED)
   #error HTTP_SERVER_WEB_SOCKET_SUPPORT parameter is not valid
#endif

//Gzip content type support
#ifndef HTTP_SERVER_GZIP_TYPE_SUPPORT
   #define HTTP_SERVER_GZIP_TYPE_SUPPORT DISABLED
#elif (HTTP_SERVER_GZIP_TYPE_SUPPORT != ENABLED && HTTP_SERVER_GZIP_TYPE_SUPPORT != DISABLED)
   #error HTTP_SERVER_GZIP_TYPE_SUPPORT parameter is not valid
#endif

//Multipart content type support
#ifndef HTTP_SERVER_MULTIPART_TYPE_SUPPORT
   #define HTTP_SERVER_MULTIPART_TYPE_SUPPORT DISABLED
#elif (HTTP_SERVER_MULTIPART_TYPE_SUPPORT != ENABLED && HTTP_SERVER_MULTIPART_TYPE_SUPPORT != DISABLED)
   #error HTTP_SERVER_MULTIPART_TYPE_SUPPORT parameter is not valid
#endif

//Cookie support
#ifndef HTTP_SERVER_COOKIE_SUPPORT
   #define HTTP_SERVER_COOKIE_SUPPORT DISABLED
#elif (HTTP_SERVER_COOKIE_SUPPORT != ENABLED && HTTP_SERVER_COOKIE_SUPPORT != DISABLED)
   #error HTTP_SERVER_COOKIE_SUPPORT parameter is not valid
#endif

//Stack size required to run the HTTP server
#ifndef HTTP_SERVER_STACK_SIZE
   #define HTTP_SERVER_STACK_SIZE 650
#elif (HTTP_SERVER_STACK_SIZE < 1)
   #error HTTP_SERVER_STACK_SIZE parameter is not valid
#endif

//Priority at which the HTTP server should run
#ifndef HTTP_SERVER_PRIORITY
   #define HTTP_SERVER_PRIORITY OS_TASK_PRIORITY_NORMAL
#endif

//HTTP connection timeout
#ifndef HTTP_SERVER_TIMEOUT
   #define HTTP_SERVER_TIMEOUT 10000
#elif (HTTP_SERVER_TIMEOUT < 1000)
   #error HTTP_SERVER_TIMEOUT parameter is not valid
#endif

//Maximum time the server will wait for a subsequent
//request before closing the connection
#ifndef HTTP_SERVER_IDLE_TIMEOUT
   #define HTTP_SERVER_IDLE_TIMEOUT 5000
#elif (HTTP_SERVER_IDLE_TIMEOUT < 1000)
   #error HTTP_SERVER_IDLE_TIMEOUT parameter is not valid
#endif

//Maximum length of the pending connection queue
#ifndef HTTP_SERVER_BACKLOG
   #define HTTP_SERVER_BACKLOG 4
#elif (HTTP_SERVER_BACKLOG < 1)
   #error HTTP_SERVER_BACKLOG parameter is not valid
#endif

//Maximum number of requests per connection
#ifndef HTTP_SERVER_MAX_REQUESTS
   #define HTTP_SERVER_MAX_REQUESTS 1000
#elif (HTTP_SERVER_MAX_REQUESTS < 1)
   #error HTTP_SERVER_MAX_REQUESTS parameter is not valid
#endif

//Size of buffer used for input/output operations
#ifndef HTTP_SERVER_BUFFER_SIZE
   #define HTTP_SERVER_BUFFER_SIZE 1024
#elif (HTTP_SERVER_BUFFER_SIZE < 128)
   #error HTTP_SERVER_BUFFER_SIZE parameter is not valid
#endif

//Size of buffer used for data from lwip socket
#ifndef HTTP_SERVER_BIGBUFFER_SIZE
   #define HTTP_SERVER_BIGBUFFER_SIZE 1024
#elif (HTTP_SERVER_BIGBUFFER_SIZE < 128)
   #error HTTP_SERVER_BIGBUFFER_SIZE parameter is not valid
#endif

//Maximum size of root directory
#ifndef HTTP_SERVER_ROOT_DIR_MAX_LEN
   #define HTTP_SERVER_ROOT_DIR_MAX_LEN 31
#elif (HTTP_SERVER_ROOT_DIR_MAX_LEN < 7)
   #error HTTP_SERVER_ROOT_DIR_MAX_LEN parameter is not valid
#endif

//Maximum size of default index file
#ifndef HTTP_SERVER_DEFAULT_DOC_MAX_LEN
   #define HTTP_SERVER_DEFAULT_DOC_MAX_LEN 31
#elif (HTTP_SERVER_DEFAULT_DOC_MAX_LEN < 7)
   #error HTTP_SERVER_DEFAULT_DOC_MAX_LEN parameter is not valid
#endif

//Maximum length of HTTP method
#ifndef HTTP_SERVER_METHOD_MAX_LEN
   #define HTTP_SERVER_METHOD_MAX_LEN 7
#elif (HTTP_SERVER_METHOD_MAX_LEN < 1)
   #error HTTP_SERVER_METHOD_MAX_LEN parameter is not valid
#endif

//Maximum length of URI
#ifndef HTTP_SERVER_URI_MAX_LEN
   #define HTTP_SERVER_URI_MAX_LEN 255
#elif (HTTP_SERVER_URI_MAX_LEN < 31)
   #error HTTP_SERVER_URI_MAX_LEN parameter is not valid
#endif

//Maximum length of query strings
#ifndef HTTP_SERVER_QUERY_STRING_MAX_LEN
   #define HTTP_SERVER_QUERY_STRING_MAX_LEN 255
#elif (HTTP_SERVER_QUERY_STRING_MAX_LEN < 7)
   #error HTTP_SERVER_QUERY_STRING_MAX_LEN parameter is not valid
#endif

//Maximum host name length
#ifndef HTTP_SERVER_HOST_MAX_LEN
   #define HTTP_SERVER_HOST_MAX_LEN 31
#elif (HTTP_SERVER_HOST_MAX_LEN < 7)
   #error HTTP_SERVER_HOST_MAX_LEN parameter is not valid
#endif

//Maximum user name length
#ifndef HTTP_SERVER_USERNAME_MAX_LEN
   #define HTTP_SERVER_USERNAME_MAX_LEN 31
#elif (HTTP_SERVER_USERNAME_MAX_LEN < 7)
   #error HTTP_SERVER_USERNAME_MAX_LEN parameter is not valid
#endif

//Maximum length of CGI parameters
#ifndef HTTP_SERVER_CGI_PARAM_MAX_LEN
   #define HTTP_SERVER_CGI_PARAM_MAX_LEN 31
#elif (HTTP_SERVER_CGI_PARAM_MAX_LEN < 7)
   #error HTTP_SERVER_CGI_PARAM_MAX_LEN parameter is not valid
#endif

//Maximum recursion limit
#ifndef HTTP_SERVER_SSI_MAX_RECURSION
   #define HTTP_SERVER_SSI_MAX_RECURSION 3
#elif (HTTP_SERVER_SSI_MAX_RECURSION < 1 || HTTP_SERVER_SSI_MAX_RECURSION > 8)
   #error HTTP_SERVER_SSI_MAX_RECURSION parameter is not valid
#endif

//Maximum age for static resources
#ifndef HTTP_SERVER_MAX_AGE
   #define HTTP_SERVER_MAX_AGE 0
#elif (HTTP_SERVER_MAX_AGE < 0)
   #error HTTP_SERVER_MAX_AGE parameter is not valid
#endif

//Nonce cache size
#ifndef HTTP_SERVER_NONCE_CACHE_SIZE
   #define HTTP_SERVER_NONCE_CACHE_SIZE 8
#elif (HTTP_SERVER_NONCE_CACHE_SIZE < 1)
   #error HTTP_SERVER_NONCE_CACHE_SIZE parameter is not valid
#endif

//Lifetime of nonces
#ifndef HTTP_SERVER_NONCE_LIFETIME
   #define HTTP_SERVER_NONCE_LIFETIME 60000
#elif (HTTP_SERVER_NONCE_LIFETIME < 1000)
   #error HTTP_SERVER_NONCE_LIFETIME parameter is not valid
#endif

//Nonce size
#ifndef HTTP_SERVER_NONCE_SIZE
   #define HTTP_SERVER_NONCE_SIZE 16
#elif (HTTP_SERVER_NONCE_SIZE < 1)
   #error HTTP_SERVER_NONCE_SIZE parameter is not valid
#endif

//Maximum length for boundary string
#ifndef HTTP_SERVER_BOUNDARY_MAX_LEN
   #define HTTP_SERVER_BOUNDARY_MAX_LEN 70
#elif (HTTP_SERVER_BOUNDARY_MAX_LEN < 1)
   #error HTTP_SERVER_BOUNDARY_MAX_LEN parameter is not valid
#endif

//Maximum length for cookies
#ifndef HTTP_SERVER_COOKIE_MAX_LEN
   #define HTTP_SERVER_COOKIE_MAX_LEN 256
#elif (HTTP_SERVER_COOKIE_MAX_LEN < 1)
   #error HTTP_SERVER_COOKIE_MAX_LEN parameter is not valid
#endif

//Application specific context
#ifndef HTTP_SERVER_PRIVATE_CONTEXT
   #define HTTP_SERVER_PRIVATE_CONTEXT
#endif

//File system support?
#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
   #include "fs_port.h"
#else
   #include "resource_manager.h"
#endif

//TLS supported?
#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
   #include "tls_ticket.h"
#endif

//Basic authentication supported?
#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "encoding/base64.h"
#endif

//Digest authentication supported?
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "hash/md5.h"
#endif

//WebSocket supported?
#if (HTTP_SERVER_WEB_SOCKET_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "encoding/base64.h"
#endif

//HTTP port number
#define HTTP_PORT 80
//HTTPS port number (HTTP over TLS)
#define HTTPS_PORT 443

//Forward declaration of HttpServerContext structure
struct _HttpServerContext;
#define HttpServerContext struct _HttpServerContext

//Forward declaration of HttpConnection structure
struct _HttpConnection;
#define HttpConnection struct _HttpConnection

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Access status
 **/

typedef enum
{
   HTTP_ACCESS_DENIED               = 0,
   HTTP_ACCESS_ALLOWED              = 1,
   HTTP_ACCESS_BASIC_AUTH_REQUIRED  = 2,
   HTTP_ACCESS_DIGEST_AUTH_REQUIRED = 3
} HttpAccessStatus;


/**
 * @brief HTTP connection states
 **/

typedef enum
{
   HTTP_CONN_STATE_IDLE        = 0,
   HTTP_CONN_STATE_REQ_LINE    = 1,
   HTTP_CONN_STATE_REQ_HEADER  = 2,
   HTTP_CONN_STATE_REQ_BODY    = 3,
   HTTP_CONN_STATE_RESP_HEADER = 4,
   HTTP_CONN_STATE_RESP_BODY   = 5,
   HTTP_CONN_STATE_SHUTDOWN    = 6,
   HTTP_CONN_STATE_CLOSE       = 7
} HttpConnState;


//The HTTP_FLAG_BREAK macro causes the httpReadStream() function to stop
//reading data whenever the specified break character is encountered
#define HTTP_FLAG_BREAK(c) (HTTP_FLAG_BREAK_CHAR | LSB(c))


//TLS supported?
#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)

/**
 * @brief TLS initialization callback function
 **/

typedef error_t (*TlsInitCallback)(HttpConnection *connection,
   TlsContext *tlsContext);

#endif


/**
 * @brief Random data generation callback function
 **/

typedef error_t (*HttpRandCallback)(uint8_t *data, size_t length);


/**
 * @brief HTTP authentication callback function
 **/

typedef HttpAccessStatus (*HttpAuthCallback)(HttpConnection *connection,
   const char_t *user, const char_t *uri);


/**
 * @brief CGI callback function
 **/

typedef error_t (*HttpCgiCallback)(HttpConnection *connection,
   const char_t *param);


/**
 * @brief HTTP request callback function
 **/

typedef error_t (*HttpRequestCallback)(HttpConnection *connection,
   const char_t *uri);


/**
 * @brief URI not found callback function
 **/

typedef error_t (*HttpUriNotFoundCallback)(HttpConnection *connection,
   const char_t *uri);


/**
 * @brief HTTP status code
 **/

typedef struct
{
   uint_t value;
   const char_t message[28];
} HttpStatusCodeDesc;


/**
 * @brief Authorization header
 **/

typedef struct
{
   bool_t found;                                  ///<The Authorization header has been found
   HttpAuthMode mode;                             ///<Authentication scheme
   char_t user[HTTP_SERVER_USERNAME_MAX_LEN + 1]; ///<User name
#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED)
   const char_t *password;                        ///<Password
#endif
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   const char_t *realm;
   const char_t *nonce;                           ///<Server nonce
   const char_t *uri;                             ///<Digest URI
   const char_t *qop;
   const char_t *nc;                              ///<Nonce count
   const char_t *cnonce;                          ///<Client nonce
   const char_t *response;
   const char_t *opaque;
#endif
} HttpAuthorizationHeader;


/**
 * @brief Authenticate header
 **/

typedef struct
{
   HttpAuthMode mode; ///<Authentication scheme
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   bool_t stale;      ///<STALE flag
#endif
} HttpAuthenticateHeader;


/**
 * @brief HTTP request
 **/

typedef struct
{
   uint_t version;                                           ///<HTTP version number
   char_t method[HTTP_SERVER_METHOD_MAX_LEN + 1];            ///<HTTP method
   char_t uri[HTTP_SERVER_URI_MAX_LEN + 1];                  ///<Resource identifier
   char_t queryString[HTTP_SERVER_QUERY_STRING_MAX_LEN + 1]; ///<Query string
   char_t host[HTTP_SERVER_HOST_MAX_LEN + 1];                ///<Host name
   bool_t keepAlive;
   bool_t chunkedEncoding;
   size_t contentLength;
   size_t byteCount;
   bool_t firstChunk;
   bool_t lastChunk;
#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   HttpAuthorizationHeader auth;                             ///<Authorization header
#endif
#if (HTTP_SERVER_WEB_SOCKET_SUPPORT == ENABLED)
   bool_t upgradeWebSocket;
   bool_t connectionUpgrade;
   char_t clientKey[WEB_SOCKET_CLIENT_KEY_SIZE + 1];
#endif
#if (HTTP_SERVER_GZIP_TYPE_SUPPORT == ENABLED)
   bool_t acceptGzipEncoding;
#endif
#if (HTTP_SERVER_MULTIPART_TYPE_SUPPORT == ENABLED)
   char_t boundary[HTTP_SERVER_BOUNDARY_MAX_LEN + 1];        ///<Boundary string
   size_t boundaryLength;                                    ///<Boundary string length
#endif
#if (HTTP_SERVER_COOKIE_SUPPORT == ENABLED)
   char_t cookie[HTTP_SERVER_COOKIE_MAX_LEN + 1];            ///<Cookie header field
#endif
} HttpRequest;


/**
 * @brief HTTP response
 **/

typedef struct
{
   uint_t version;                                   ///<HTTP version number
   uint_t statusCode;                                ///<HTTP status code
   bool_t keepAlive;
   bool_t noCache;
   uint_t maxAge;
   const char_t *location;
   const char_t *contentType;
   bool_t chunkedEncoding;
   size_t contentLength;
   size_t byteCount;
#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   HttpAuthenticateHeader auth;                      ///<Authenticate header
#endif
#if (HTTP_SERVER_GZIP_TYPE_SUPPORT == ENABLED)
   bool_t gzipEncoding;
#endif
#if (HTTP_SERVER_COOKIE_SUPPORT == ENABLED)
   char_t setCookie[HTTP_SERVER_COOKIE_MAX_LEN + 1]; ///<Set-Cookie header field
#endif
} HttpResponse;


/**
 * @brief HTTP server settings
 **/

typedef struct
{
   NetInterface *interface;                                     ///<Underlying network interface
   uint16_t port;                                               ///<HTTP port number
   uint_t backlog;                                              ///<Maximum length of the pending connection queue
   uint_t maxConnections;                                       ///<Maximum number of client connections
   HttpConnection *connections;                                 ///<Client connections
   char_t rootDirectory[HTTP_SERVER_ROOT_DIR_MAX_LEN + 1];      ///<Web root directory
   char_t defaultDocument[HTTP_SERVER_DEFAULT_DOC_MAX_LEN + 1]; ///<Default home page
#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   bool_t useTls;                                               ///<Deprecated flag
   TlsInitCallback tlsInitCallback;                             ///<TLS initialization callback function
#endif
#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   HttpRandCallback randCallback;                               ///<Random data generation callback function
   HttpAuthCallback authCallback;                               ///<HTTP authentication callback function
#endif
   HttpCgiCallback cgiCallback;                                 ///<CGI callback function
   HttpRequestCallback requestCallback;                         ///<HTTP request callback function
   HttpUriNotFoundCallback uriNotFoundCallback;                 ///<URI not found callback function
} HttpServerSettings;


/**
 * @brief Nonce cache entry
 **/

typedef struct
{
   char_t nonce[HTTP_SERVER_NONCE_SIZE * 2 + 1]; ///<Nonce
   uint32_t count;                               ///<Nonce count
   systime_t timestamp;                          ///<Time stamp to manage entry lifetime
} HttpNonceCacheEntry;


/**
 * @brief HTTP server context
 **/

struct _HttpServerContext
{
   HttpServerSettings settings;                                  ///<User settings
   OsTask *taskHandle;                                           ///<Listener task handle
   OsSemaphore semaphore;                                        ///<Semaphore limiting the number of connections
   socket_t *socket;                                               ///<Listening socket
   HttpConnection *connections;                                  ///<Client connections
#if (HTTP_SERVER_TLS_SUPPORT == ENABLED && TLS_TICKET_SUPPORT == ENABLED)
   TlsTicketContext tlsTicketContext;                            ///<TLS ticket encryption context
#endif
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   OsMutex nonceCacheMutex;                                      ///<Mutex preventing simultaneous access to the nonce cache
   HttpNonceCacheEntry nonceCache[HTTP_SERVER_NONCE_CACHE_SIZE]; ///<Nonce cache
#endif
};


/**
 * @brief HTTP connection
 *
 * An HttpConnection instance represents one
 * transaction with an HTTP client
 *
 **/

struct _HttpConnection
{
   HttpServerSettings *settings;                       ///<Reference to the HTTP server settings
   HttpServerContext *serverContext;                   ///<Reference to the HTTP server context
   OsTask *taskHandle;                                 ///<Client task handle
   OsEvent startEvent;
   bool_t running;
   socket_t *socket;                                     ///<Socket
#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   TlsContext *tlsContext;                             ///<TLS context
#endif
   HttpRequest request;                                ///<Incoming HTTP request header
   HttpResponse response;                              ///<HTTP response header
   HttpAccessStatus status;                            ///<Access status
   char_t cgiParam[HTTP_SERVER_CGI_PARAM_MAX_LEN + 1]; ///<CGI parameter
   uint32_t dummy;                                     ///<Force alignment of the buffer on 32-bit boundaries
   char_t buffer[HTTP_SERVER_BUFFER_SIZE];             ///<Memory buffer for input/output operations
   char_t bigBuffer[HTTP_SERVER_BIGBUFFER_SIZE];
   size_t totalReceived;                               ///<Total data received
   size_t totalRead;                                   ///<Total data processed
#if (NET_RTOS_SUPPORT == DISABLED)
   HttpConnState state;                                ///<Connection state
   systime_t timestamp;
   size_t bufferPos;
   size_t bufferLen;
   uint8_t *bodyStart;
   size_t bodyPos;
   size_t bodyLen;
#endif
   HTTP_SERVER_PRIVATE_CONTEXT                         ///<Application specific context
};


//HTTP server related functions
void httpServerGetDefaultSettings(HttpServerSettings *settings);
error_t httpServerInit(HttpServerContext *context, const HttpServerSettings *settings);
error_t httpServerStart(HttpServerContext *context);

void httpListenerTask(void *param);
void httpConnectionTask(void *param);

error_t httpWriteHeader(HttpConnection *connection);

error_t httpReadStream(HttpConnection *connection,
   void *data, size_t size, size_t *received, uint_t flags);

error_t httpWriteStream(HttpConnection *connection,
   const void *data, size_t length);

error_t httpCloseStream(HttpConnection *connection);

error_t httpSendResponse(HttpConnection *connection, const char_t *uri);

error_t httpSendErrorResponse(HttpConnection *connection,
   uint_t statusCode, const char_t *message);

error_t httpSendRedirectResponse(HttpConnection *connection,
   uint_t statusCode, const char_t *uri);

//HTTP authentication related functions
bool_t httpCheckPassword(HttpConnection *connection,
   const char_t *password, HttpAuthMode mode);

//WebSocket related functions
#if 0
bool_t httpCheckWebSocketHandshake(HttpConnection *connection);
WebSocket *httpUpgradeToWebSocket(HttpConnection *connection);
#endif
//Miscellaneous functions
error_t httpDecodePercentEncodedString(const char_t *input,
   char_t *output, size_t outputSize);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
