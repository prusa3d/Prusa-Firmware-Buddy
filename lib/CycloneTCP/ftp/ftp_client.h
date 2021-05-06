/**
 * @file ftp_client.h
 * @brief FTP client (File Transfer Protocol)
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

#ifndef _FTP_CLIENT_H
#define _FTP_CLIENT_H

//Dependencies
#include "core/net.h"
#include "date_time.h"

//FTP client support
#ifndef FTP_CLIENT_SUPPORT
   #define FTP_CLIENT_SUPPORT ENABLED
#elif (FTP_CLIENT_SUPPORT != ENABLED && FTP_CLIENT_SUPPORT != DISABLED)
   #error FTP_CLIENT_SUPPORT parameter is not valid
#endif

//FTP over TLS
#ifndef FTP_CLIENT_TLS_SUPPORT
   #define FTP_CLIENT_TLS_SUPPORT DISABLED
#elif (FTP_CLIENT_TLS_SUPPORT != ENABLED && FTP_CLIENT_TLS_SUPPORT != DISABLED)
   #error FTP_CLIENT_TLS_SUPPORT parameter is not valid
#endif

//Default timeout
#ifndef FTP_CLIENT_DEFAULT_TIMEOUT
   #define FTP_CLIENT_DEFAULT_TIMEOUT 20000
#elif (FTP_CLIENT_DEFAULT_TIMEOUT < 1000)
   #error FTP_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//Size of the buffer for input/output operations
#ifndef FTP_CLIENT_BUFFER_SIZE
   #define FTP_CLIENT_BUFFER_SIZE 512
#elif (FTP_CLIENT_BUFFER_SIZE < 64)
   #error FTP_CLIENT_BUFFER_SIZE parameter is not valid
#endif

//Minimum buffer size for TCP sockets
#ifndef FTP_CLIENT_MIN_TCP_BUFFER_SIZE
   #define FTP_CLIENT_MIN_TCP_BUFFER_SIZE 1430
#elif (FTP_CLIENT_MIN_TCP_BUFFER_SIZE < 512)
   #error FTP_CLIENT_MIN_TCP_BUFFER_SIZE parameter is not valid
#endif

//Maximum buffer size for TCP sockets
#ifndef FTP_CLIENT_MAX_TCP_BUFFER_SIZE
   #define FTP_CLIENT_MAX_TCP_BUFFER_SIZE 2860
#elif (FTP_CLIENT_MAX_TCP_BUFFER_SIZE < 512)
   #error FTP_CLIENT_MAX_TCP_BUFFER_SIZE parameter is not valid
#endif

//TX buffer size for TLS connections
#ifndef FTP_CLIENT_TLS_TX_BUFFER_SIZE
   #define FTP_CLIENT_TLS_TX_BUFFER_SIZE 2048
#elif (FTP_CLIENT_TLS_TX_BUFFER_SIZE < 512)
   #error FTP_CLIENT_TLS_TX_BUFFER_SIZE parameter is not valid
#endif

//Minimum RX buffer size for TLS connections
#ifndef FTP_CLIENT_MIN_TLS_RX_BUFFER_SIZE
   #define FTP_CLIENT_MIN_TLS_RX_BUFFER_SIZE 4096
#elif (FTP_CLIENT_MIN_TLS_RX_BUFFER_SIZE < 512)
   #error FTP_CLIENT_MIN_TLS_RX_BUFFER_SIZE parameter is not valid
#endif

//Maximum RX buffer size for TLS connections
#ifndef FTP_CLIENT_MAX_TLS_RX_BUFFER_SIZE
   #define FTP_CLIENT_MAX_TLS_RX_BUFFER_SIZE 16384
#elif (FTP_CLIENT_MAX_TLS_RX_BUFFER_SIZE < 512)
   #error FTP_CLIENT_MAX_TLS_RX_BUFFER_SIZE parameter is not valid
#endif

//Maximum length of file names
#ifndef FTP_CLIENT_MAX_FILENAME_LEN
   #define FTP_CLIENT_MAX_FILENAME_LEN 64
#elif (FTP_CLIENT_MAX_FILENAME_LEN < 16)
   #error FTP_CLIENT_MAX_FILENAME_LEN parameter is not valid
#endif

//TLS supported?
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//Test macros for FTP response codes
#define FTP_REPLY_CODE_1YZ(code) ((code) >= 100 && (code) < 200)
#define FTP_REPLY_CODE_2YZ(code) ((code) >= 200 && (code) < 300)
#define FTP_REPLY_CODE_3YZ(code) ((code) >= 300 && (code) < 400)
#define FTP_REPLY_CODE_4YZ(code) ((code) >= 400 && (code) < 500)
#define FTP_REPLY_CODE_5YZ(code) ((code) >= 500 && (code) < 600)

//Forward declaration of FtpClientContext structure
struct _FtpClientContext;
#define FtpClientContext struct _FtpClientContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief FTP connection modes
 **/

