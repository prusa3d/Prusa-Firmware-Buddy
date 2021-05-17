/**
 * @file dhcpv6_relay.h
 * @brief DHCPv6 relay agent (Dynamic Host Configuration Protocol for IPv6)
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

#ifndef _DHCPV6_RELAY_H
#define _DHCPV6_RELAY_H

//Dependencies
#include "dhcpv6/dhcpv6_common.h"
#include "core/socket.h"

//DHCPv6 relay agent support
#ifndef DHCPV6_RELAY_SUPPORT
   #define DHCPV6_RELAY_SUPPORT ENABLED
#elif (DHCPV6_RELAY_SUPPORT != ENABLED && DHCPV6_RELAY_SUPPORT != DISABLED)
   #error DHCPV6_RELAY_SUPPORT parameter is not valid
#endif

//Stack size required to run the DHCPv6 relay agent
#ifndef DHCPV6_RELAY_STACK_SIZE
   #define DHCPV6_RELAY_STACK_SIZE 500
#elif (DHCPV6_RELAY_STACK_SIZE < 1)
   #error DHCPV6_RELAY_STACK_SIZE parameter is not valid
#endif

//Priority at which the DHCPv6 relay agent should run
#ifndef DHCPV6_RELAY_PRIORITY
   #define DHCPV6_RELAY_PRIORITY OS_TASK_PRIORITY_NORMAL
#endif

//Maximum number of client-facing interfaces
#ifndef DHCPV6_RELAY_MAX_CLIENT_IF
   #define DHCPV6_RELAY_MAX_CLIENT_IF 4
#elif (DHCPV6_RELAY_MAX_CLIENT_IF < 1)
   #error DHCPV6_RELAY_MAX_CLIENT_IF parameter is not valid
#endif

//The amount of overhead added by relay forwarding
#define DHCPV6_RELAY_FORW_OVERHEAD (sizeof(Dhcpv6RelayMessage) + 2 * sizeof(Dhcpv6Option) + sizeof(uint32_t))

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DHCPv6 relay agent settings
 **/

typedef struct
{
   NetInterface *serverInterface;                             ///<Network-facing interface
   NetInterface *clientInterface[DHCPV6_RELAY_MAX_CLIENT_IF]; ///<Client-facing interfaces
   uint_t clientInterfaceCount;                               ///<Number of client-facing interfaces
   Ipv6Addr serverAddress;                                    ///<Address to be used when relaying messages to the server
} Dhcpv6RelaySettings;


/**
 * @brief DHCPv6 relay agent context
 **/

typedef struct
{
   NetInterface *serverInterface;                             ///<Network-facing interface
   NetInterface *clientInterface[DHCPV6_RELAY_MAX_CLIENT_IF]; ///<Client-facing interfaces
   uint_t clientInterfaceCount;                               ///<Number of client-facing interfaces
   Ipv6Addr serverAddress;                                    ///<Address to be used when relaying messages to the server
   Socket *serverSocket;                                      ///<Socket that handles the network-facing interface
   Socket *clientSocket[DHCPV6_RELAY_MAX_CLIENT_IF];          ///<Sockets that handle client-facing interfaces
   SocketEventDesc eventDesc[DHCPV6_RELAY_MAX_CLIENT_IF];     ///<The events the application is interested in
   bool_t running;                                            ///<DHCPv6 relay agent is currently running or not?
   bool_t stopRequest;                                        ///<Stop request
   OsEvent ackEvent;                                          ///<Event object use to acknowledge user requests
   OsEvent event;                                             ///<Event object used to poll the sockets
   uint8_t buffer[DHCPV6_MAX_MSG_SIZE];                       ///<Scratch buffer to store DHCPv6 messages
} Dhcpv6RelayContext;


//DHCPv6 relay agent specific functions
error_t dhcpv6RelayStart(Dhcpv6RelayContext *context, const Dhcpv6RelaySettings *settings);
error_t dhcpv6RelayStop(Dhcpv6RelayContext *context);

error_t dhcpv6RelayJoinMulticastGroup(Dhcpv6RelayContext *context);
error_t dhcpv6RelayLeaveMulticastGroup(Dhcpv6RelayContext *context);

void dhcpv6RelayTask(void *param);

error_t dhcpv6ForwardClientMessage(Dhcpv6RelayContext *context, uint_t index);
error_t dhcpv6ForwardRelayReplyMessage(Dhcpv6RelayContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
