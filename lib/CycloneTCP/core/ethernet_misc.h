/**
 * @file ethernet_misc.h
 * @brief Helper functions for Ethernet
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

#ifndef _ETHERNET_MISC_H
#define _ETHERNET_MISC_H

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Ethernet related constants
extern const uint8_t ethPadding[64];

//Ethernet related functions
error_t ethPadFrame(NetBuffer *buffer, size_t *length);

error_t ethEncodeVlanTag(NetBuffer *buffer, size_t *offset, uint16_t vlanId,
   int8_t vlanPcp, int8_t vlanDei, uint16_t type);

error_t ethDecodeVlanTag(const uint8_t *frame, size_t length, uint16_t *vlanId,
   uint16_t *type);

error_t ethCheckDestAddr(NetInterface *interface, const MacAddr *macAddr);

void ethUpdateInStats(NetInterface *interface, const MacAddr *destMacAddr);

void ethUpdateOutStats(NetInterface *interface, const MacAddr *destMacAddr,
   size_t length);

void ethUpdateErrorStats(NetInterface *interface, error_t error);

uint32_t ethCalcCrc(const void *data, size_t length);
uint32_t ethCalcCrcEx(const NetBuffer *buffer, size_t offset, size_t length);

error_t ethCheckCrc(NetInterface *interface, const uint8_t *frame,
   size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
