/**
 * @file icecast_client.h
 * @brief Icecast client
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

#ifndef _ICECAST_CLIENT_H
#define _ICECAST_CLIENT_H

//Dependencies
#include "core/net.h"
#include "core/socket.h"

//Icecast client support
#ifndef ICECAST_CLIENT_SUPPORT
   #define ICECAST_CLIENT_SUPPORT DISABLED
#elif (ICECAST_CLIENT_SUPPORT != ENABLED && ICECAST_CLIENT_SUPPORT != DISABLED)
   #error ICECAST_CLIENT_SUPPORT parameter is not valid
#endif

//Stack size required to run the Icecast client
#ifndef ICECAST_CLIENT_STACK_SIZE
   #define ICECAST_CLIENT_STACK_SIZE 650
#elif (ICECAST_CLIENT_STACK_SIZE < 1)
   #error ICECAST_CLIENT_STACK_SIZE parameter is not valid
#endif

//Priority at which the Icecast client should run
#ifndef ICECAST_CLIENT_PRIORITY
   #define ICECAST_CLIENT_PRIORITY OS_TASK_PRIORITY_NORMAL
#endif

//Maximum time the Icecast client will wait before closing the connection
#ifndef ICECAST_CLIENT_TIMEOUT
   #define ICECAST_CLIENT_TIMEOUT 10000
#elif (ICECAST_CLIENT_TIMEOUT < 1000)
   #error ICECAST_CLIENT_TIMEOUT parameter is not valid
#endif

//Recovery delay
#ifndef ICECAST_RECOVERY_DELAY
   #define ICECAST_RECOVERY_DELAY 5000
#elif (ICECAST_RECOVERY_DELAY < 1000)
   #error ICECAST_RECOVERY_DELAY parameter is not valid
#endif

//Maximum length of the server name
#ifndef ICECAST_SERVER_NAME_MAX_LEN
   #define ICECAST_SERVER_NAME_MAX_LEN 48
#elif (ICECAST_SERVER_NAME_MAX_LEN < 1)
   #error ICECAST_SERVER_NAME_MAX_LEN parameter is not valid
#endif

//Maxmimum length of the requested resource
#ifndef ICECAST_RESOURCE_MAX_LEN
   #define ICECAST_RESOURCE_MAX_LEN 32
#elif (ICECAST_RESOURCE_MAX_LEN < 1)
   #error ICECAST_RESOURCE_MAX_LEN parameter is not valid
#endif

//Maximum size of metadata blocks
#ifndef ICECAST_CLIENT_METADATA_MAX_SIZE
   #define ICECAST_CLIENT_METADATA_MAX_SIZE 512
#elif (ICECAST_CLIENT_METADATA_MAX_SIZE < 128)
   #error ICECAST_CLIENT_METADATA_MAX_SIZE parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Icecast client settings
 **/

typedef struct
{
   NetInterface *interface;                        ///<Underlying network interface
   char_t serverName[ICECAST_SERVER_NAME_MAX_LEN]; ///<Icecast server name
   uint16_t serverPort;                            ///<Icecast server port
   char_t resource[ICECAST_RESOURCE_MAX_LEN];      ///<Requested resource
   size_t bufferSize;                              ///<Streaming buffer size
} IcecastClientSettings;


/**
 * @brief Icecast client context
 **/

typedef struct
{
   IcecastClientSettings settings;                    ///<User settings
   OsMutex mutex;                                     ///<Mutex protecting critical sections
   OsEvent writeEvent;                                ///<This event tells whether the buffer is writable
   OsEvent readEvent;                                 ///<This event tells whether the buffer is readable
   Socket *socket;                                    ///<Underlying socket
   size_t blockSize;                                  ///<Number of data bytes between subsequent metadata blocks
   uint8_t *streamBuffer;                             ///<Streaming buffer
   size_t bufferSize;                                 ///<Streaming buffer size
   size_t bufferLength;                               ///<Streaming buffer length
   size_t writeIndex;                                 ///<Current write index within the buffer
   size_t readIndex;                                  ///<Current read index within the buffer
   size_t totalLength;                                ///<Total number of bytes that have been received
   char_t buffer[ICECAST_CLIENT_METADATA_MAX_SIZE];   ///<Memory buffer for input/output operations
   char_t metadata[ICECAST_CLIENT_METADATA_MAX_SIZE]; ///<Metadata information
   size_t metadataLength;                             ///<Length of the metadata
} IcecastClientContext;


//Icecast related functions
void icecastClientGetDefaultSettings(IcecastClientSettings *settings);

error_t icecastClientInit(IcecastClientContext *context,
   const IcecastClientSettings *settings);

error_t icecastClientStart(IcecastClientContext *context);

error_t icecastClientReadStream(IcecastClientContext *context,
   uint8_t *data, size_t size, size_t *length, systime_t timeout);

error_t icecastClientReadMetadata(IcecastClientContext *context,
   char_t *metadata, size_t size, size_t *length);

void icecastClientTask(void *param);

error_t icecastClientConnect(IcecastClientContext *context);
error_t icecastClientProcessMetadata(IcecastClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
