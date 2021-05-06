/**
 * @file ftp_server.h
 * @brief FTP server (File Transfer Protocol)
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

#ifndef _FTP_SERVER_H
#define _FTP_SERVER_H

//Dependencies
#include "core/net.h"
#include "core/socket.h"
#include "fs_port.h"

//FTP server support
#ifndef FTP_SERVER_SUPPORT
   #define FTP_SERVER_SUPPORT ENABLED
#elif (FTP_SERVER_SUPPORT != ENABLED && FTP_SERVER_SUPPORT != DISABLED)
   #error FTP_SERVER_SUPPORT parameter is not valid
#endif

//FTP over TLS
#ifndef FTP_SERVER_TLS_SUPPORT
   #define FTP_SERVER_TLS_SUPPORT DISABLED
#elif (FTP_SERVER_TLS_SUPPORT != ENABLED && FTP_SERVER_TLS_SUPPORT != DISABLED)
   #error FTP_SERVER_TLS_SUPPORT parameter is not valid
#endif

//Stack size required to run the FTP server
#ifndef FTP_SERVER_STACK_SIZE
   #define FTP_SERVER_STACK_SIZE 650
#elif (FTP_SERVER_STACK_SIZE < 1)
   #error FTP_SERVER_STACK_SIZE parameter is not valid
#endif

//Priority at which the FTP server should run
#ifndef FTP_SERVER_PRIORITY
   #define FTP_SERVER_PRIORITY OS_TASK_PRIORITY_NORMAL
#endif

//Maximum number of simultaneous connections
#ifndef FTP_SERVER_MAX_CONNECTIONS
   #define FTP_SERVER_MAX_CONNECTIONS 10
#elif (FTP_SERVER_MAX_CONNECTIONS < 1)
   #error FTP_SERVER_MAX_CONNECTIONS parameter is not valid
#endif

//Maximum time the server will wait before closing the connection
#ifndef FTP_SERVER_TIMEOUT
   #define FTP_SERVER_TIMEOUT 60000
#elif (FTP_SERVER_TIMEOUT < 1000)
   #error FTP_SERVER_TIMEOUT parameter is not valid
#endif

//FTP server tick interval
#ifndef FTP_SERVER_TICK_INTERVAL
   #define FTP_SERVER_TICK_INTERVAL 1000
#elif (FTP_SERVER_TICK_INTERVAL < 100)
   #error FTP_SERVER_TICK_INTERVAL parameter is not valid
#endif

//Maximum length of the pending connection queue
#ifndef FTP_SERVER_BACKLOG
   #define FTP_SERVER_BACKLOG 4
#elif (FTP_SERVER_BACKLOG < 1)
   #error FTP_SERVER_BACKLOG parameter is not valid
#endif

//Maximum line length
#ifndef FTP_SERVER_MAX_LINE_LEN
   #define FTP_SERVER_MAX_LINE_LEN 255
#elif (FTP_SERVER_MAX_LINE_LEN < 64)
   #error FTP_SERVER_MAX_LINE_LEN parameter is not valid
#endif

//Size of buffer used for input/output operations
#ifndef FTP_SERVER_BUFFER_SIZE
   #define FTP_SERVER_BUFFER_SIZE 1024
#elif (FTP_SERVER_BUFFER_SIZE < 128)
   #error FTP_SERVER_BUFFER_SIZE parameter is not valid
#endif

//Maximum size of root directory
#ifndef FTP_SERVER_MAX_ROOT_DIR_LEN
   #define FTP_SERVER_MAX_ROOT_DIR_LEN 63
#elif (FTP_SERVER_MAX_ROOT_DIR_LEN < 7)
   #error FTP_SERVER_MAX_ROOT_DIR_LEN parameter is not valid
#endif

//Maximum size of home directory
#ifndef FTP_SERVER_MAX_HOME_DIR_LEN
   #define FTP_SERVER_MAX_HOME_DIR_LEN 63
#elif (FTP_SERVER_MAX_HOME_DIR_LEN < 7)
   #error FTP_SERVER_MAX_HOME_DIR_LEN parameter is not valid
#endif

//Maximum user name length
#ifndef FTP_SERVER_MAX_USERNAME_LEN
   #define FTP_SERVER_MAX_USERNAME_LEN 63
#elif (FTP_SERVER_MAX_USERNAME_LEN < 7)
   #error FTP_SERVER_MAX_USERNAME_LEN parameter is not valid
#endif

//Maximum path length
#ifndef FTP_SERVER_MAX_PATH_LEN
   #define FTP_SERVER_MAX_PATH_LEN 255
#elif (FTP_SERVER_MAX_PATH_LEN < 7)
   #error FTP_SERVER_MAX_PATH_LEN parameter is not valid
#endif

//Minimum buffer size for TCP sockets
#ifndef FTP_SERVER_MIN_TCP_BUFFER_SIZE
   #define FTP_SERVER_MIN_TCP_BUFFER_SIZE 1430
#elif (FTP_SERVER_MIN_TCP_BUFFER_SIZE < 512)
   #error FTP_SERVER_MIN_TCP_BUFFER_SIZE parameter is not valid
#endif

//Maximum buffer size for TCP sockets
#ifndef FTP_SERVER_MAX_TCP_BUFFER_SIZE
   #define FTP_SERVER_MAX_TCP_BUFFER_SIZE 2860
#elif (FTP_SERVER_MAX_TCP_BUFFER_SIZE < 512)
   #error FTP_SERVER_MAX_TCP_BUFFER_SIZE parameter is not valid
#endif

//TX buffer size for TLS connections
#ifndef FTP_SERVER_TLS_TX_BUFFER_SIZE
   #define FTP_SERVER_TLS_TX_BUFFER_SIZE 4096
#elif (FTP_SERVER_TLS_TX_BUFFER_SIZE < 512)
   #error FTP_SERVER_TLS_TX_BUFFER_SIZE parameter is not valid
#endif

//Minimum RX buffer size for TLS connections
#ifndef FTP_SERVER_MIN_TLS_RX_BUFFER_SIZE
   #define FTP_SERVER_MIN_TLS_RX_BUFFER_SIZE 2048
#elif (FTP_SERVER_MIN_TLS_RX_BUFFER_SIZE < 512)
   #error FTP_SERVER_MIN_TLS_RX_BUFFER_SIZE parameter is not valid
#endif

//Maximum RX buffer size for TLS connections
#ifndef FTP_SERVER_MAX_TLS_RX_BUFFER_SIZE
   #define FTP_SERVER_MAX_TLS_RX_BUFFER_SIZE 16384
#elif (FTP_SERVER_MAX_TLS_RX_BUFFER_SIZE < 512)
   #error FTP_SERVER_MAX_TLS_RX_BUFFER_SIZE parameter is not valid
#endif

//Passive port range (lower limit)
#ifndef FTP_SERVER_PASSIVE_PORT_MIN
   #define FTP_SERVER_PASSIVE_PORT_MIN 48128
#elif (FTP_SERVER_PASSIVE_PORT_MIN < 1024)
   #error FTP_SERVER_PASSIVE_PORT_MIN parameter is not valid
#endif

//Passive port range (upper limit)
#ifndef FTP_SERVER_PASSIVE_PORT_MAX
   #define FTP_SERVER_PASSIVE_PORT_MAX 49151
#elif (FTP_SERVER_PASSIVE_PORT_MAX <= FTP_SERVER_PASSIVE_PORT_MIN || FTP_SERVER_PASSIVE_PORT_MAX > 65535)
   #error FTP_SERVER_PASSIVE_PORT_MAX parameter is not valid
#endif

//TLS supported?
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
   #include "tls_ticket.h"
#endif

//FTP port number
#define FTP_PORT 21
//FTP data port number
#define FTP_DATA_PORT 20

//FTPS port number (implicit mode)
#define FTPS_PORT 990
//FTPS data port number (implicit mode)
#define FTPS_DATA_PORT 989

//Forward declaration of FtpServerContext structure
struct _FtpServerContext;
#define FtpServerContext struct _FtpServerContext

//Forward declaration of FtpClientConnection structure
struct _FtpClientConnection;
#define FtpClientConnection struct _FtpClientConnection

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Channel state
 **/

