/**
 * @file smtp_client.h
 * @brief SMTP client (Simple Mail Transfer Protocol)
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

#ifndef _SMTP_CLIENT_H
#define _SMTP_CLIENT_H

//Dependencies
#include "core/net.h"

//SMTP client support
#ifndef SMTP_CLIENT_SUPPORT
   #define SMTP_CLIENT_SUPPORT ENABLED
#elif (SMTP_CLIENT_SUPPORT != ENABLED && SMTP_CLIENT_SUPPORT != DISABLED)
   #error SMTP_CLIENT_SUPPORT parameter is not valid
#endif

//MIME extension support
#ifndef SMTP_CLIENT_MIME_SUPPORT
   #define SMTP_CLIENT_MIME_SUPPORT ENABLED
#elif (SMTP_CLIENT_MIME_SUPPORT != ENABLED && SMTP_CLIENT_MIME_SUPPORT != DISABLED)
   #error SMTP_CLIENT_MIME_SUPPORT parameter is not valid
#endif

//SMTP over TLS
#ifndef SMTP_CLIENT_TLS_SUPPORT
   #define SMTP_CLIENT_TLS_SUPPORT DISABLED
#elif (SMTP_CLIENT_TLS_SUPPORT != ENABLED && SMTP_CLIENT_TLS_SUPPORT != DISABLED)
   #error SMTP_CLIENT_TLS_SUPPORT parameter is not valid
#endif

//LOGIN authentication support
#ifndef SMTP_CLIENT_LOGIN_AUTH_SUPPORT
   #define SMTP_CLIENT_LOGIN_AUTH_SUPPORT ENABLED
#elif (SMTP_CLIENT_LOGIN_AUTH_SUPPORT != ENABLED && SMTP_CLIENT_LOGIN_AUTH_SUPPORT != DISABLED)
   #error SMTP_CLIENT_LOGIN_AUTH_SUPPORT parameter is not valid
#endif

//PLAIN authentication support
#ifndef SMTP_CLIENT_PLAIN_AUTH_SUPPORT
   #define SMTP_CLIENT_PLAIN_AUTH_SUPPORT ENABLED
#elif (SMTP_CLIENT_PLAIN_AUTH_SUPPORT != ENABLED && SMTP_CLIENT_PLAIN_AUTH_SUPPORT != DISABLED)
   #error SMTP_CLIENT_PLAIN_AUTH_SUPPORT parameter is not valid
#endif

//CRAM-MD5 authentication support
#ifndef SMTP_CLIENT_CRAM_MD5_AUTH_SUPPORT
   #define SMTP_CLIENT_CRAM_MD5_AUTH_SUPPORT DISABLED
#elif (SMTP_CLIENT_CRAM_MD5_AUTH_SUPPORT != ENABLED && SMTP_CLIENT_CRAM_MD5_AUTH_SUPPORT != DISABLED)
   #error SMTP_CLIENT_CRAM_MD5_AUTH_SUPPORT parameter is not valid
#endif

//Default timeout
#ifndef SMTP_CLIENT_DEFAULT_TIMEOUT
   #define SMTP_CLIENT_DEFAULT_TIMEOUT 20000
#elif (SMTP_CLIENT_DEFAULT_TIMEOUT < 1000)
   #error SMTP_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//Size of the buffer for input/output operations
#ifndef SMTP_CLIENT_BUFFER_SIZE
   #define SMTP_CLIENT_BUFFER_SIZE 512
#elif (SMTP_CLIENT_BUFFER_SIZE < 64)
   #error SMTP_CLIENT_BUFFER_SIZE parameter is not valid
#endif

//TX buffer size for TLS connections
#ifndef SMTP_CLIENT_TLS_TX_BUFFER_SIZE
   #define SMTP_CLIENT_TLS_TX_BUFFER_SIZE 2048
#elif (SMTP_CLIENT_TLS_TX_BUFFER_SIZE < 512)
   #error SMTP_CLIENT_TLS_TX_BUFFER_SIZE parameter is not valid
#endif

//RX buffer size for TLS connections
#ifndef SMTP_CLIENT_TLS_RX_BUFFER_SIZE
   #define SMTP_CLIENT_TLS_RX_BUFFER_SIZE 4096
#elif (SMTP_CLIENT_TLS_RX_BUFFER_SIZE < 512)
   #error SMTP_CLIENT_TLS_RX_BUFFER_SIZE parameter is not valid
#endif

//Maximum length for content type
#ifndef SMTP_CLIENT_CONTENT_TYPE_MAX_LEN
   #define SMTP_CLIENT_CONTENT_TYPE_MAX_LEN 32
#elif (SMTP_CLIENT_CONTENT_TYPE_MAX_LEN < 1)
   #error SMTP_CLIENT_CONTENT_TYPE_MAX_LEN parameter is not valid
#endif

//Maximum length for boundary string
#ifndef SMTP_CLIENT_BOUNDARY_MAX_LEN
   #define SMTP_CLIENT_BOUNDARY_MAX_LEN 70
#elif (SMTP_CLIENT_BOUNDARY_MAX_LEN < 1)
   #error SMTP_CLIENT_BOUNDARY_MAX_LEN parameter is not valid
#endif

//TLS supported?
#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//LOGIN or PLAIN authentication supported?
#if (SMTP_CLIENT_LOGIN_AUTH_SUPPORT == ENABLED || SMTP_CLIENT_PLAIN_AUTH_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "encoding/base64.h"
#endif

//CRAM-MD5 authentication supported?
#if (SMTP_CLIENT_CRAM_MD5_AUTH_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "encoding/base64.h"
   #include "mac/hmac.h"
   #include "hash/md5.h"
#endif

//SMTP port number (message relaying)
#define SMTP_RELAY_PORT 25
//SMTP port number (message submission)
#define SMTP_SUBMISSION_PORT 587
//SMTPS port number (message submission over TLS)
#define SMTPS_SUBMISSION_PORT 465

//Test macros for SMTP response codes
#define SMTP_REPLY_CODE_2YZ(code) ((code) >= 200 && (code) < 300)
#define SMTP_REPLY_CODE_3YZ(code) ((code) >= 300 && (code) < 400)
#define SMTP_REPLY_CODE_4YZ(code) ((code) >= 400 && (code) < 500)
#define SMTP_REPLY_CODE_5YZ(code) ((code) >= 500 && (code) < 600)

//Forward declaration of SmtpClientContext structure
struct _SmtpClientContext;
#define SmtpClientContext struct _SmtpClientContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief SMTP connection modes
 **/

