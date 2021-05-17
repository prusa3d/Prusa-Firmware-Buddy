/**
 * @file ndp_misc.h
 * @brief Helper functions for NDP (Neighbor Discovery Protocol)
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

#ifndef _NDP_MISC_H
#define _NDP_MISC_H

//Dependencies
#include "core/net.h"
#include "ipv6/ndp.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//NDP related functions
void ndpParsePrefixInfoOption(NetInterface *interface, NdpPrefixInfoOption *option);

void ndpUpdateAddrList(NetInterface *interface);
void ndpUpdatePrefixList(NetInterface *interface);
void ndpUpdateDefaultRouterList(NetInterface *interface);

error_t ndpSelectDefaultRouter(NetInterface *interface,
   const Ipv6Addr *unreachableAddr, Ipv6Addr *addr);

bool_t ndpIsFirstHopRouter(NetInterface *interface,
   const Ipv6Addr *destAddr, const Ipv6Addr *nextHop);

error_t ndpSelectNextHop(NetInterface *interface, const Ipv6Addr *destAddr,
   const Ipv6Addr *unreachableNextHop, Ipv6Addr *nextHop, bool_t dontRoute);

void ndpUpdateNextHop(NetInterface *interface, const Ipv6Addr *nextHop);

void ndpAddOption(void *message, size_t *messageLen, uint8_t type,
   const void *value, size_t length);

void *ndpGetOption(uint8_t *options, size_t length, uint8_t type);

error_t ndpCheckOptions(const uint8_t *options, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