typedef enum
{
   FTP_CHANNEL_STATE_CLOSED       = 0,
   FTP_CHANNEL_STATE_CONNECT_TLS  = 1,
   FTP_CHANNEL_STATE_LISTEN       = 2,
   FTP_CHANNEL_STATE_IDLE         = 3,
   FTP_CHANNEL_STATE_SEND         = 4,
   FTP_CHANNEL_STATE_RECEIVE      = 5,
   FTP_CHANNEL_STATE_DISCARD      = 6,
   FTP_CHANNEL_STATE_AUTH_TLS_1   = 7,
   FTP_CHANNEL_STATE_AUTH_TLS_2   = 8,
   FTP_CHANNEL_STATE_USER         = 9,
   FTP_CHANNEL_STATE_LIST         = 10,
   FTP_CHANNEL_STATE_NLST         = 11,
   FTP_CHANNEL_STATE_RETR         = 12,
   FTP_CHANNEL_STATE_STOR         = 13,
   FTP_CHANNEL_STATE_APPE         = 14,
   FTP_CHANNEL_STATE_RNFR         = 15,
   FTP_CHANNEL_STATE_SHUTDOWN_TLS = 16,
   FTP_CHANNEL_STATE_WAIT_ACK     = 17,
   FTP_CHANNEL_STATE_SHUTDOWN_TX  = 18,
   FTP_CHANNEL_STATE_SHUTDOWN_RX  = 19
} FtpServerChannelState;


