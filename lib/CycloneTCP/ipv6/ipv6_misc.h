/**
 * @file ipv6_misc.h
 * @brief Helper functions for IPv6
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

#ifndef _IPV6_MISC_H
#define _IPV6_MISC_H

//Dependencies
#include "core/net.h"
#include "ipv6/ipv6.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//IPv6 related functions
Ipv6AddrState ipv6GetAddrState(NetInterface *interface, const Ipv6Addr *addr);

error_t ipv6SetAddr(NetInterface *interface, uint_t index,
   const Ipv6Addr *addr, Ipv6AddrState state, systime_t validLifetime,
   systime_t preferredLifetime, bool_t permanent);

void ipv6AddAddr(NetInterface *interface, const Ipv6Addr *addr,
   uint32_t validLifetime, uint32_t preferredLifetime);

void ipv6RemoveAddr(NetInterface *interface, const Ipv6Addr *addr);

void ipv6AddPrefix(NetInterface *interface, const Ipv6Addr *prefix,
   uint_t length, bool_t onLinkFlag, bool_t autonomousFlag,
   uint32_t validLifetime, uint32_t preferredLifetime);

void ipv6RemovePrefix(NetInterface *interface, const Ipv6Addr *prefix,
   uint_t length);

void ipv6AddDefaultRouter(NetInterface *interface, const Ipv6Addr *addr,
   uint16_t lifetime, uint8_t preference);

void ipv6RemoveDefaultRouter(NetInterface *interface, const Ipv6Addr *addr);

void ipv6FlushAddrList(NetInterface *interface);
void ipv6FlushPrefixList(NetInterface *interface);
void ipv6FlushDefaultRouterList(NetInterface *interface);
void ipv6FlushDnsServerList(NetInterface *interface);

error_t ipv6CheckSourceAddr(NetInterface *interface, const Ipv6Addr *ipAddr);
error_t ipv6CheckDestAddr(NetInterface *interface, const Ipv6Addr *ipAddr);

error_t ipv6SelectSourceAddr(NetInterface **interface,
   const Ipv6Addr *destAddr, Ipv6Addr *srcAddr);

bool_t ipv6IsOnLink(NetInterface *interface, const Ipv6Addr *ipAddr);
bool_t ipv6IsAnycastAddr(NetInterface *interface, const Ipv6Addr *ipAddr);
bool_t ipv6IsTentativeAddr(NetInterface *interface, const Ipv6Addr *ipAddr);
bool_t ipv6IsLocalHostAddr(const Ipv6Addr *ipAddr);

bool_t ipv6CompPrefix(const Ipv6Addr *ipAddr1, const Ipv6Addr *ipAddr2,
   size_t length);

uint_t ipv6GetAddrScope(const Ipv6Addr *ipAddr);
uint_t ipv6GetMulticastAddrScope(const Ipv6Addr *ipAddr);

uint_t ipv6GetCommonPrefixLength(const Ipv6Addr *ipAddr1,
   const Ipv6Addr *ipAddr2);

error_t ipv6ComputeSolicitedNodeAddr(const Ipv6Addr *ipAddr,
   Ipv6Addr *solicitedNodeAddr);

error_t ipv6MapMulticastAddrToMac(const Ipv6Addr *ipAddr, MacAddr *macAddr);

void ipv6GenerateLinkLocalAddr(const Eui64 *interfaceId, Ipv6Addr *ipAddr);

void ipv6UpdateInStats(NetInterface *interface, const Ipv6Addr *destIpAddr,
   size_t length);

void ipv6UpdateOutStats(NetInterface *interface, const Ipv6Addr *destIpAddr,
   size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
