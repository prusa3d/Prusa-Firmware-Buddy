/**
 * @file ipv4_misc.h
 * @brief Helper functions for IPv4
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

#ifndef _IPV4_MISC_H
#define _IPV4_MISC_H

//Dependencies
#include <string.h>
#include "core/net.h"
#include "ipv4/ipv4.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//IPv4 related functions
error_t ipv4AddRouterAlertOption(NetBuffer *buffer, size_t *offset);

error_t ipv4CheckSourceAddr(NetInterface *interface, Ipv4Addr ipAddr);
error_t ipv4CheckDestAddr(NetInterface *interface, Ipv4Addr ipAddr);

error_t ipv4SelectSourceAddr(NetInterface **interface,
   Ipv4Addr destAddr, Ipv4Addr *srcAddr);

error_t ipv4SelectDefaultGateway(NetInterface *interface, Ipv4Addr srcAddr,
   Ipv4Addr *defaultGatewayAddr);

bool_t ipv4IsOnLink(NetInterface *interface, Ipv4Addr ipAddr);
bool_t ipv4IsBroadcastAddr(NetInterface *interface, Ipv4Addr ipAddr);
bool_t ipv4IsTentativeAddr(NetInterface *interface, Ipv4Addr ipAddr);
bool_t ipv4IsLocalHostAddr(Ipv4Addr ipAddr);

uint_t ipv4GetAddrScope(Ipv4Addr ipAddr);
uint_t ipv4GetPrefixLength(Ipv4Addr mask);

error_t ipv4GetBroadcastAddr(NetInterface *interface, Ipv4Addr *addr);

error_t ipv4MapMulticastAddrToMac(Ipv4Addr ipAddr, MacAddr *macAddr);

void ipv4UpdateInStats(NetInterface *interface, Ipv4Addr destIpAddr,
   size_t length);

void ipv4UpdateOutStats(NetInterface *interface, Ipv4Addr destIpAddr,
   size_t length);

void ipv4UpdateErrorStats(NetInterface *interface, error_t error);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
