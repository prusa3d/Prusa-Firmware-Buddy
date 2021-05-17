/**
 * @file nbns_client.h
 * @brief NBNS client (NetBIOS Name Service)
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

#ifndef _NBNS_CLIENT_H
#define _NBNS_CLIENT_H

//Dependencies
#include "core/net.h"
#include "core/socket.h"
#include "core/udp.h"
#include "dns/dns_cache.h"
#include "dns/dns_common.h"
#include "netbios/nbns_common.h"

//NBNS client support
#ifndef NBNS_CLIENT_SUPPORT
   #define NBNS_CLIENT_SUPPORT ENABLED
#elif (NBNS_CLIENT_SUPPORT != ENABLED && NBNS_CLIENT_SUPPORT != DISABLED)
   #error NBNS_CLIENT_SUPPORT parameter is not valid
#endif

//Maximum number of retransmissions of NBNS queries
#ifndef NBNS_CLIENT_MAX_RETRIES
   #define NBNS_CLIENT_MAX_RETRIES 3
#elif (NBNS_CLIENT_MAX_RETRIES < 1)
   #error NBNS_CLIENT_MAX_RETRIES parameter is not valid
#endif

//Initial retransmission timeout
#ifndef NBNS_CLIENT_INIT_TIMEOUT
   #define NBNS_CLIENT_INIT_TIMEOUT 1000
#elif (NBNS_CLIENT_INIT_TIMEOUT < 1000)
   #error NBNS_CLIENT_INIT_TIMEOUT parameter is not valid
#endif

//Maximum retransmission timeout
#ifndef NBNS_CLIENT_MAX_TIMEOUT
   #define NBNS_CLIENT_MAX_TIMEOUT 1000
#elif (NBNS_CLIENT_MAX_TIMEOUT < 1000)
   #error NBNS_CLIENT_MAX_TIMEOUT parameter is not valid
#endif

//Maximum cache lifetime for NBNS entries
#ifndef NBNS_MAX_LIFETIME
   #define NBNS_MAX_LIFETIME 60000
#elif (NBNS_MAX_LIFETIME < 1000)
   #error NBNS_MAX_LIFETIME parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//NBNS related functions
error_t nbnsResolve(NetInterface *interface, const char_t *name, IpAddr *ipAddr);

error_t nbnsSendQuery(DnsCacheEntry *entry);

void nbnsProcessResponse(NetInterface *interface, const Ipv4PseudoHeader *pseudoHeader,
   const UdpHeader *udpHeader, const NbnsHeader *message, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