typedef enum
{
   FTP_MODE_PLAINTEXT    = 0,
   FTP_MODE_IMPLICIT_TLS = 1,
   FTP_MODE_EXPLICIT_TLS = 2,
   FTP_MODE_ACTIVE       = 0,
   FTP_MODE_PASSIVE      = 4
} FtpConnectionModes;


/**
 * @brief File access modes
 **/

typedef enum
{
   FTP_FILE_MODE_READ   = 0,
   FTP_FILE_MODE_WRITE  = 1,
   FTP_FILE_MODE_APPEND = 2,
   FTP_FILE_MODE_BINARY = 0,
   FTP_FILE_MODE_TEXT   = 4
} FtpFileModes;


/**
 * @brief Flags used by I/O functions
 **/

typedef enum
{
   FTP_FLAG_WAIT_ALL   = 0x0800,
   FTP_FLAG_BREAK_CHAR = 0x1000,
   FTP_FLAG_BREAK_CRLF = 0x100A,
   FTP_FLAG_NO_DELAY   = 0x4000,
   FTP_FLAG_DELAY      = 0x8000
} FtpFileFlags;


/**
 * @brief File attributes
 **/

typedef enum
{
   FTP_FILE_ATTR_DIRECTORY = 1,
   FTP_FILE_ATTR_READ_ONLY = 2
} FtpFileAttributes;


/**
 * @brief FTP client states
 */

typedef enum
{
   FTP_CLIENT_STATE_DISCONNECTED    = 0,
   FTP_CLIENT_STATE_ACCEPTING_TCP   = 1,
   FTP_CLIENT_STATE_CONNECTING_TCP  = 2,
   FTP_CLIENT_STATE_CONNECTING_TLS  = 3,
   FTP_CLIENT_STATE_CONNECTED       = 4,
   FTP_CLIENT_STATE_SUB_COMMAND_1   = 5,
   FTP_CLIENT_STATE_SUB_COMMAND_2   = 6,
   FTP_CLIENT_STATE_SUB_COMMAND_3   = 7,
   FTP_CLIENT_STATE_SUB_COMMAND_4   = 8,
   FTP_CLIENT_STATE_SUB_COMMAND_5   = 9,
   FTP_CLIENT_STATE_SUB_COMMAND_6   = 10,
   FTP_CLIENT_STATE_SUB_COMMAND_7   = 11,
   FTP_CLIENT_STATE_SUB_COMMAND_8   = 12,
   FTP_CLIENT_STATE_SUB_COMMAND_9   = 13,
   FTP_CLIENT_STATE_WRITING_DATA    = 14,
   FTP_CLIENT_STATE_READING_DATA    = 15,
   FTP_CLIENT_STATE_DISCONNECTING_1 = 16,
   FTP_CLIENT_STATE_DISCONNECTING_2 = 17
} FtpClientState;


//TLS supported?
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief TLS initialization callback function
 **/

