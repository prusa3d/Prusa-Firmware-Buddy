/**
 * @file ipv6_pmtu.h
 * @brief Path MTU Discovery for IPv6
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

#ifndef _IPV6_PMTU_H
#define _IPV6_PMTU_H

//Dependencies
#include "core/net.h"

//Path MTU discovery support
#ifndef IPV6_PMTU_SUPPORT
   #define IPV6_PMTU_SUPPORT ENABLED
#elif (IPV6_PMTU_SUPPORT != ENABLED && IPV6_PMTU_SUPPORT != DISABLED)
   #error IPV6_PMTU_SUPPORT parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Path MTU discovery related functions
size_t ipv6GetPathMtu(NetInterface *interface, const Ipv6Addr *destAddr);

void ipv6UpdatePathMtu(NetInterface *interface,
   const Ipv6Addr *destAddr, size_t tentativePathMtu);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