typedef enum
{
   SMTP_MODE_PLAINTEXT    = 1,
   SMTP_MODE_IMPLICIT_TLS = 2,
   SMTP_MODE_EXPLICIT_TLS = 3
} SmtpConnectionMode;


/**
 * @brief Email address types
 **/

typedef enum
{
   SMTP_ADDR_TYPE_FROM = 0,
   SMTP_ADDR_TYPE_TO   = 1,
   SMTP_ADDR_TYPE_CC   = 2,
   SMTP_ADDR_TYPE_BCC  = 3
} SmtpMailAddrType;


/**
 * @brief SMTP client states
 */

typedef enum
{
   SMTP_CLIENT_STATE_DISCONNECTED     = 0,
   SMTP_CLIENT_STATE_CONNECTING_TCP   = 1,
   SMTP_CLIENT_STATE_CONNECTING_TLS   = 2,
   SMTP_CLIENT_STATE_CONNECTED        = 3,
   SMTP_CLIENT_STATE_SUB_COMMAND_1    = 4,
   SMTP_CLIENT_STATE_SUB_COMMAND_2    = 5,
   SMTP_CLIENT_STATE_SUB_COMMAND_3    = 6,
   SMTP_CLIENT_STATE_MAIL_HEADER      = 7,
   SMTP_CLIENT_STATE_MAIL_BODY        = 8,
   SMTP_CLIENT_STATE_MULTIPART_HEADER = 9,
   SMTP_CLIENT_STATE_MULTIPART_BODY   = 10,
   SMTP_CLIENT_STATE_DISCONNECTING    = 11
} SmtpClientState;


/**
 * @brief Multiline reply parsing callback function
 **/

typedef error_t (*SmtpClientReplyCallback)(SmtpClientContext *context,
   char_t *replyLine);