typedef error_t (*FtpClientTlsInitCallback)(FtpClientContext *context,
   TlsContext *tlsContext);

#endif


/**
 * @brief Control or data channel
 **/

typedef struct
{
   Socket *socket;         ///<Underlying TCP socket
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   TlsContext *tlsContext; ///<TLS context
#endif
} FtpClientChannel;


/**
 * @brief FTP client context
 **/

struct _FtpClientContext
{
   FtpClientState state;                     ///<FTP client state
   NetInterface *interface;                  ///<Underlying network interface
   systime_t timeout;                        ///<Timeout value
   systime_t timestamp;                      ///<Timestamp to manage timeout
   IpAddr serverIpAddr;                      ///<IP address of the FTP server
   uint16_t serverPort;                      ///<TCP port number
   bool_t passiveMode;                       ///<Passive mode
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   TlsSessionState tlsSession;               ///<TLS session state
   FtpClientTlsInitCallback tlsInitCallback; ///<TLS initialization callback function
#endif
   FtpClientChannel controlChannel;          ///<Control channel
   FtpClientChannel dataChannel;             ///<Data channel
   char_t buffer[FTP_CLIENT_BUFFER_SIZE];    ///<Memory buffer for input/output operations
   size_t bufferPos;                         ///<Current position in the buffer
   size_t commandLen;                        ///<Length of the FTP command, in bytes
   size_t replyLen;                          ///<Length of the FTP reply, in bytes
   uint_t replyCode;                         ///<FTP reply code
};


/**
 * @brief Directory entry
 **/

typedef struct
{
   char_t name[FTP_CLIENT_MAX_FILENAME_LEN + 1];
   uint32_t attributes;
   uint32_t size;
   DateTime modified;
} FtpDirEntry;


//FTP client related functions
error_t ftpClientInit(FtpClientContext *context);

#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)

error_t ftpClientRegisterTlsInitCallback(FtpClientContext *context,
   FtpClientTlsInitCallback callback);

#endif

error_t ftpClientSetTimeout(FtpClientContext *context, systime_t timeout);

error_t ftpClientBindToInterface(FtpClientContext *context,
   NetInterface *interface);

error_t ftpClientConnect(FtpClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort, uint_t mode);

error_t ftpClientLogin(FtpClientContext *context, const char_t *username,
   const char_t *password);

error_t ftpClientLoginEx(FtpClientContext *context, const char_t *username,
   const char_t *password, const char_t *account);

error_t ftpClientGetWorkingDir(FtpClientContext *context, char_t *path,
   size_t maxLen);

error_t ftpClientChangeWorkingDir(FtpClientContext *context,
   const char_t *path);

error_t ftpClientChangeToParentDir(FtpClientContext *context);

error_t ftpClientOpenDir(FtpClientContext *context, const char_t *path);
error_t ftpClientReadDir(FtpClientContext *context, FtpDirEntry *dirEntry);
error_t ftpClientCloseDir(FtpClientContext *context);

error_t ftpClientCreateDir(FtpClientContext *context, const char_t *path);
error_t ftpClientDeleteDir(FtpClientContext *context, const char_t *path);

error_t ftpClientOpenFile(FtpClientContext *context, const char_t *path,
   uint_t mode);

error_t ftpClientWriteFile(FtpClientContext *context, const void *data,
   size_t length, size_t *written, uint_t flags);

error_t ftpClientReadFile(FtpClientContext *context, void *data, size_t size,
   size_t *received, uint_t flags);

error_t ftpClientCloseFile(FtpClientContext *context);

error_t ftpClientRenameFile(FtpClientContext *context, const char_t *oldPath,
   const char_t *newPath);

error_t ftpClientDeleteFile(FtpClientContext *context, const char_t *path);

uint_t ftpClientGetReplyCode(FtpClientContext *context);

error_t ftpClientDisconnect(FtpClientContext *context);
error_t ftpClientClose(FtpClientContext *context);

void ftpClientDeinit(FtpClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
