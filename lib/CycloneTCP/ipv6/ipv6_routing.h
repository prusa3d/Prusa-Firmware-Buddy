/**
 * @file ipv6_routing.h
 * @brief IPv6 routing
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

#ifndef _IPV6_ROUTING_H
#define _IPV6_ROUTING_H

//Dependencies
#include "core/net.h"
#include "ipv6/ipv6.h"

//IPv6 routing support
#ifndef IPV6_ROUTING_SUPPORT
   #define IPV6_ROUTING_SUPPORT DISABLED
#elif (IPV6_ROUTING_SUPPORT != ENABLED && IPV6_ROUTING_SUPPORT != DISABLED)
   #error IPV6_ROUTING_SUPPORT parameter is not valid
#endif

//Size of the IPv6 routing table
#ifndef IPV6_ROUTING_TABLE_SIZE
   #define IPV6_ROUTING_TABLE_SIZE 8
#elif (IPV6_ROUTING_TABLE_SIZE < 1)
   #error IPV6_ROUTING_TABLE_SIZE parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Routing table entry
 **/

typedef struct
{
   bool_t valid;            ///<Valid entry
   Ipv6Addr prefix;         ///<Destination
   uint_t prefixLen;        ///<IPv6 prefix length
   NetInterface *interface; ///<Outgoing network interface
   Ipv6Addr nextHop;        ///<Next hop
   uint_t metric;           ///<Metric value
} Ipv6RoutingTableEntry;


//IPv6 routing related functions
error_t ipv6InitRouting(void);
error_t ipv6EnableRouting(NetInterface *interface, bool_t enable);

error_t ipv6AddRoute(const Ipv6Addr *prefix, uint_t prefixLen,
   NetInterface *interface, const Ipv6Addr *nextHop, uint_t metric);

error_t ipv6DeleteRoute(const Ipv6Addr *prefix, uint_t prefixLen);
error_t ipv6DeleteAllRoutes(void);

error_t ipv6ForwardPacket(NetInterface *srcInterface, NetBuffer *ipPacket,
   size_t ipPacketOffset);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
