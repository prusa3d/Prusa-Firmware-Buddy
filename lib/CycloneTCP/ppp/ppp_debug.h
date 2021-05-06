/**
 * @file ppp_debug.h
 * @brief Data logging functions for debugging purpose (PPP)
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

#ifndef _DHCP_DEBUG_H
#define _DHCP_DEBUG_H

//Dependencies
#include "core/net.h"
#include "ppp/ppp.h"
#include "ppp/lcp.h"
#include "ppp/ipcp.h"
#include "debug.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Check current trace level
#if (PPP_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   error_t pppDumpPacket(const PppPacket *packet, size_t length, PppProtocol protocol);
   error_t lcpDumpPacket(const PppPacket *packet, size_t length);
   error_t ncpDumpPacket(const PppPacket *packet, size_t length, PppProtocol protocol);
   error_t papDumpPacket(const PppPacket *packet, size_t length);
   error_t chapDumpPacket(const PppPacket *packet, size_t length);
   error_t lcpDumpOptions(const PppOption *option, size_t length);
   error_t ipcpDumpOptions(const PppOption *option, size_t length);
   error_t ipv6cpDumpOptions(const PppOption *option, size_t length);
#else
   #define pppDumpPacket(packet, length, protocol)
#endif

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
