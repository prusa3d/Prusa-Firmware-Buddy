/**
 * @file dhcpv6_debug.h
 * @brief Data logging functions for debugging purpose (DHCPv6)
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

#ifndef _DHCPV6_DEBUG_H
#define _DHCPV6_DEBUG_H

//Dependencies
#include "core/net.h"
#include "dhcpv6/dhcpv6_common.h"
#include "debug.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Check current trace level
#if (DHCPV6_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   error_t dhcpv6DumpMessage(const void *message, size_t length);
   error_t dhcpv6DumpOptions(const uint8_t *options, size_t length, uint_t level);
   error_t dhcpv6DumpGenericOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpClientIdOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpServerIdOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpIaNaOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpIaTaOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpIaAddrOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpOroOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpPreferenceOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpElapsedTimeOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpRelayMessageOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpAuthOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpServerUnicastOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpStatusCodeOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpRapidCommitOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpUserClassOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpVendorClassOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpVendorSpecificInfoOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpInterfaceIdOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpReconfMessageOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpReconfAcceptOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpDnsServersOption(const Dhcpv6Option *option, uint_t level);
   error_t dhcpv6DumpDomainListOption(const Dhcpv6Option *option, uint_t level);
#else
   #define dhcpv6DumpMessage(message, length)
#endif

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