/**
 * @brief Security modes
 **/

typedef enum
{
   FTP_SERVER_MODE_PLAINTEXT    = 1,
   FTP_SERVER_MODE_IMPLICIT_TLS = 2,
   FTP_SERVER_MODE_EXPLICIT_TLS = 4
} FtpServerMode;


/**
 * @brief FTP server access status
 **/

typedef enum
{
   FTP_ACCESS_DENIED     = 0,
   FTP_ACCESS_ALLOWED    = 1,
   FTP_PASSWORD_REQUIRED = 2
} FtpAccessStatus;


/**
 * @brief File permissions
 **/

typedef enum
{
   FTP_FILE_PERM_LIST  = 0x01,
   FTP_FILE_PERM_READ  = 0x02,
   FTP_FILE_PERM_WRITE = 0x04
} FtpFilePerm;


/**
 * @brief Connection callback function
 **/

typedef error_t (*FtpServerConnectCallback)(FtpClientConnection *connection,
   const IpAddr *clientIpAddr, uint16_t clientPort);


/**
 * @brief Disconnection callback function
 **/

typedef void (*FtpServerDisconnectCallback)(FtpClientConnection *connection,
   const IpAddr *clientIpAddr, uint16_t clientPort);


//TLS supported?
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)

/**
 * @brief TLS initialization callback function
 **/

typedef error_t (*FtpServerTlsInitCallback)(FtpClientConnection *connection,
   TlsContext *tlsContext);

#endif


/**
 * @brief User verification callback function
 **/

typedef uint_t (*FtpServerCheckUserCallback)(FtpClientConnection *connection,
   const char_t *user);


/**
 * @brief Password verification callback function
 **/

typedef uint_t (*FtpServerCheckPasswordCallback)(FtpClientConnection *connection,
   const char_t *user, const char_t *password);


/**
 * @brief Callback used to retrieve file permissions
 **/

typedef uint_t (*FtpServerGetFilePermCallback)(FtpClientConnection *connection,
   const char_t *user, const char_t *path);


/**
 * @brief Unknown command callback function
 **/

typedef error_t (*FtpServerUnknownCommandCallback)(FtpClientConnection *connection,
   const char_t *command, const char_t *param);


/**
 * @brief FTP server settings
 **/