//TLS supported?
#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief TLS initialization callback function
 **/

typedef error_t (*SmtpClientTlsInitCallback)(SmtpClientContext *context,
   TlsContext *tlsContext);

#endif


/**
 * @brief Email address
 **/

typedef struct
{
   char_t *name;
   char_t *addr;
   SmtpMailAddrType type;
} SmtpMailAddr;


/**
 * @brief SMTP client context
 **/

struct _SmtpClientContext
{
   SmtpClientState state;                      ///<SMTP client state
   NetInterface *interface;                    ///<Underlying network interface
   systime_t timeout;                          ///<Timeout value
   systime_t timestamp;                        ///<Timestamp to manage timeout
   Socket *socket;                             ///<Underlying socket
#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   TlsContext *tlsContext;                     ///<TLS context
   TlsSessionState tlsSession;                 ///<TLS session state
   SmtpClientTlsInitCallback tlsInitCallback;  ///<TLS initialization callback function
#endif
#if (SMTP_CLIENT_MIME_SUPPORT == ENABLED)
   char_t contentType[SMTP_CLIENT_CONTENT_TYPE_MAX_LEN + 1]; ///<Content type
   char_t boundary[SMTP_CLIENT_BOUNDARY_MAX_LEN + 1];        ///<Boundary string
   bool_t base64Encoding;                                    ///<Base64 encoding
#endif
   bool_t startTlsSupported;                   ///<STARTTLS command supported
   bool_t authLoginSupported;                  ///<LOGIN authentication mechanism supported
   bool_t authPlainSupported;                  ///<PLAIN authentication mechanism supported
   bool_t authCramMd5Supported;                ///<CRAM-MD5 authentication mechanism supported
   char_t buffer[SMTP_CLIENT_BUFFER_SIZE + 1]; ///<Memory buffer for input/output operations
   size_t bufferLen;                           ///<Length of the buffer, in bytes
   size_t bufferPos;                           ///<Current position in the buffer
   size_t commandLen;                          ///<Length of the SMTP command, in bytes
   size_t replyLen;                            ///<Length of the SMTP reply, in bytes
   uint_t replyCode;                           ///<SMTP reply code
   uint_t recipientIndex;                      ///<Index of the current recipient
};


//Callback function to parse a response line
typedef error_t (*SmtpReplyCallback)(SmtpClientContext *context, char_t *replyLine, uint_t replyCode);

//SMTP related functions
error_t smtpClientInit(SmtpClientContext *context);

#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)

error_t smtpClientRegisterTlsInitCallback(SmtpClientContext *context,
   SmtpClientTlsInitCallback callback);

#endif

error_t smtpClientSetTimeout(SmtpClientContext *context, systime_t timeout);

error_t smtpClientBindToInterface(SmtpClientContext *context,
   NetInterface *interface);

error_t smtpClientConnect(SmtpClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort, SmtpConnectionMode mode);

error_t smtpClientLogin(SmtpClientContext *context, const char_t *username,
   const char_t *password);

error_t smtpClientSetContentType(SmtpClientContext *context,
   const char_t *contentType);

error_t smtpClientSetMultipartBoundary(SmtpClientContext *context,
   const char_t *boundary);

error_t smtpClientWriteMailHeader(SmtpClientContext *context,
   const SmtpMailAddr *from, const SmtpMailAddr *recipients,
   uint_t numRecipients, const char_t *subject);

error_t smtpClientWriteMailBody(SmtpClientContext *context,
   const void *data, size_t length, size_t *written, uint_t flags);

error_t smtpClientWriteMultipartHeader(SmtpClientContext *context,
   const char_t *filename, const char_t *contentType,
   const char_t *contentTransferEncoding, bool_t last);

error_t smtpClientWriteMultipartBody(SmtpClientContext *context,
   const void *data, size_t length, size_t *written, uint_t flags);

error_t smtpClientCloseMailBody(SmtpClientContext *context);

uint_t smtpClientGetReplyCode(SmtpClientContext *context);

error_t smtpClientDisconnect(SmtpClientContext *context);
error_t smtpClientClose(SmtpClientContext *context);

void smtpClientDeinit(SmtpClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
