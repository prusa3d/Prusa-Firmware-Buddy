/**
 * @file dhcp_debug.h
 * @brief Data logging functions for debugging purpose (DHCP)
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
#include "dhcp/dhcp_common.h"
#include "debug.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Check current trace level
#if (DHCP_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   error_t dhcpDumpMessage(const DhcpMessage *message, size_t length);
   error_t dhcpDumpMessageType(const DhcpOption *option);
   error_t dhcpDumpParamRequestList(const DhcpOption *option);
   error_t dhcpDumpBoolean(const DhcpOption *option);
   error_t dhcpDumpInt8(const DhcpOption *option);
   error_t dhcpDumpInt16(const DhcpOption *option);
   error_t dhcpDumpInt32(const DhcpOption *option);
   error_t dhcpDumpString(const DhcpOption *option);
   error_t dhcpDumpIpv4Addr(const DhcpOption *option);
   error_t dhcpDumpIpv4AddrList(const DhcpOption *option);
   error_t dhcpDumpRawData(const DhcpOption *option);
#else
   #define dhcpDumpMessage(message, length)
#endif

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
