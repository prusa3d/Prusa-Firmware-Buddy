/**
 * @file dns_debug.h
 * @brief Data logging functions for debugging purpose (DNS)
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

#ifndef _DNS_DEBUG_H
#define _DNS_DEBUG_H

//Dependencies
#include "core/net.h"
#include "dns/dns_common.h"
#include "debug.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Check current trace level
#if (DNS_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   void dnsDumpMessage(const DnsHeader *message, size_t length);
   size_t dnsDumpQuestion(const DnsHeader *message, size_t length, size_t pos, char_t *buffer);
   size_t dnsDumpResourceRecord(const DnsHeader *message, size_t length, size_t pos, char_t *buffer);
#else
   #define dnsDumpMessage(message, length)
#endif

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
