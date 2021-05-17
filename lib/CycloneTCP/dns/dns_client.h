/**
 * @file dns_client.h
 * @brief DNS client (Domain Name System)
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

#ifndef _DNS_CLIENT_H
#define _DNS_CLIENT_H

//Dependencies
#include "core/net.h"
#include "core/socket.h"
#include "core/udp.h"
#include "dns/dns_cache.h"

//DNS client support
#ifndef DNS_CLIENT_SUPPORT
   #define DNS_CLIENT_SUPPORT ENABLED
#elif (DNS_CLIENT_SUPPORT != ENABLED && DNS_CLIENT_SUPPORT != DISABLED)
   #error DNS_CLIENT_SUPPORT parameter is not valid
#endif

//Maximum number of retransmissions of DNS queries
#ifndef DNS_CLIENT_MAX_RETRIES
   #define DNS_CLIENT_MAX_RETRIES 3
#elif (DNS_CLIENT_MAX_RETRIES < 1)
   #error DNS_CLIENT_MAX_RETRIES parameter is not valid
#endif

//Initial retransmission timeout
#ifndef DNS_CLIENT_INIT_TIMEOUT
   #define DNS_CLIENT_INIT_TIMEOUT 1000
#elif (DNS_CLIENT_INIT_TIMEOUT < 1000)
   #error DNS_CLIENT_INIT_TIMEOUT parameter is not valid
#endif

//Maximum retransmission timeout
#ifndef DNS_CLIENT_MAX_TIMEOUT
   #define DNS_CLIENT_MAX_TIMEOUT 5000
#elif (DNS_CLIENT_MAX_TIMEOUT < 1000)
   #error DNS_CLIENT_MAX_TIMEOUT parameter is not valid
#endif

//Minimum cache lifetime for DNS entries
#ifndef DNS_MIN_LIFETIME
   #define DNS_MIN_LIFETIME 1000
#elif (DNS_MIN_LIFETIME < 0)
   #error DNS_MIN_LIFETIME parameter is not valid
#endif

//Maximum cache lifetime for DNS entries
#ifndef DNS_MAX_LIFETIME
   #define DNS_MAX_LIFETIME 3600000
#elif (DNS_MAX_LIFETIME < 1000 || DNS_MAX_LIFETIME < DNS_MIN_LIFETIME)
   #error DNS_MAX_LIFETIME parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//DNS related functions
error_t dnsResolve(NetInterface *interface, const char_t *name,
   HostType type, IpAddr *ipAddr);

error_t dnsSendQuery(DnsCacheEntry *entry);

void dnsProcessResponse(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
