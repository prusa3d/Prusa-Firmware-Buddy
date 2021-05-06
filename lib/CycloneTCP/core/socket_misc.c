/**
 * @file socket_misc.c
 * @brief Helper functions for sockets
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

//Switch to the appropriate trace level
#define TRACE_LEVEL SOCKET_TRACE_LEVEL

//Dependencies
#include <string.h>
#include "core/net.h"
#include "core/socket.h"
#include "core/socket_misc.h"
#include "core/raw_socket.h"
#include "core/udp.h"
#include "core/tcp.h"
#include "core/tcp_misc.h"
#include "debug.h"


/**
 * @brief Allocate a socket
 * @param[in] type Type specification for the new socket
 * @param[in] protocol Protocol to be used
 * @return Handle referencing the new socket
 **/

Socket *socketAllocate(uint_t type, uint_t protocol)
{
   error_t error;
   uint_t i;
   uint16_t port;
   Socket *socket;
   OsEvent event;

   //Initialize socket handle
   socket = NULL;

#if (TCP_SUPPORT == ENABLED)
   //Connection-oriented socket?
   if(type == SOCKET_TYPE_STREAM)
   {
      //Always use TCP as underlying transport protocol
      protocol = SOCKET_IP_PROTO_TCP;
      //Get an ephemeral port number
      port = tcpGetDynamicPort();
      //Continue processing
      error = NO_ERROR;
   }
   else
#endif
#if (UDP_SUPPORT == ENABLED)
   //Connectionless socket?
   if(type == SOCKET_TYPE_DGRAM)
   {
      //Always use UDP as underlying transport protocol
      protocol = SOCKET_IP_PROTO_UDP;
      //Get an ephemeral port number
      port = udpGetDynamicPort();
      //Continue processing
      error = NO_ERROR;
   }
   else
#endif
#if (RAW_SOCKET_SUPPORT == ENABLED)
   //Raw socket?
   if(type == SOCKET_TYPE_RAW_IP || type == SOCKET_TYPE_RAW_ETH)
   {
      //Port numbers are not relevant for raw sockets
      port = 0;
      //Continue processing
      error = NO_ERROR;
   }
   else
#endif
   {
      //The socket type is not supported
      error = ERROR_INVALID_PARAMETER;
   }

   //Check status code
   if(!error)
   {
      //Loop through socket descriptors
      for(i = 0; i < SOCKET_MAX_COUNT; i++)
      {
         //Unused socket found?
         if(socketTable[i].type == SOCKET_TYPE_UNUSED)
         {
            //Save socket handle
            socket = &socketTable[i];
            //We are done
            break;
         }
      }

#if (TCP_SUPPORT == ENABLED)
      //No more sockets available?
      if(socket == NULL)
      {
         //Kill the oldest connection in the TIME-WAIT state whenever the
         //socket table runs out of space
         socket = tcpKillOldestConnection();
      }
#endif

      //Check whether the current entry is free
      if(socket != NULL)
      {
         //Save socket descriptor
         i = socket->descriptor;
         //Save event object instance
         osMemcpy(&event, &socket->event, sizeof(OsEvent));

         //Clear associated structure
         osMemset(socket, 0, sizeof(Socket));
         //Reuse event objects and avoid recreating them whenever possible
         osMemcpy(&socket->event, &event, sizeof(OsEvent));

         //Save socket characteristics
         socket->descriptor = i;
         socket->type = type;
         socket->protocol = protocol;
         socket->localPort = port;
         socket->timeout = INFINITE_DELAY;

#if (ETH_VLAN_SUPPORT == ENABLED)
         //Default VLAN PCP and DEI fields
         socket->vlanPcp = -1;
         socket->vlanDei = -1;
#endif

#if (ETH_VMAN_SUPPORT == ENABLED)
         //Default VMAN PCP and DEI fields
         socket->vmanPcp = -1;
         socket->vmanDei = -1;
#endif

#if (TCP_SUPPORT == ENABLED && TCP_KEEP_ALIVE_SUPPORT == ENABLED)
         //TCP keep-alive mechanism must be disabled by default (refer to
         //RFC 1122, section 4.2.3.6)
         socket->keepAliveEnabled = FALSE;

         //Default TCP keep-alive parameters
         socket->keepAliveIdle = TCP_DEFAULT_KEEP_ALIVE_IDLE;
         socket->keepAliveInterval = TCP_DEFAULT_KEEP_ALIVE_INTERVAL;
         socket->keepAliveMaxProbes = TCP_DEFAULT_KEEP_ALIVE_PROBES;
#endif

#if (TCP_SUPPORT == ENABLED)
         //Default TX and RX buffer size
         socket->txBufferSize = MIN(TCP_DEFAULT_TX_BUFFER_SIZE, TCP_MAX_TX_BUFFER_SIZE);
         socket->rxBufferSize = MIN(TCP_DEFAULT_RX_BUFFER_SIZE, TCP_MAX_RX_BUFFER_SIZE);
#endif
      }
   }

   //Return a handle to the freshly created socket
   return socket;
}


/**
 * @brief Subscribe to the specified socket events
 * @param[in] socket Handle that identifies a socket
 * @param[in] event Event object used to receive notifications
 * @param[in] eventMask Logic OR of the requested socket events
 **/

void socketRegisterEvents(Socket *socket, OsEvent *event, uint_t eventMask)
{
   //Valid socket handle?
   if(socket != NULL)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //An user event may have been previously registered...
      if(socket->userEvent != NULL)
      {
         socket->eventMask |= eventMask;
      }
      else
      {
         socket->eventMask = eventMask;
      }

      //Suscribe to get notified of events
      socket->userEvent = event;

#if (TCP_SUPPORT == ENABLED)
      //Handle TCP specific events
      if(socket->type == SOCKET_TYPE_STREAM)
      {
         tcpUpdateEvents(socket);
      }
#endif
#if (UDP_SUPPORT == ENABLED)
      //Handle UDP specific events
      if(socket->type == SOCKET_TYPE_DGRAM)
      {
         udpUpdateEvents(socket);
      }
#endif
#if (RAW_SOCKET_SUPPORT == ENABLED)
      //Handle events that are specific to raw sockets
      if(socket->type == SOCKET_TYPE_RAW_IP ||
         socket->type == SOCKET_TYPE_RAW_ETH)
      {
         rawSocketUpdateEvents(socket);
      }
#endif

      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}


/**
 * @brief Unsubscribe previously registered events
 * @param[in] socket Handle that identifies a socket
 **/

void socketUnregisterEvents(Socket *socket)
{
   //Valid socket handle?
   if(socket != NULL)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Unsuscribe socket events
      socket->userEvent = NULL;

      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}


/**
 * @brief Retrieve event flags for a specified socket
 * @param[in] socket Handle that identifies a socket
 * @return Logic OR of events in the signaled state
 **/

uint_t socketGetEvents(Socket *socket)
{
   uint_t eventFlags;

   //Valid socket handle?
   if(socket != NULL)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Read event flags for the specified socket
      eventFlags = socket->eventFlags;

      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
   else
   {
      //The socket handle is not valid
      eventFlags = 0;
   }

   //Return the events in the signaled state
   return eventFlags;
}