typedef struct
{
   NetInterface *interface;                                ///<Underlying network interface
   uint16_t port;                                          ///<FTP command port number
   uint16_t dataPort;                                      ///<FTP data port number
   uint16_t passivePortMin;                                ///<Passive port range (lower value)
   uint16_t passivePortMax;                                ///<Passive port range (upper value)
   Ipv4Addr publicIpv4Addr;                                ///<Public IPv4 address to be used in PASV replies
   uint_t mode;                                            ///<Security modes
   uint_t maxConnections;                                  ///<Maximum number of client connections
   FtpClientConnection *connections;                       ///<Client connections
   char_t rootDir[FTP_SERVER_MAX_ROOT_DIR_LEN + 1];        ///<Root directory
   FtpServerConnectCallback connectCallback;               ///<Connection callback function
   FtpServerDisconnectCallback disconnectCallback;         ///<Disconnection callback function
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   FtpServerTlsInitCallback tlsInitCallback;               ///<TLS initialization callback function
#endif
   FtpServerCheckUserCallback checkUserCallback;           ///<User verification callback function
   FtpServerCheckPasswordCallback checkPasswordCallback;   ///<Password verification callback function
   FtpServerGetFilePermCallback getFilePermCallback;       ///<Callback used to retrieve file permissions
   FtpServerUnknownCommandCallback unknownCommandCallback; ///<Unknown command callback function
} FtpServerSettings;


/**
 * @brief Control or data channel
 **/

typedef struct
{
   FtpServerChannelState state; ///<Channel state
   Socket *socket;              ///<Underlying TCP socket
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   TlsContext *tlsContext;      ///<TLS context
#endif
} FtpServerChannel;


/**
 * @brief FTP client connection
 **/

struct _FtpClientConnection
{
   FtpServerContext *context;                       ///<FTP server context
   NetInterface *interface;                         ///<Underlying network interface
   bool_t userLoggedIn;                             ///<This flag tells whether the user is logged in
   systime_t timestamp;                             ///<Time stamp to manage timeout
   FtpServerChannel controlChannel;                 ///<Control channel
   FtpServerChannel dataChannel;                    ///<Data channel
   FsFile *file;                                    ///<File pointer
   FsDir *dir;                                      ///<Directory pointer
   bool_t passiveMode;                              ///<Passive data transfer
   IpAddr remoteIpAddr;                             ///<Remote IP address
   uint16_t remotePort;                             ///<Remote port number
   char_t user[FTP_SERVER_MAX_USERNAME_LEN + 1];    ///<User name
   char_t homeDir[FTP_SERVER_MAX_HOME_DIR_LEN + 1]; ///<Home directory
   char_t currentDir[FTP_SERVER_MAX_PATH_LEN + 1];  ///<Current directory
   char_t path[FTP_SERVER_MAX_PATH_LEN + 1];        ///<Pathname
   char_t command[FTP_SERVER_MAX_LINE_LEN + 1];     ///<Incoming command
   size_t commandLen;                               ///<Number of bytes available in the command buffer
   char_t response[FTP_SERVER_MAX_LINE_LEN + 1];    ///<Response buffer
   size_t responseLen;                              ///<Number of bytes available in the response buffer
   size_t responsePos;                              ///<Current position in the response buffer
   char_t buffer[FTP_SERVER_BUFFER_SIZE];           ///<Memory buffer for input/output operations
   size_t bufferLength;                             ///<Length of the buffer, in bytes
   size_t bufferPos;                                ///<Current position in the buffer
};


/**
 * @brief FTP server context
 **/

struct _FtpServerContext
{
   FtpServerSettings settings;                                    ///<User settings
   bool_t running;                                                ///<Operational state of the FTP server
   bool_t stop;                                                   ///<Stop request
   OsEvent event;                                                 ///<Event object used to poll the sockets
   Socket *socket;                                                ///<Listening socket
   uint16_t passivePort;                                          ///<Current passive port number
   FtpClientConnection *connections;                              ///<Client connections
   SocketEventDesc eventDesc[2 * FTP_SERVER_MAX_CONNECTIONS + 1]; ///<The events the application is interested in
#if (FTP_SERVER_TLS_SUPPORT == ENABLED && TLS_TICKET_SUPPORT == ENABLED)
   TlsTicketContext tlsTicketContext;                            ///<TLS ticket encryption context
#endif
};


//FTP server related functions
void ftpServerGetDefaultSettings(FtpServerSettings *settings);

error_t ftpServerInit(FtpServerContext *context,
   const FtpServerSettings *settings);

error_t ftpServerStart(FtpServerContext *context);
error_t ftpServerStop(FtpServerContext *context);

error_t ftpServerSetHomeDir(FtpClientConnection *connection,
   const char_t *homeDir);

void ftpServerTask(FtpServerContext *context);

void ftpServerDeinit(FtpServerContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
